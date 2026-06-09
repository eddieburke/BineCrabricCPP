#include "net/minecraft/client/platform/audio/AudioEngine.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/platform/audio/backend/audio_backend.h"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::platform::audio {
namespace {

constexpr const char* kMusicSlot = "BgMusic";
constexpr const char* kRecordSlot = "streaming";

constexpr float kUiSoundScale = 0.25f;
constexpr float kWorldAttenuationDistance = 16.0f;
constexpr float kRecordAttenuationDistance = 64.0f;
constexpr float kRecordVolumeScale = 0.5f;

struct RegisteredSound {
    std::string id;
    std::string path;
};

// Groups variant files (step1.ogg, step2.ogg) under one lookup id (step).
struct SoundRegistry {
    bool pickRandomVariant = true;
    mutable JavaRandom random;
    std::unordered_map<std::string, std::vector<const RegisteredSound*>> byBaseId;
    std::vector<std::unique_ptr<RegisteredSound>> owned;
};

[[nodiscard]] std::string normalizeSoundBaseId(const std::string& soundName, bool stripVariantSuffix)
{
    std::string baseId = soundName;
    const std::size_t dot = baseId.find('.');
    if (dot != std::string::npos) {
        baseId = baseId.substr(0, dot);
    }
    if (stripVariantSuffix) {
        while (!baseId.empty() && std::isdigit(static_cast<unsigned char>(baseId.back())) != 0) {
            baseId.pop_back();
        }
    }
    for (char& c : baseId) {
        if (c == '/') {
            c = '.';
        }
    }
    return baseId;
}

void registerSound(SoundRegistry& registry, const std::string& soundName, const std::filesystem::path& file)
{
    auto sound = std::make_unique<RegisteredSound>();
    sound->id = soundName;
    sound->path = file.generic_string();

    const std::string baseId = normalizeSoundBaseId(soundName, registry.pickRandomVariant);
    RegisteredSound* entry = sound.get();
    registry.byBaseId[baseId].push_back(entry);
    registry.owned.push_back(std::move(sound));
}

[[nodiscard]] const RegisteredSound* findSound(const SoundRegistry& registry, const std::string& baseId)
{
    const auto it = registry.byBaseId.find(baseId);
    if (it == registry.byBaseId.end() || it->second.empty()) {
        return nullptr;
    }
    const auto& variants = it->second;
    const int index = registry.random.nextInt(static_cast<int>(variants.size()));
    return variants[static_cast<std::size_t>(index)];
}

[[nodiscard]] const RegisteredSound* pickRandomSound(const SoundRegistry& registry)
{
    if (registry.owned.empty()) {
        return nullptr;
    }
    const int index = registry.random.nextInt(static_cast<int>(registry.owned.size()));
    return registry.owned[static_cast<std::size_t>(index)].get();
}

[[nodiscard]] bool isMusFile(const std::string& path)
{
    if (path.size() < 4) {
        return false;
    }
    const auto lower = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };
    return path[path.size() - 4] == '.'
        && lower(path[path.size() - 3]) == 'm'
        && lower(path[path.size() - 2]) == 'u'
        && lower(path[path.size() - 1]) == 's';
}

// Beta 1.7.3 .mus files are OGG data XOR-scrambled with a filename hash seed.
[[nodiscard]] int javaFilenameHash(const std::filesystem::path& path)
{
    const std::string name = path.filename().string();
    int hash = 0;
    for (const unsigned char c : name) {
        hash = 31 * hash + static_cast<int>(c);
    }
    return hash;
}

void decryptMusInPlace(std::uint8_t* data, int length, int hash)
{
    for (int i = 0; i < length; ++i) {
        const auto decrypted = static_cast<std::uint8_t>(data[i] ^ static_cast<std::uint8_t>(hash >> 8));
        data[i] = decrypted;
        hash = hash * 498729871 + 85731 * static_cast<int>(static_cast<std::int8_t>(decrypted));
    }
}

[[nodiscard]] bool loadDecryptedMus(const std::string& path, std::vector<std::uint8_t>& out)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size <= 0) {
        return false;
    }
    input.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    if (!input.read(reinterpret_cast<char*>(out.data()), size)) {
        return false;
    }
    int hash = javaFilenameHash(std::filesystem::path(path));
    decryptMusInPlace(out.data(), static_cast<int>(out.size()), hash);
    return true;
}

struct BackendDeleter {
    void operator()(AudioBackend* backend) const noexcept { audio_backend_destroy(backend); }
};

[[nodiscard]] AudioSourceParams localSoundParams()
{
    return {false, false, false, 0.0f, 0.0f, 0.0f, AUDIO_ATT_NONE, 0.0f};
}

[[nodiscard]] AudioSourceParams worldSoundParams(float x, float y, float z, float maxDistance)
{
    return {false, false, true, x, y, z, AUDIO_ATT_LINEAR, maxDistance};
}

[[nodiscard]] AudioSourceParams musicParams()
{
    return {false, false, false, 0.0f, 0.0f, 0.0f, AUDIO_ATT_NONE, 0.0f};
}

[[nodiscard]] AudioSourceParams recordParams(float x, float y, float z)
{
    return {false, true, true, x, y, z, AUDIO_ATT_LINEAR, kRecordAttenuationDistance};
}

bool loadSource(AudioBackend* backend, const char* slot, const std::string& path, AudioSourceParams params)
{
    if (isMusFile(path)) {
        std::vector<std::uint8_t> decrypted;
        if (loadDecryptedMus(path, decrypted)) {
            return audio_source_create_memory(
                backend, slot, decrypted.data(), decrypted.size(), params);
        }
    }
    return audio_source_create_file(backend, slot, path.c_str(), params);
}

[[nodiscard]] bool volumesAreZero(const option::GameOptions* options)
{
    return options != nullptr && options->soundVolume == 0.0f && options->musicVolume == 0.0f;
}

} // namespace

struct AudioEngine::Impl {
    std::unique_ptr<AudioBackend, BackendDeleter> backend;
    option::GameOptions* options = nullptr;
    bool started = false;

    SoundRegistry effects;
    SoundRegistry streaming;
    SoundRegistry music;

    std::vector<std::string> activeEffectSlots;
    int nextEffectSlot = 0;
    JavaRandom random;
    int ticksUntilMusic = 0;

    Impl() : ticksUntilMusic(random.nextInt(12000)) {}

    void ensureBackend()
    {
        if (backend) {
            return;
        }
        backend.reset(audio_backend_create());
        if (!audio_backend_ready(backend.get())) {
            std::cerr << "AudioEngine: backend unavailable; audio muted\n";
            backend.reset();
            return;
        }
        started = true;
    }

    [[nodiscard]] bool ready() const
    {
        return started && backend != nullptr && options != nullptr;
    }

    void removeFinishedEffects()
    {
        if (!backend) {
            return;
        }
        auto slot = activeEffectSlots.begin();
        while (slot != activeEffectSlots.end()) {
            if (!audio_source_playing(backend.get(), slot->c_str())) {
                audio_source_remove(backend.get(), slot->c_str());
                slot = activeEffectSlots.erase(slot);
            } else {
                ++slot;
            }
        }
    }

    std::string nextEffectSlotName()
    {
        ++nextEffectSlot;
        return "sfx_" + std::to_string(nextEffectSlot);
    }
};

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() = default;

bool AudioEngine::isReady() const
{
    return impl_->ready();
}

void AudioEngine::start(option::GameOptions* options)
{
    impl_->options = options;
    impl_->streaming.pickRandomVariant = false;
    if (!impl_->started && !volumesAreZero(options)) {
        impl_->ensureBackend();
    }
}

void AudioEngine::shutdown()
{
    if (impl_->backend) {
        audio_stop_all(impl_->backend.get());
        impl_->backend.reset();
    }
    impl_->activeEffectSlots.clear();
    impl_->started = false;
}

void AudioEngine::reset()
{
    shutdown();
    impl_->effects.byBaseId.clear();
    impl_->effects.owned.clear();
    impl_->streaming.byBaseId.clear();
    impl_->streaming.owned.clear();
    impl_->music.byBaseId.clear();
    impl_->music.owned.clear();
    impl_->nextEffectSlot = 0;
    impl_->ticksUntilMusic = impl_->random.nextInt(12000);
}

void AudioEngine::registerEffect(const std::string& id, const std::filesystem::path& file)
{
    registerSound(impl_->effects, id, file);
}

void AudioEngine::registerStreaming(const std::string& id, const std::filesystem::path& file)
{
    registerSound(impl_->streaming, id, file);
}

void AudioEngine::registerMusic(const std::string& id, const std::filesystem::path& file)
{
    registerSound(impl_->music, id, file);
}

void AudioEngine::refreshMusicVolume()
{
    if (impl_->options == nullptr) {
        return;
    }
    if (!impl_->started && !volumesAreZero(impl_->options)) {
        impl_->ensureBackend();
    }
    if (!impl_->backend) {
        return;
    }
    if (impl_->options->musicVolume == 0.0f) {
        audio_source_stop(impl_->backend.get(), kMusicSlot);
    } else {
        audio_source_set_volume(impl_->backend.get(), kMusicSlot, impl_->options->musicVolume);
    }
}

void AudioEngine::updateListener(entity::LivingEntity* player, float partialTick)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.0f || player == nullptr) {
        return;
    }

    const float yaw = player->prevYaw + (player->yaw - player->prevYaw) * partialTick;
    const double x = player->prevX + (player->x - player->prevX) * static_cast<double>(partialTick);
    const double y = player->prevY + (player->y - player->prevY) * static_cast<double>(partialTick);
    const double z = player->prevZ + (player->z - player->prevZ) * static_cast<double>(partialTick);

    const float yawRad = -yaw * (util::math::kPiF / 180.0f) - util::math::kPiF;
    const float lookX = MathHelper::sin(yawRad);
    const float lookZ = MathHelper::cos(yawRad);

    audio_set_listener_pos(
        impl_->backend.get(), static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    audio_set_listener_dir(impl_->backend.get(), -lookX, 0.0f, -lookZ, 0.0f, 1.0f, 0.0f);
}

void AudioEngine::tick()
{
    if (!impl_->ready() || impl_->options->musicVolume == 0.0f) {
        return;
    }

    impl_->removeFinishedEffects();

    if (audio_source_playing(impl_->backend.get(), kMusicSlot)
        || audio_source_playing(impl_->backend.get(), kRecordSlot)) {
        return;
    }

    if (impl_->ticksUntilMusic > 0) {
        --impl_->ticksUntilMusic;
        return;
    }

    const RegisteredSound* track = pickRandomSound(impl_->music);
    if (track == nullptr) {
        return;
    }

    impl_->ticksUntilMusic = impl_->random.nextInt(12000) + 12000;
    if (!loadSource(impl_->backend.get(), kMusicSlot, track->path, musicParams())) {
        return;
    }
    audio_source_set_volume(impl_->backend.get(), kMusicSlot, impl_->options->musicVolume);
    audio_source_play(impl_->backend.get(), kMusicSlot);
}

void AudioEngine::playAt(const std::string& id, float x, float y, float z, float volume, float pitch)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.0f || volume <= 0.0f) {
        return;
    }

    const RegisteredSound* sound = findSound(impl_->effects, id);
    if (sound == nullptr) {
        return;
    }

    impl_->removeFinishedEffects();
    const std::string slot = impl_->nextEffectSlotName();

    float maxDistance = kWorldAttenuationDistance;
    if (volume > 1.0f) {
        maxDistance *= volume;
    }

    if (!loadSource(
            impl_->backend.get(), slot.c_str(), sound->path,
            worldSoundParams(x, y, z, maxDistance))) {
        return;
    }

    const float clampedVolume = volume > 1.0f ? 1.0f : volume;
    audio_source_set_pitch(impl_->backend.get(), slot.c_str(), pitch);
    audio_source_set_volume(
        impl_->backend.get(), slot.c_str(), clampedVolume * impl_->options->soundVolume);
    audio_source_play(impl_->backend.get(), slot.c_str());
    impl_->activeEffectSlots.push_back(slot);
}

void AudioEngine::play(const std::string& id, float volume, float pitch)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.0f) {
        return;
    }

    const RegisteredSound* sound = findSound(impl_->effects, id);
    if (sound == nullptr) {
        return;
    }

    impl_->removeFinishedEffects();
    const std::string slot = impl_->nextEffectSlotName();

    if (!loadSource(impl_->backend.get(), slot.c_str(), sound->path, localSoundParams())) {
        return;
    }

    if (volume > 1.0f) {
        volume = 1.0f;
    }
    volume *= kUiSoundScale;

    audio_source_set_pitch(impl_->backend.get(), slot.c_str(), pitch);
    audio_source_set_volume(impl_->backend.get(), slot.c_str(), volume * impl_->options->soundVolume);
    audio_source_play(impl_->backend.get(), slot.c_str());
    impl_->activeEffectSlots.push_back(slot);
}

void AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.0f) {
        return;
    }

    if (audio_source_playing(impl_->backend.get(), kRecordSlot)) {
        audio_source_stop(impl_->backend.get(), kRecordSlot);
    }
    if (id.empty()) {
        return;
    }

    const RegisteredSound* sound = findSound(impl_->streaming, id);
    if (sound == nullptr || volume <= 0.0f) {
        return;
    }

    if (audio_source_playing(impl_->backend.get(), kMusicSlot)) {
        audio_source_stop(impl_->backend.get(), kMusicSlot);
    }

    if (!loadSource(
            impl_->backend.get(), kRecordSlot, sound->path, recordParams(x, y, z))) {
        return;
    }

    audio_source_set_volume(
        impl_->backend.get(), kRecordSlot, kRecordVolumeScale * impl_->options->soundVolume);
    audio_source_play(impl_->backend.get(), kRecordSlot);
}

} // namespace net::minecraft::client::platform::audio
