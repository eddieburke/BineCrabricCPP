#include "net/minecraft/client/platform/audio/AudioEngine.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/platform/audio/backend/AudioBackend.hpp"
#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
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

[[nodiscard]] backend::SourceParams localSoundParams()
{
    return {false, false, 0.0f, 0.0f, 0.0f, 0.0f};
}

[[nodiscard]] backend::SourceParams worldSoundParams(float x, float y, float z, float maxDistance)
{
    return {false, true, x, y, z, maxDistance};
}

[[nodiscard]] backend::SourceParams musicParams()
{
    return {false, false, 0.0f, 0.0f, 0.0f, 0.0f};
}

[[nodiscard]] backend::SourceParams recordParams(float x, float y, float z)
{
    return {false, true, x, y, z, kRecordAttenuationDistance};
}

bool loadSource(backend::AudioBackend* backend, const char* slot, const std::string& path, backend::SourceParams params)
{
    return backend->loadSourceFile(slot, path, params);
}

[[nodiscard]] bool volumesAreZero(const option::GameOptions* options)
{
    return options != nullptr && options->soundVolume == 0.0f && options->musicVolume == 0.0f;
}

} // namespace

struct AudioEngine::Impl {
    std::unique_ptr<backend::AudioBackend> backend;
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
        backend = backend::AudioBackend::create();
        if (!backend || !backend->ready()) {
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
            if (!backend->playing(*slot)) {
                backend->remove(*slot);
                slot = activeEffectSlots.erase(slot);
            } else {
                ++slot;
            }
        }
    }

    std::string nextEffectSlotName()
    {
        nextEffectSlot = (nextEffectSlot + 1) % 256;
        return "sound_" + std::to_string(nextEffectSlot);
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
        impl_->backend->stopAll();
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
        impl_->backend->stop(kMusicSlot);
    } else {
        impl_->backend->setVolume(kMusicSlot, impl_->options->musicVolume);
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

    impl_->backend->setListener(
        static_cast<float>(x), static_cast<float>(y), static_cast<float>(z),
        -lookX, 0.0f, -lookZ, 0.0f, 1.0f, 0.0f);
}

void AudioEngine::tick()
{
    if (!impl_->ready() || impl_->options->musicVolume == 0.0f) {
        return;
    }

    impl_->removeFinishedEffects();

    if (impl_->backend->playing(kMusicSlot) || impl_->backend->playing(kRecordSlot)) {
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
    impl_->backend->setVolume(kMusicSlot, impl_->options->musicVolume);
    impl_->backend->play(kMusicSlot);
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
    impl_->backend->setPitch(slot, pitch);
    impl_->backend->setVolume(slot, clampedVolume * impl_->options->soundVolume);
    impl_->backend->play(slot);
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

    impl_->backend->setPitch(slot, pitch);
    impl_->backend->setVolume(slot, volume * impl_->options->soundVolume);
    impl_->backend->play(slot);
    impl_->activeEffectSlots.push_back(slot);
}

void AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.0f) {
        return;
    }

    if (impl_->backend->playing(kRecordSlot)) {
        impl_->backend->stop(kRecordSlot);
    }
    if (id.empty()) {
        return;
    }

    const RegisteredSound* sound = findSound(impl_->streaming, id);
    if (sound == nullptr || volume <= 0.0f) {
        return;
    }

    if (impl_->backend->playing(kMusicSlot)) {
        impl_->backend->stop(kMusicSlot);
    }

    if (!loadSource(
            impl_->backend.get(), kRecordSlot, sound->path, recordParams(x, y, z))) {
        return;
    }

    impl_->backend->setVolume(
        kRecordSlot, kRecordVolumeScale * impl_->options->soundVolume);
    impl_->backend->play(kRecordSlot);
}

} // namespace net::minecraft::client::platform::audio
