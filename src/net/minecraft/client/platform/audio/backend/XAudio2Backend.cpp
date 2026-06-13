#include "net/minecraft/client/platform/audio/backend/AudioBackend.hpp"

#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <xaudio2.h>
#endif

namespace net::minecraft::client::platform::audio::backend {

constexpr std::size_t kMaxSlots = 256;
constexpr char kMusicSlotName[] = "BgMusic";
constexpr char kRecordSlotName[] = "streaming";

#ifdef _WIN32
struct VoiceDeleter {
    IXAudio2* engine = nullptr;
    void operator()(IXAudio2SourceVoice* voice) const noexcept
    {
        if (voice != nullptr && engine != nullptr) {
            voice->Stop(0);
            voice->DestroyVoice();
        }
    }
};
#endif

struct SourceSlot {
    std::string name;
#ifdef _WIN32
    std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> voice;
    std::vector<XAUDIO2_BUFFER> xaudioBuffers;
#endif
    std::vector<std::uint8_t> pcmBytes;
    bool loaded = false;
    bool spatial = false;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float maxDistance = 16.0f;
    float userVolume = 1.0f;
    float pitch = 1.0f;
};

namespace {

struct XAudio2BackendImpl {
#ifdef _WIN32
    IXAudio2* engine = nullptr;
    IXAudio2MasteringVoice* masteringVoice = nullptr;
#endif
    bool ready = false;
    mutable std::mutex mutex;

    float listenerX = 0.0f;
    float listenerY = 0.0f;
    float listenerZ = 0.0f;

    std::vector<std::unique_ptr<SourceSlot>> slots;
    std::unordered_map<std::string, SourceSlot*> byName;

    [[nodiscard]] float spatialGain(const SourceSlot& slot) const
    {
        const float dx = slot.x - listenerX;
        const float dy = slot.y - listenerY;
        const float dz = slot.z - listenerZ;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (slot.maxDistance <= 0.0f) {
            return 0.0f;
        }
        const float linear = 1.0f - (distance / slot.maxDistance);
        return std::clamp(linear, 0.0f, 1.0f);
    }

    SourceSlot* findSlot(const std::string& name)
    {
        const auto it = byName.find(name);
        return it == byName.end() ? nullptr : it->second;
    }

    const SourceSlot* findSlot(const std::string& name) const
    {
        const auto it = byName.find(name);
        return it == byName.end() ? nullptr : it->second;
    }

    void clearSlot(SourceSlot& slot)
    {
#ifdef _WIN32
        if (slot.voice) {
            slot.voice->Stop(0);
            slot.voice.reset();
        }
        slot.xaudioBuffers.clear();
#endif
        slot.pcmBytes.clear();
        slot.loaded = false;
        slot.spatial = false;
        slot.userVolume = 1.0f;
        slot.pitch = 1.0f;
        if (!slot.name.empty()) {
            byName.erase(slot.name);
            slot.name.clear();
        }
    }

    SourceSlot* acquireSlot(const std::string& name)
    {
        if (SourceSlot* existing = findSlot(name)) {
            clearSlot(*existing);
            existing->name = name;
            byName[name] = existing;
            return existing;
        }

        for (auto& slot : slots) {
            if (!slot->loaded && slot->name.empty()) {
                slot->name = name;
                byName[name] = slot.get();
                return slot.get();
            }
        }

        if (slots.size() >= kMaxSlots) {
            for (auto& slot : slots) {
                if (slot->name != kMusicSlotName && slot->name != kRecordSlotName) {
                    clearSlot(*slot);
                    slot->name = name;
                    byName[name] = slot.get();
                    return slot.get();
                }
            }
            return nullptr;
        }

        auto slot = std::make_unique<SourceSlot>();
        slot->name = name;
        SourceSlot* raw = slot.get();
        byName[name] = raw;
        slots.push_back(std::move(slot));
        return raw;
    }

    bool finishSlot(SourceSlot& slot, const decode::PcmBuffer& pcm, SourceParams params)
    {
#ifdef _WIN32
        if (!ready || engine == nullptr || pcm.samples.empty() || pcm.channels == 0) {
            return false;
        }

        slot.pcmBytes.resize(pcm.samples.size() * sizeof(float));
        std::memcpy(slot.pcmBytes.data(), pcm.samples.data(), slot.pcmBytes.size());

        WAVEFORMATEX format {};
        format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format.nChannels = pcm.channels;
        format.nSamplesPerSec = pcm.sampleRate;
        format.wBitsPerSample = 32;
        format.nBlockAlign = static_cast<WORD>(pcm.channels * sizeof(float));
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

        IXAudio2SourceVoice* voice = nullptr;
        if (FAILED(engine->CreateSourceVoice(&voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr))) {
            std::cerr << "AudioBackend: CreateSourceVoice failed\n";
            clearSlot(slot);
            return false;
        }

        slot.voice.reset(voice);
        slot.voice.get_deleter().engine = engine;

        XAUDIO2_BUFFER buffer {};
        buffer.AudioBytes = static_cast<UINT32>(slot.pcmBytes.size());
        buffer.pAudioData = slot.pcmBytes.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        if (params.loop) {
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }
        slot.xaudioBuffers = {buffer};

        if (FAILED(slot.voice->SubmitSourceBuffer(&slot.xaudioBuffers[0]))) {
            clearSlot(slot);
            return false;
        }

        slot.loaded = true;
        slot.spatial = params.spatial;
        slot.x = params.x;
        slot.y = params.y;
        slot.z = params.z;
        slot.maxDistance = params.maxDistance > 0.0f ? params.maxDistance : 16.0f;
        slot.userVolume = 1.0f;
        slot.pitch = 1.0f;
        return true;
#else
        (void)slot;
        (void)pcm;
        (void)params;
        return false;
#endif
    }

    void updateSpatialVolumesLocked()
    {
#ifdef _WIN32
        for (auto& slot : slots) {
            if (!slot->loaded || !slot->spatial || !slot->voice) {
                continue;
            }
            slot->voice->SetVolume(slot->userVolume * spatialGain(*slot));
        }
#endif
    }
};

} // namespace

struct XAudio2Backend::Impl : XAudio2BackendImpl {};

XAudio2Backend::XAudio2Backend() : impl_(std::make_unique<Impl>())
{
#ifdef _WIN32
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
        std::cerr << "AudioBackend: COM init failed\n";
        return;
    }

    if (FAILED(XAudio2Create(&impl_->engine, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        std::cerr << "AudioBackend: XAudio2Create failed\n";
        CoUninitialize();
        return;
    }

    if (FAILED(impl_->engine->CreateMasteringVoice(&impl_->masteringVoice, 2, 44100))) {
        std::cerr << "AudioBackend: CreateMasteringVoice failed\n";
        impl_->engine->Release();
        impl_->engine = nullptr;
        CoUninitialize();
        return;
    }

    impl_->slots.reserve(kMaxSlots);
    impl_->ready = true;
#else
    std::cerr << "AudioBackend: XAudio2 is only implemented on Windows\n";
#endif
}

XAudio2Backend::~XAudio2Backend()
{
    stopAll();
#ifdef _WIN32
    if (impl_->masteringVoice != nullptr) {
        impl_->masteringVoice->DestroyVoice();
        impl_->masteringVoice = nullptr;
    }
    if (impl_->engine != nullptr) {
        impl_->engine->Release();
        impl_->engine = nullptr;
    }
    CoUninitialize();
#endif
}

bool XAudio2Backend::ready() const
{
    return impl_->ready;
}

void XAudio2Backend::setListener(
    float x, float y, float z,
    float /*lookX*/, float /*lookY*/, float /*lookZ*/,
    float /*upX*/, float /*upY*/, float /*upZ*/)
{
    std::lock_guard lock(impl_->mutex);
    impl_->listenerX = x;
    impl_->listenerY = y;
    impl_->listenerZ = z;
    impl_->updateSpatialVolumesLocked();
}

bool XAudio2Backend::loadSource(const std::string& name, const decode::PcmBuffer& pcm, SourceParams params)
{
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->acquireSlot(name);
    if (slot == nullptr) {
        return false;
    }
    return impl_->finishSlot(*slot, pcm, params);
}

bool XAudio2Backend::loadSourceFile(const std::string& name, const std::string& path, SourceParams params)
{
    decode::PcmBuffer pcm;
    if (!decode::decodeAudioFile(path, pcm)) {
        std::cerr << "AudioBackend: failed to decode '" << path << "'\n";
        return false;
    }
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->acquireSlot(name);
    if (slot == nullptr) {
        return false;
    }
    return impl_->finishSlot(*slot, pcm, params);
}

bool XAudio2Backend::loadSourceMemory(
    const std::string& name, const std::uint8_t* data, std::size_t size,
    const std::string& pathHint, SourceParams params)
{
    decode::PcmBuffer pcm;
    if (!decode::decodeAudioMemory(data, size, pathHint, pcm)) {
        return false;
    }
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->acquireSlot(name);
    if (slot == nullptr) {
        return false;
    }
    return impl_->finishSlot(*slot, pcm, params);
}

void XAudio2Backend::play(const std::string& name)
{
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->findSlot(name);
#ifdef _WIN32
    if (slot != nullptr && slot->voice) {
        slot->voice->Start(0);
    }
#else
    (void)slot;
#endif
}

void XAudio2Backend::stop(const std::string& name)
{
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->findSlot(name);
#ifdef _WIN32
    if (slot != nullptr && slot->voice) {
        slot->voice->Stop(0);
    }
#else
    (void)slot;
#endif
}

void XAudio2Backend::remove(const std::string& name)
{
    std::lock_guard lock(impl_->mutex);
    if (SourceSlot* slot = impl_->findSlot(name)) {
        impl_->clearSlot(*slot);
    }
}

void XAudio2Backend::setVolume(const std::string& name, float volume)
{
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->findSlot(name);
    if (slot == nullptr) {
        return;
    }
    slot->userVolume = volume;
#ifdef _WIN32
    if (slot->voice) {
        const float gain = slot->spatial ? volume * impl_->spatialGain(*slot) : volume;
        slot->voice->SetVolume(gain);
    }
#endif
}

void XAudio2Backend::setPitch(const std::string& name, float pitch)
{
    std::lock_guard lock(impl_->mutex);
    SourceSlot* slot = impl_->findSlot(name);
    if (slot == nullptr) {
        return;
    }
    slot->pitch = pitch;
#ifdef _WIN32
    if (slot->voice) {
        slot->voice->SetFrequencyRatio(std::clamp(pitch, 0.01f, 100.0f));
    }
#endif
}

bool XAudio2Backend::playing(const std::string& name) const
{
    std::lock_guard lock(impl_->mutex);
    const SourceSlot* slot = impl_->findSlot(name);
#ifdef _WIN32
    if (slot == nullptr || !slot->voice) {
        return false;
    }
    XAUDIO2_VOICE_STATE state {};
    slot->voice->GetState(&state);
    return state.BuffersQueued > 0;
#else
    (void)slot;
    return false;
#endif
}

void XAudio2Backend::stopAll()
{
    std::lock_guard lock(impl_->mutex);
    for (auto& slot : impl_->slots) {
        if (slot->loaded || !slot->name.empty()) {
            impl_->clearSlot(*slot);
        }
    }
}

void XAudio2Backend::updateSpatialVolumes()
{
    std::lock_guard lock(impl_->mutex);
    impl_->updateSpatialVolumesLocked();
}

std::unique_ptr<AudioBackend> AudioBackend::create()
{
    auto backend = std::make_unique<XAudio2Backend>();
    if (!backend->ready()) {
        return nullptr;
    }
    return backend;
}

} // namespace net::minecraft::client::platform::audio::backend
