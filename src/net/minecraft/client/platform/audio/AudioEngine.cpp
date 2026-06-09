#include "net/minecraft/client/platform/audio/AudioEngine.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/platform/audio/backend/audio_backend.h"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <algorithm>
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

constexpr const char* kMusicSlot  = "BgMusic";
constexpr const char* kRecordSlot = "streaming";

constexpr float kUiVolumeScale         = 0.25f;
constexpr float kWorldAttenuationDist  = 16.0f;
constexpr float kRecordAttenuationDist = 64.0f;
constexpr float kRecordVolumeScale     = 0.5f;
constexpr int   kMusicCooldownBase     = 12000;

// ---- MUS decrypt (Beta 1.7.3 XOR-scrambled OGG) --------------------------------

int javaFilenameHash(const std::filesystem::path& path)
{
    int hash = 0;
    for (const unsigned char c : path.filename().string())
        hash = 31 * hash + static_cast<int>(c);
    return hash;
}

void decryptMusInPlace(std::uint8_t* data, int len, int hash)
{
    for (int i = 0; i < len; ++i) {
        const auto dec = static_cast<std::uint8_t>(data[i] ^ static_cast<std::uint8_t>(hash >> 8));
        data[i] = dec;
        hash = hash * 498729871 + 85731 * static_cast<int>(static_cast<std::int8_t>(dec));
    }
}

bool loadDecryptedMus(const std::string& path, std::vector<std::uint8_t>& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    const auto size = static_cast<std::streamoff>(f.tellg());
    if (size <= 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    if (!f.read(reinterpret_cast<char*>(out.data()), size)) return false;
    decryptMusInPlace(out.data(), static_cast<int>(out.size()), javaFilenameHash(path));
    return true;
}

bool isMusFile(const std::string& path)
{
    if (path.size() < 4) return false;
    const auto lo = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };
    const auto e = path.size();
    return path[e-4] == '.' && lo(path[e-3]) == 'm' && lo(path[e-2]) == 'u' && lo(path[e-1]) == 's';
}

// ---- Backend helpers -----------------------------------------------------------

// .mus files are decrypted in memory and handed to the backend as raw OGG bytes;
// everything else is loaded by path (the backend decodes OGG/WAV itself).
bool loadSource(AudioBackend* b, const char* slot, const std::string& path, AudioSourceParams p)
{
    if (isMusFile(path)) {
        std::vector<std::uint8_t> buf;
        if (loadDecryptedMus(path, buf))
            return audio_source_create_memory(b, slot, buf.data(), buf.size(), p);
    }
    return audio_source_create_file(b, slot, path.c_str(), p);
}

AudioSourceParams localParams()
{
    return {false, false, false, 0.f, 0.f, 0.f, AUDIO_ATT_NONE, 0.f};
}

AudioSourceParams worldParams(float x, float y, float z, float dist)
{
    return {false, false, true, x, y, z, AUDIO_ATT_LINEAR, dist};
}

AudioSourceParams recordParams(float x, float y, float z)
{
    return {false, true, true, x, y, z, AUDIO_ATT_LINEAR, kRecordAttenuationDist};
}

// Faithful to paulscode Source.setPitch — OpenAL clamps pitch to [0.5, 2.0].
float clampPitch(float p)
{
    return p < 0.5f ? 0.5f : (p > 2.0f ? 2.0f : p);
}

// ---- Sound registry (id -> path variants) --------------------------------------

// Strips a trailing variant index and the dot-suffix so "step1.ogg", "step2.ogg"
// both resolve to the base id "step". '/' becomes '.' to match Java sound names.
std::string baseId(const std::string& id)
{
    std::string s = id;
    const auto dot = s.find('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    while (!s.empty() && std::isdigit(static_cast<unsigned char>(s.back())))
        s.pop_back();
    for (char& c : s) if (c == '/') c = '.';
    return s;
}

struct Registry {
    std::unordered_map<std::string, std::vector<std::string>> paths; // base id -> variant paths
    bool stripVariants = true;

    void add(const std::string& id, const std::filesystem::path& file)
    {
        paths[stripVariants ? baseId(id) : id].push_back(file.generic_string());
    }

    // Random variant of one id (e.g. a footstep), or null if unknown.
    const std::string* pick(const std::string& id, JavaRandom& rng) const
    {
        const auto it = paths.find(stripVariants ? baseId(id) : id);
        if (it == paths.end() || it->second.empty()) return nullptr;
        return &it->second[static_cast<std::size_t>(rng.nextInt(static_cast<int>(it->second.size())))];
    }

    // Uniformly random track across every registered file (used for music).
    const std::string* pickAny(JavaRandom& rng) const
    {
        std::size_t total = 0;
        for (const auto& [key, variants] : paths) total += variants.size();
        if (total == 0) return nullptr;
        auto idx = static_cast<std::size_t>(rng.nextInt(static_cast<int>(total)));
        for (const auto& [key, variants] : paths) {
            if (idx < variants.size()) return &variants[idx];
            idx -= variants.size();
        }
        return nullptr;
    }

    void clear() { paths.clear(); }
};

} // namespace

// ---- Impl ----------------------------------------------------------------------

struct AudioEngine::Impl {
    AudioBackend* backend = nullptr;
    option::GameOptions* options = nullptr;
    bool started = false;

    Registry effects;
    Registry streaming;
    Registry music;

    std::vector<std::string> activeSlots;
    int nextSlot = 0;
    JavaRandom rng;
    int ticksUntilMusic;

    Impl() : ticksUntilMusic(rng.nextInt(kMusicCooldownBase)) {}
    ~Impl() { destroyBackend(); }

    void ensureBackend()
    {
        if (backend) return;
        backend = audio_backend_create();
        if (!audio_backend_ready(backend)) {
            std::cerr << "AudioEngine: backend unavailable; audio muted\n";
            audio_backend_destroy(backend);
            backend = nullptr;
            return;
        }
        started = true;
    }

    void destroyBackend()
    {
        if (backend) {
            audio_backend_destroy(backend);
            backend = nullptr;
        }
        started = false;
    }

    bool ready() const { return started && backend && options; }

    // Round-robins 256 spatial voice slots (faithful to SoundManager's suffix scheme).
    std::string nextSlotName()
    {
        nextSlot = (nextSlot + 1) % 256;
        return "sound_" + std::to_string(nextSlot);
    }

    void markActive(const std::string& slot)
    {
        activeSlots.erase(std::remove(activeSlots.begin(), activeSlots.end(), slot), activeSlots.end());
        activeSlots.push_back(slot);
    }

    void reapFinished()
    {
        if (!backend) return;
        for (auto it = activeSlots.begin(); it != activeSlots.end(); ) {
            if (!audio_source_playing(backend, it->c_str())) {
                audio_source_remove(backend, it->c_str());
                it = activeSlots.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// ---- Public API ----------------------------------------------------------------

AudioEngine::AudioEngine()  : impl_(std::make_unique<Impl>()) {}
AudioEngine::~AudioEngine() = default;

bool AudioEngine::isReady() const { return impl_->ready(); }

void AudioEngine::start(option::GameOptions* options)
{
    impl_->options = options;
    impl_->streaming.stripVariants = false; // records are looked up by exact id
    if (!impl_->started && options && (options->soundVolume > 0.f || options->musicVolume > 0.f))
        impl_->ensureBackend();
}

void AudioEngine::shutdown()
{
    if (impl_->backend)
        audio_stop_all(impl_->backend);
    impl_->destroyBackend();
    impl_->activeSlots.clear();
}

void AudioEngine::reset()
{
    shutdown();
    impl_->effects.clear();
    impl_->streaming.clear();
    impl_->music.clear();
    impl_->nextSlot = 0;
    impl_->ticksUntilMusic = impl_->rng.nextInt(kMusicCooldownBase);
}

void AudioEngine::registerEffect(const std::string& id, const std::filesystem::path& file)
{
    impl_->effects.add(id, file);
}

void AudioEngine::registerStreaming(const std::string& id, const std::filesystem::path& file)
{
    impl_->streaming.add(id, file);
}

void AudioEngine::registerMusic(const std::string& id, const std::filesystem::path& file)
{
    impl_->music.add(id, file);
}

void AudioEngine::refreshMusicVolume()
{
    if (!impl_->options) return;
    if (!impl_->started && (impl_->options->soundVolume > 0.f || impl_->options->musicVolume > 0.f))
        impl_->ensureBackend();
    if (!impl_->backend) return;

    if (impl_->options->musicVolume == 0.f)
        audio_source_stop(impl_->backend, kMusicSlot);
    else
        audio_source_set_volume(impl_->backend, kMusicSlot, impl_->options->musicVolume);
}

void AudioEngine::updateListener(entity::LivingEntity* player, float partialTick)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.f || !player) return;

    const float yaw    = player->prevYaw + (player->yaw - player->prevYaw) * partialTick;
    const float x      = static_cast<float>(player->prevX + (player->x - player->prevX) * partialTick);
    const float y      = static_cast<float>(player->prevY + (player->y - player->prevY) * partialTick);
    const float z      = static_cast<float>(player->prevZ + (player->z - player->prevZ) * partialTick);
    const float yawRad = -yaw * (util::math::kPiF / 180.f) - util::math::kPiF;

    audio_set_listener_pos(impl_->backend, x, y, z);
    audio_set_listener_dir(impl_->backend,
        -MathHelper::sin(yawRad), 0.f, -MathHelper::cos(yawRad),
        0.f, 1.f, 0.f);
}

void AudioEngine::tick()
{
    if (!impl_->ready() || impl_->options->musicVolume == 0.f) return;

    impl_->reapFinished();

    if (audio_source_playing(impl_->backend, kMusicSlot) ||
        audio_source_playing(impl_->backend, kRecordSlot))
        return;

    if (impl_->ticksUntilMusic > 0) { --impl_->ticksUntilMusic; return; }

    const std::string* track = impl_->music.pickAny(impl_->rng);
    if (!track) return;

    impl_->ticksUntilMusic = impl_->rng.nextInt(kMusicCooldownBase) + kMusicCooldownBase;
    if (!loadSource(impl_->backend, kMusicSlot, *track, localParams())) return;
    audio_source_set_volume(impl_->backend, kMusicSlot, impl_->options->musicVolume);
    audio_source_play(impl_->backend, kMusicSlot);
}

void AudioEngine::playAt(const std::string& id, float x, float y, float z, float volume, float pitch)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.f || volume <= 0.f) return;

    const std::string* path = impl_->effects.pick(id, impl_->rng);
    if (!path) return;

    impl_->reapFinished();
    const std::string slot = impl_->nextSlotName();
    const float dist = kWorldAttenuationDist * (volume > 1.f ? volume : 1.f);

    if (!loadSource(impl_->backend, slot.c_str(), *path, worldParams(x, y, z, dist))) return;
    audio_source_set_pitch(impl_->backend, slot.c_str(), clampPitch(pitch));
    audio_source_set_volume(impl_->backend, slot.c_str(),
        (volume > 1.f ? 1.f : volume) * impl_->options->soundVolume);
    audio_source_play(impl_->backend, slot.c_str());
    impl_->markActive(slot);
}

void AudioEngine::play(const std::string& id, float volume, float pitch)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.f) return;

    const std::string* path = impl_->effects.pick(id, impl_->rng);
    if (!path) return;

    impl_->reapFinished();
    const std::string slot = impl_->nextSlotName();

    if (!loadSource(impl_->backend, slot.c_str(), *path, localParams())) return;
    audio_source_set_pitch(impl_->backend, slot.c_str(), clampPitch(pitch));
    audio_source_set_volume(impl_->backend, slot.c_str(),
        (volume > 1.f ? 1.f : volume) * kUiVolumeScale * impl_->options->soundVolume);
    audio_source_play(impl_->backend, slot.c_str());
    impl_->markActive(slot);
}

void AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume)
{
    if (!impl_->ready() || impl_->options->soundVolume == 0.f) return;

    audio_source_stop(impl_->backend, kRecordSlot);
    if (id.empty()) return;

    const std::string* path = impl_->streaming.pick(id, impl_->rng);
    if (!path || volume <= 0.f) return;

    if (audio_source_playing(impl_->backend, kMusicSlot))
        audio_source_stop(impl_->backend, kMusicSlot);

    if (!loadSource(impl_->backend, kRecordSlot, *path, recordParams(x, y, z))) return;
    audio_source_set_volume(impl_->backend, kRecordSlot,
        kRecordVolumeScale * impl_->options->soundVolume);
    audio_source_play(impl_->backend, kRecordSlot);
}

} // namespace net::minecraft::client::platform::audio
