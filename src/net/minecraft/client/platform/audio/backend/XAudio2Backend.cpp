#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
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
using Clock = std::chrono::steady_clock;
constexpr std::size_t kMaxSlots = 256;
constexpr std::size_t kMaxQueuedPlaybacks = 512;
constexpr std::size_t kMaxCachedPcmBytes = 64 * 1024 * 1024;
constexpr auto kTransientLifetime = std::chrono::milliseconds(250);
constexpr auto kPlaybackSweepInterval = std::chrono::milliseconds(10);
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
 std::uint64_t generation = 0;
 std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> voice;
 std::shared_ptr<const decode::PcmBuffer> pcm;
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
struct DecodeCache {
 struct Entry {
  std::shared_ptr<const decode::PcmBuffer> pcm;
  std::size_t bytes = 0;
  std::uint64_t stamp = 0;
 };
 std::mutex mutex;
 std::unordered_map<std::string, Entry> entries;
 std::size_t bytes = 0;
 std::uint64_t clock = 0;
 [[nodiscard]] std::shared_ptr<const decode::PcmBuffer> decodeFile(const std::string& path) {
  {
   const std::scoped_lock lock(mutex);
   const auto cached = entries.find(path);
   if(cached != entries.end()) {
    cached->second.stamp = ++clock;
    return cached->second.pcm;
   }
  }
  auto pcm = std::make_shared<decode::PcmBuffer>();
  if(!decode::decodeAudioFile(path, *pcm)) {
   return {};
  }
  const std::size_t pcmBytes = pcm->samples.size() * sizeof(float);
  if(pcmBytes > kMaxCachedPcmBytes) {
   return pcm;
  }
  const std::scoped_lock lock(mutex);
  const auto existing = entries.find(path);
  if(existing != entries.end()) {
   existing->second.stamp = ++clock;
   return existing->second.pcm;
  }
  while(bytes + pcmBytes > kMaxCachedPcmBytes) {
   auto victim = entries.end();
   for(auto it = entries.begin(); it != entries.end(); ++it) {
    if(it->second.pcm.use_count() == 1 &&
       (victim == entries.end() || it->second.stamp < victim->second.stamp)) {
     victim = it;
    }
   }
   if(victim == entries.end()) {
    break;
   }
   bytes -= victim->second.bytes;
   entries.erase(victim);
  }
  if(bytes + pcmBytes <= kMaxCachedPcmBytes) {
   bytes += pcmBytes;
   entries.emplace(path, Entry{pcm, pcmBytes, ++clock});
  }
  return pcm;
 }
};
struct BackendCore {
#ifdef _WIN32
 IXAudio2* engine = nullptr;
 IXAudio2MasteringVoice* masteringVoice = nullptr;
 bool comInitialized = false;
 std::vector<std::unique_ptr<SourceSlot>> slots;
 std::unordered_map<std::string, SourceSlot*> byName;
#endif
 float listenerX = 0.0f;
 float listenerY = 0.0f;
 float listenerZ = 0.0f;
 float listenerLookX = 0.0f;
 float listenerLookZ = 1.0f;
 [[nodiscard]] bool initialize() {
#ifdef _WIN32
  if(FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
   return false;
  }
  comInitialized = true;
  if(FAILED(XAudio2Create(&engine, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
   return false;
  }
  if(FAILED(engine->CreateMasteringVoice(&masteringVoice, 2, 44100))) {
   return false;
  }
  slots.reserve(kMaxSlots);
  return true;
#else
  return false;
#endif
 }
 void shutdown() {
  stopAll();
#ifdef _WIN32
  if(masteringVoice != nullptr) {
   masteringVoice->DestroyVoice();
   masteringVoice = nullptr;
  }
  if(engine != nullptr) {
   engine->Release();
   engine = nullptr;
  }
  if(comInitialized) {
   CoUninitialize();
   comInitialized = false;
  }
#endif
 }
#ifdef _WIN32
 [[nodiscard]] SourceSlot* findSlot(const std::string& name) {
  const auto it = byName.find(name);
  return it == byName.end() ? nullptr : it->second;
 }
 [[nodiscard]] const SourceSlot* findSlot(const std::string& name) const {
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
 }
 [[nodiscard]] bool slotPlaying(const SourceSlot& slot) const {
  if(slot.voice == nullptr) {
   return false;
  }
  XAUDIO2_VOICE_STATE state{};
  slot.voice->GetState(&state);
  return state.BuffersQueued > 0;
 }
 void clearSlot(SourceSlot& slot) {
  slot.voice.reset();
  slot.buffer = {};
  slot.pcm.reset();
  slot.loaded = false;
  slot.spatial = false;
  slot.inputChannels = 0;
  slot.userVolume = 1.0f;
  slot.generation = 0;
  if(!slot.name.empty()) {
   byName.erase(slot.name);
   slot.name.clear();
  }
 }
 [[nodiscard]] SourceSlot* acquireSlot(const std::string& name, std::uint64_t generation) {
  if(SourceSlot* existing = findSlot(name)) {
   clearSlot(*existing);
   existing->name = name;
   existing->generation = generation;
   byName[name] = existing;
   return existing;
  }
  for(auto& slot : slots) {
   if(!slot->loaded && slot->name.empty()) {
    slot->name = name;
    slot->generation = generation;
    byName[name] = slot.get();
    return slot.get();
   }
  }
  if(slots.size() >= kMaxSlots) {
   for(auto& slot : slots) {
    if(slot->name != kMusicSlot && slot->name != kRecordSlot && !slotPlaying(*slot)) {
     clearSlot(*slot);
     slot->name = name;
     slot->generation = generation;
     byName[name] = slot.get();
     return slot.get();
    }
   }
   return nullptr;
  }
  auto slot = std::make_unique<SourceSlot>();
  slot->name = name;
  slot->generation = generation;
  SourceSlot* raw = slot.get();
  byName[name] = raw;
  slots.push_back(std::move(slot));
  return raw;
 }
#endif
 [[nodiscard]] bool playDecoded(const std::string& name,
                                const std::shared_ptr<const decode::PcmBuffer>& pcm,
                                SourceParams params,
                                float volume,
                                float pitch,
                                std::uint64_t generation,
                                Clock::time_point expiresAt) {
  if(!pcm || Clock::now() > expiresAt) {
   return false;
  }
#ifdef _WIN32
  if(engine == nullptr || pcm->samples.empty() || pcm->channels == 0) {
   return false;
  }
  const std::size_t audioBytes = pcm->samples.size() * sizeof(float);
  if(audioBytes > static_cast<std::size_t>(std::numeric_limits<UINT32>::max())) {
   return false;
  }
  SourceSlot* slot = acquireSlot(name, generation);
  if(slot == nullptr) {
   return false;
  }
  slot->pcm = pcm;
  WAVEFORMATEX format{};
  format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
  format.nChannels = pcm->channels;
  format.nSamplesPerSec = pcm->sampleRate;
  format.wBitsPerSample = 32;
  format.nBlockAlign = static_cast<WORD>(pcm->channels * sizeof(float));
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  IXAudio2SourceVoice* voice = nullptr;
  if(FAILED(engine->CreateSourceVoice(&voice, &format, 0, XAUDIO2_MAX_FREQ_RATIO, nullptr, nullptr, nullptr))) {
   clearSlot(*slot);
   return false;
  }
  slot->voice.reset(voice);
  slot->voice.get_deleter().engine = engine;
  slot->buffer = {};
  slot->buffer.AudioBytes = static_cast<UINT32>(audioBytes);
  slot->buffer.pAudioData = reinterpret_cast<const BYTE*>(slot->pcm->samples.data());
  slot->buffer.Flags = XAUDIO2_END_OF_STREAM;
  if(params.loop) {
   slot->buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
  }
  if(FAILED(slot->voice->SubmitSourceBuffer(&slot->buffer))) {
   clearSlot(*slot);
   return false;
  }
  slot->loaded = true;
  slot->spatial = params.spatial;
  slot->inputChannels = pcm->channels;
  slot->x = params.x;
  slot->y = params.y;
  slot->z = params.z;
  slot->maxDistance = params.maxDistance > 0.0f ? params.maxDistance : 16.0f;
  slot->userVolume = volume;
  if(FAILED(slot->voice->SetFrequencyRatio(std::clamp(pitch, 0.01f, 100.0f)))) {
   clearSlot(*slot);
   return false;
  }
  if(slot->spatial) {
   applySpatial(*slot);
  } else if(FAILED(slot->voice->SetVolume(volume))) {
   clearSlot(*slot);
   return false;
  }
  if(FAILED(slot->voice->Start(0))) {
   clearSlot(*slot);
   return false;
  }
  return true;
#else
  (void)name;
  (void)params;
  (void)volume;
  (void)pitch;
  (void)generation;
  return false;
#endif
 }
 void setListener(float x, float y, float z, float lookX, float lookZ) {
  listenerX = x;
  listenerY = y;
  listenerZ = z;
  listenerLookX = lookX;
  listenerLookZ = lookZ;
#ifdef _WIN32
  for(auto& slot : slots) {
   if(slot->loaded && slot->spatial && slot->voice != nullptr) {
    applySpatial(*slot);
   }
  }
#endif
 }
 void stop(const std::string& name, std::uint64_t generation) {
#ifdef _WIN32
  if(SourceSlot* slot = findSlot(name)) {
   if(generation == 0 || slot->generation == generation) {
    clearSlot(*slot);
   }
  }
#else
  (void)name;
  (void)generation;
#endif
 }
 void setVolume(const std::string& name, float volume, std::uint64_t generation) {
#ifdef _WIN32
  SourceSlot* slot = findSlot(name);
  if(slot == nullptr || (generation != 0 && slot->generation != generation)) {
   return;
  }
  slot->userVolume = volume;
  if(slot->voice != nullptr) {
   if(slot->spatial) {
    applySpatial(*slot);
   } else {
    slot->voice->SetVolume(volume);
   }
  }
#else
  (void)name;
  (void)volume;
  (void)generation;
#endif
 }
 template <typename Finished>
 void reclaimFinished(Finished&& finished) {
#ifdef _WIN32
  for(auto& slot : slots) {
   if(slot->loaded && !slotPlaying(*slot)) {
    const std::string name = slot->name;
    const std::uint64_t generation = slot->generation;
    clearSlot(*slot);
    finished(name, generation);
   }
  }
#else
  (void)finished;
#endif
 }
 void stopAll() {
#ifdef _WIN32
  for(auto& slot : slots) {
   if(slot->loaded || !slot->name.empty()) {
    clearSlot(*slot);
   }
  }
#endif
 }
};
enum class CommandType {
 Playback,
 Stop,
 Volume,
 StopAll
};
struct Command {
 CommandType type = CommandType::Playback;
 std::string name;
 std::string path;
 SourceParams params;
 float volume = 1.0f;
 float pitch = 1.0f;
 std::uint64_t generation = 0;
 Clock::time_point queuedAt{};
 std::shared_ptr<const decode::PcmBuffer> pcm;
};
struct ListenerState {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 float lookX = 0.0f;
 float lookZ = 1.0f;
};
}
struct XAudio2Backend::Impl {
 mutable std::mutex mutex;
 std::condition_variable wake;
 std::condition_variable decodeWake;
 std::deque<Command> commands;
 std::deque<Command> effectDecodeQueue;
 std::deque<Command> longDecodeQueue;
 std::unordered_map<std::string, std::uint64_t> latestGeneration;
 std::unordered_map<std::string, bool> playingState;
 DecodeCache decodeCache;
 std::thread worker;
 std::array<std::thread, 2> effectDecoders;
 std::thread longDecoder;
 ListenerState listener;
 std::uint64_t nextGeneration = 0;
 std::size_t queuedPlaybacks = 0;
 bool listenerDirty = false;
 bool initialized = false;
 bool readyState = false;
 bool stopping = false;
 [[nodiscard]] bool isCurrent(const std::string& name, std::uint64_t generation) const {
  const std::scoped_lock lock(mutex);
  const auto latest = latestGeneration.find(name);
  return latest != latestGeneration.end() && latest->second == generation;
 }
 void publishStopped(const std::string& name, std::uint64_t generation) {
  const std::scoped_lock lock(mutex);
  const auto latest = latestGeneration.find(name);
  if(latest != latestGeneration.end() && latest->second == generation) {
   playingState[name] = false;
  }
 }
 void rejectPlayback(const std::string& name, std::uint64_t generation) {
  const std::scoped_lock lock(mutex);
  if(queuedPlaybacks > 0) {
   --queuedPlaybacks;
  }
  const auto latest = latestGeneration.find(name);
  if(latest != latestGeneration.end() && latest->second == generation) {
   playingState[name] = false;
  }
 }
 void decode(bool longForm) {
  for(;;) {
   Command command;
   {
    std::unique_lock lock(mutex);
    decodeWake.wait(lock, [this, longForm]() {
     return stopping || !(longForm ? longDecodeQueue : effectDecodeQueue).empty();
    });
    if(stopping) {
     return;
    }
    auto& queue = longForm ? longDecodeQueue : effectDecodeQueue;
    command = std::move(queue.front());
    queue.pop_front();
   }
   const bool transient = !command.params.loop && command.name != kMusicSlot && command.name != kRecordSlot;
   if(transient && Clock::now() > command.queuedAt + kTransientLifetime) {
    rejectPlayback(command.name, command.generation);
    continue;
   }
   command.pcm = decodeCache.decodeFile(command.path);
   if(!command.pcm || (transient && Clock::now() > command.queuedAt + kTransientLifetime)) {
    rejectPlayback(command.name, command.generation);
    continue;
   }
   bool accepted = false;
   {
    const std::scoped_lock lock(mutex);
    const auto latest = latestGeneration.find(command.name);
    if(!stopping && latest != latestGeneration.end() && latest->second == command.generation) {
     commands.push_back(std::move(command));
     accepted = true;
    } else if(queuedPlaybacks > 0) {
     --queuedPlaybacks;
    }
   }
   if(accepted) {
    wake.notify_one();
   }
  }
 }
 void run() {
  BackendCore core;
  const bool available = core.initialize();
  {
   const std::scoped_lock lock(mutex);
   initialized = true;
   readyState = available;
  }
  wake.notify_all();
  if(!available) {
   core.shutdown();
   return;
  }
  auto nextSweep = Clock::now() + kPlaybackSweepInterval;
  for(;;) {
   Command command;
   ListenerState nextListener;
   bool hasCommand = false;
   bool hasListener = false;
   {
    std::unique_lock lock(mutex);
    wake.wait_until(lock, nextSweep, [this]() { return stopping || listenerDirty || !commands.empty(); });
    if(stopping) {
     break;
    }
    if(listenerDirty) {
     nextListener = listener;
     listenerDirty = false;
     hasListener = true;
    }
    if(!commands.empty()) {
     command = std::move(commands.front());
     commands.pop_front();
     if(command.type == CommandType::Playback && queuedPlaybacks > 0) {
      --queuedPlaybacks;
     }
     hasCommand = true;
    }
   }
   if(hasListener) {
    core.setListener(nextListener.x, nextListener.y, nextListener.z, nextListener.lookX, nextListener.lookZ);
   }
    if(hasCommand) {
     if(command.type == CommandType::Playback) {
      const bool transient = !command.params.loop && command.name != kMusicSlot && command.name != kRecordSlot;
      const Clock::time_point expiresAt =
          transient ? command.queuedAt + kTransientLifetime : Clock::time_point::max();
      if(isCurrent(command.name, command.generation) &&
         !core.playDecoded(command.name,
                           command.pcm,
                           command.params,
                           command.volume,
                           command.pitch,
                           command.generation,
                           expiresAt)) {
       publishStopped(command.name, command.generation);
      }
    } else if(command.type == CommandType::Stop) {
     core.stop(command.name, command.generation);
     publishStopped(command.name, command.generation);
    } else if(command.type == CommandType::Volume) {
     core.setVolume(command.name, command.volume, command.generation);
    } else {
     core.stopAll();
    }
   }
   const auto now = Clock::now();
   if(now >= nextSweep) {
    core.reclaimFinished(
        [this](const std::string& name, std::uint64_t generation) { publishStopped(name, generation); });
    nextSweep = now + kPlaybackSweepInterval;
   }
  }
  core.stopAll();
  core.shutdown();
  {
   const std::scoped_lock lock(mutex);
   readyState = false;
   playingState.clear();
  }
 }
};
XAudio2Backend::XAudio2Backend() : impl_(std::make_unique<Impl>()) {
 impl_->worker = std::thread([state = impl_.get()]() { state->run(); });
 std::unique_lock lock(impl_->mutex);
 impl_->wake.wait(lock, [this]() { return impl_->initialized; });
 const bool available = impl_->readyState;
 lock.unlock();
 if(available) {
  for(auto& decoder : impl_->effectDecoders) {
   decoder = std::thread([state = impl_.get()]() { state->decode(false); });
  }
  impl_->longDecoder = std::thread([state = impl_.get()]() { state->decode(true); });
 }
}
XAudio2Backend::~XAudio2Backend() {
 {
  const std::scoped_lock lock(impl_->mutex);
  impl_->stopping = true;
  impl_->commands.clear();
  impl_->effectDecodeQueue.clear();
  impl_->longDecodeQueue.clear();
  impl_->queuedPlaybacks = 0;
 }
 impl_->wake.notify_one();
 impl_->decodeWake.notify_all();
 for(auto& decoder : impl_->effectDecoders) {
  if(decoder.joinable()) {
   decoder.join();
  }
 }
 if(impl_->longDecoder.joinable()) {
  impl_->longDecoder.join();
 }
 if(impl_->worker.joinable()) {
  impl_->worker.join();
 }
}
bool XAudio2Backend::ready() const {
 const std::scoped_lock lock(impl_->mutex);
 return impl_->readyState && !impl_->stopping;
}
void XAudio2Backend::setListener(
    float x, float y, float z, float lookX, float, float lookZ, float, float, float) {
 {
  const std::scoped_lock lock(impl_->mutex);
  if(!impl_->readyState || impl_->stopping) {
   return;
  }
  impl_->listener = {x, y, z, lookX, lookZ};
  impl_->listenerDirty = true;
 }
 impl_->wake.notify_one();
}
bool XAudio2Backend::playSourceFile(
    const std::string& name, const std::string& path, SourceParams params, float volume, float pitch) {
 if(name.empty() || path.empty()) {
  return false;
 }
 {
  const std::scoped_lock lock(impl_->mutex);
  if(!impl_->readyState || impl_->stopping || impl_->queuedPlaybacks >= kMaxQueuedPlaybacks) {
   return false;
  }
  const std::uint64_t generation = ++impl_->nextGeneration;
  impl_->latestGeneration[name] = generation;
  impl_->playingState[name] = true;
  auto& queue = params.loop || name == kMusicSlot || name == kRecordSlot ? impl_->longDecodeQueue
                                                                         : impl_->effectDecodeQueue;
  queue.push_back({CommandType::Playback, name, path, params, volume, pitch, generation, Clock::now()});
  ++impl_->queuedPlaybacks;
 }
 impl_->decodeWake.notify_all();
 return true;
}
void XAudio2Backend::stop(const std::string& name) {
 if(name.empty()) {
  return;
 }
 {
  const std::scoped_lock lock(impl_->mutex);
  if(!impl_->readyState || impl_->stopping) {
   return;
  }
  const auto latest = impl_->latestGeneration.find(name);
  const std::uint64_t generation = latest == impl_->latestGeneration.end() ? 0 : latest->second;
  impl_->latestGeneration[name] = ++impl_->nextGeneration;
  impl_->playingState[name] = false;
  impl_->commands.push_back({CommandType::Stop, name, {}, {}, 1.0f, 1.0f, generation, Clock::now()});
 }
 impl_->wake.notify_one();
}
void XAudio2Backend::setVolume(const std::string& name, float volume) {
 if(name.empty()) {
  return;
 }
 {
  const std::scoped_lock lock(impl_->mutex);
  if(!impl_->readyState || impl_->stopping) {
   return;
  }
  const auto latest = impl_->latestGeneration.find(name);
  const std::uint64_t generation = latest == impl_->latestGeneration.end() ? 0 : latest->second;
  impl_->commands.push_back({CommandType::Volume, name, {}, {}, volume, 1.0f, generation, Clock::now()});
 }
 impl_->wake.notify_one();
}
bool XAudio2Backend::playing(const std::string& name) const {
 const std::scoped_lock lock(impl_->mutex);
 const auto it = impl_->playingState.find(name);
 return it != impl_->playingState.end() && it->second;
}
void XAudio2Backend::stopAll() {
 {
  const std::scoped_lock lock(impl_->mutex);
  impl_->commands.clear();
  impl_->effectDecodeQueue.clear();
  impl_->longDecodeQueue.clear();
  impl_->queuedPlaybacks = 0;
  impl_->playingState.clear();
  impl_->latestGeneration.clear();
  if(impl_->readyState && !impl_->stopping) {
   impl_->commands.push_back({CommandType::StopAll});
  }
 }
 impl_->wake.notify_one();
 impl_->decodeWake.notify_all();
}
}
