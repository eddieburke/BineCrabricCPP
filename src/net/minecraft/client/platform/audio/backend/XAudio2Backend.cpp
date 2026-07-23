#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/platform/audio/backend/AudioBackend.hpp"
#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <xaudio2.h>
#endif
namespace net::minecraft::client::platform::audio::backend {
namespace {
constexpr std::size_t kMaxSlots = 256;
constexpr char kMusicSlot[] = "BgMusic";
constexpr char kRecordSlot[] = "streaming";
#ifdef _WIN32
struct VoiceDeleter {
 IXAudio2* engine = nullptr;
 void operator()(IXAudio2SourceVoice* voice) const noexcept {
  if(voice != nullptr && engine != nullptr) {
   voice->Stop(0);
   voice->DestroyVoice();
  }
 }
};
struct SourceSlot {
 std::string name;
 std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> voice;
 std::vector<std::uint8_t> pcmBytes;
 XAUDIO2_BUFFER buffer{};
 bool loaded = false;
 bool spatial = false;
 std::uint16_t inputChannels = 0;
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 float maxDistance = 16.0f;
 float userVolume = 1.0f;
};
[[nodiscard]] float listenerPan(
    float listenerX, float listenerZ, float lookX, float lookZ, float sourceX, float sourceZ) {
 const float dx = sourceX - listenerX;
 const float dz = sourceZ - listenerZ;
 const float lenSq = dx * dx + dz * dz;
 if(lenSq <= 1.0e-8f) {
  return 0.0f;
 }
 float lx = lookX;
 float lz = lookZ;
 const float lookLenSq = lx * lx + lz * lz;
 if(lookLenSq > 1.0e-8f) {
  const float inv = 1.0f / std::sqrt(lookLenSq);
  lx *= inv;
  lz *= inv;
 } else {
  lx = 0.0f;
  lz = 1.0f;
 }
 return std::clamp((dx * -lz + dz * lx) / std::sqrt(lenSq), -1.0f, 1.0f);
}
#endif
struct XAudio2BackendImpl {
#ifdef _WIN32
 IXAudio2* engine = nullptr;
 IXAudio2MasteringVoice* masteringVoice = nullptr;
#endif
 bool ready = false;
 float listenerX = 0.0f;
 float listenerY = 0.0f;
 float listenerZ = 0.0f;
 float listenerLookX = 0.0f;
 float listenerLookZ = 1.0f;
 std::vector<std::unique_ptr<SourceSlot>> slots;
 std::unordered_map<std::string, SourceSlot*> byName;
 SourceSlot* findSlot(const std::string& name) {
  const auto it = byName.find(name);
  return it == byName.end() ? nullptr : it->second;
 }
 [[nodiscard]] float spatialGain(const SourceSlot& slot) const {
  const float dx = slot.x - listenerX;
  const float dy = slot.y - listenerY;
  const float dz = slot.z - listenerZ;
  const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
  if(slot.maxDistance <= 0.0f) {
   return 0.0f;
  }
  return std::clamp(1.0f - distance / slot.maxDistance, 0.0f, 1.0f);
 }
 void applySpatial(SourceSlot& slot) {
#ifdef _WIN32
  if(!slot.loaded || !slot.spatial || slot.voice == nullptr || masteringVoice == nullptr ||
     slot.inputChannels == 0) {
   return;
  }
  const float pan = listenerPan(listenerX, listenerZ, listenerLookX, listenerLookZ, slot.x, slot.z);
  const float left = std::clamp(1.0f - pan, 0.0f, 1.0f);
  const float right = std::clamp(1.0f + pan, 0.0f, 1.0f);
  if(slot.inputChannels == 1) {
   const std::array<float, 2> matrix{left, right};
   slot.voice->SetOutputMatrix(masteringVoice, 1, 2, matrix.data());
  } else {
   const std::array<float, 4> matrix{left * 0.5f, right * 0.5f, left * 0.5f, right * 0.5f};
   slot.voice->SetOutputMatrix(masteringVoice, 2, 2, matrix.data());
  }
  slot.voice->SetVolume(slot.userVolume * spatialGain(slot));
#endif
 }
 [[nodiscard]] bool slotPlaying(const SourceSlot& slot) const {
#ifdef _WIN32
  if(slot.voice == nullptr) {
   return false;
  }
  XAUDIO2_VOICE_STATE state{};
  slot.voice->GetState(&state);
  return state.BuffersQueued > 0;
#else
  (void)slot;
  return false;
#endif
 }
 void clearSlot(SourceSlot& slot) {
#ifdef _WIN32
  slot.voice.reset();
  slot.buffer = {};
#endif
  slot.pcmBytes.clear();
  slot.loaded = false;
  slot.spatial = false;
  slot.inputChannels = 0;
  slot.userVolume = 1.0f;
  if(!slot.name.empty()) {
   byName.erase(slot.name);
   slot.name.clear();
  }
 }
 SourceSlot* acquireSlot(const std::string& name) {
  if(SourceSlot* existing = findSlot(name)) {
   clearSlot(*existing);
   existing->name = name;
   byName[name] = existing;
   return existing;
  }
  for(auto& slot : slots) {
   if(!slot->loaded && slot->name.empty()) {
    slot->name = name;
    byName[name] = slot.get();
    return slot.get();
   }
  }
  if(slots.size() >= kMaxSlots) {
   for(auto& slot : slots) {
    if(slot->name != kMusicSlot && slot->name != kRecordSlot && !slotPlaying(*slot)) {
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
 bool finishSlot(SourceSlot& slot, const decode::PcmBuffer& pcm, SourceParams params) {
#ifdef _WIN32
  if(!ready || engine == nullptr || pcm.samples.empty() || pcm.channels == 0) {
   return false;
  }
  slot.pcmBytes.resize(pcm.samples.size() * sizeof(float));
  std::memcpy(slot.pcmBytes.data(), pcm.samples.data(), slot.pcmBytes.size());
  WAVEFORMATEX format{};
  format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
  format.nChannels = pcm.channels;
  format.nSamplesPerSec = pcm.sampleRate;
  format.wBitsPerSample = 32;
  format.nBlockAlign = static_cast<WORD>(pcm.channels * sizeof(float));
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  IXAudio2SourceVoice* voice = nullptr;
  if(FAILED(
         engine->CreateSourceVoice(&voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr))) {
   clearSlot(slot);
   return false;
  }
  slot.voice.reset(voice);
  slot.voice.get_deleter().engine = engine;
  slot.buffer = {};
  slot.buffer.AudioBytes = static_cast<UINT32>(slot.pcmBytes.size());
  slot.buffer.pAudioData = slot.pcmBytes.data();
  slot.buffer.Flags = XAUDIO2_END_OF_STREAM;
  if(params.loop) {
   slot.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
  }
  if(FAILED(slot.voice->SubmitSourceBuffer(&slot.buffer))) {
   clearSlot(slot);
   return false;
  }
  slot.loaded = true;
  slot.spatial = params.spatial;
  slot.inputChannels = pcm.channels;
  slot.x = params.x;
  slot.y = params.y;
  slot.z = params.z;
  slot.maxDistance = params.maxDistance > 0.0f ? params.maxDistance : 16.0f;
  slot.userVolume = 1.0f;
  applySpatial(slot);
  return true;
#else
  (void)slot;
  (void)pcm;
  (void)params;
  return false;
#endif
 }
 void refreshSpatial() {
#ifdef _WIN32
  for(auto& slot : slots) {
   if(slot->loaded && slot->spatial && slot->voice != nullptr) {
    applySpatial(*slot);
   }
  }
#endif
 }
};
} // namespace
struct XAudio2Backend::Impl : XAudio2BackendImpl {};
XAudio2Backend::XAudio2Backend() : impl_(std::make_unique<Impl>()) {
#ifdef _WIN32
 if(FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
  return;
 }
 if(FAILED(XAudio2Create(&impl_->engine, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
  CoUninitialize();
  return;
 }
 if(FAILED(impl_->engine->CreateMasteringVoice(&impl_->masteringVoice, 2, 44100))) {
  impl_->engine->Release();
  impl_->engine = nullptr;
  CoUninitialize();
  return;
 }
 impl_->slots.reserve(kMaxSlots);
 impl_->ready = true;
#endif
}
XAudio2Backend::~XAudio2Backend() {
 stopAll();
#ifdef _WIN32
 if(impl_->masteringVoice != nullptr) {
  impl_->masteringVoice->DestroyVoice();
 }
 if(impl_->engine != nullptr) {
  impl_->engine->Release();
 }
 CoUninitialize();
#endif
}
bool XAudio2Backend::ready() const {
 return impl_->ready;
}
void XAudio2Backend::setListener(
    float x, float y, float z, float lookX, float /*lookY*/, float lookZ, float /*upX*/, float /*upY*/, float /*upZ*/) {
 impl_->listenerX = x;
 impl_->listenerY = y;
 impl_->listenerZ = z;
 impl_->listenerLookX = lookX;
 impl_->listenerLookZ = lookZ;
 impl_->refreshSpatial();
}
bool XAudio2Backend::loadSourceFile(const std::string& name, const std::string& path, SourceParams params) {
 decode::PcmBuffer pcm;
 if(!decode::decodeAudioFile(path, pcm)) {
  return false;
 }
 SourceSlot* slot = impl_->acquireSlot(name);
 return slot != nullptr && impl_->finishSlot(*slot, pcm, params);
}
void XAudio2Backend::play(const std::string& name) {
 if(SourceSlot* slot = impl_->findSlot(name)) {
#ifdef _WIN32
  if(slot->voice != nullptr) {
   slot->voice->Start(0);
  }
#else
  (void)slot;
#endif
 }
}
void XAudio2Backend::stop(const std::string& name) {
 if(SourceSlot* slot = impl_->findSlot(name)) {
#ifdef _WIN32
  if(slot->voice != nullptr) {
   slot->voice->Stop(0);
  }
#endif
 }
}
void XAudio2Backend::setVolume(const std::string& name, float volume) {
 SourceSlot* slot = impl_->findSlot(name);
 if(slot == nullptr) {
  return;
 }
 slot->userVolume = volume;
#ifdef _WIN32
 if(slot->voice != nullptr) {
  if(slot->spatial) {
   impl_->applySpatial(*slot);
  } else {
   slot->voice->SetVolume(volume);
  }
 }
#endif
}
void XAudio2Backend::setPitch(const std::string& name, float pitch) {
#ifdef _WIN32
 if(SourceSlot* slot = impl_->findSlot(name)) {
  if(slot->voice != nullptr) {
   slot->voice->SetFrequencyRatio(std::clamp(pitch, 0.01f, 100.0f));
  }
 }
#else
 (void)name;
 (void)pitch;
#endif
}
bool XAudio2Backend::playing(const std::string& name) const {
 const SourceSlot* slot = impl_->findSlot(name);
 return slot != nullptr && impl_->slotPlaying(*slot);
}
void XAudio2Backend::stopAll() {
 for(auto& slot : impl_->slots) {
  if(slot->loaded || !slot->name.empty()) {
   impl_->clearSlot(*slot);
  }
 }
}
} // namespace net::minecraft::client::platform::audio::backend
