#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#define MINA_IMPLEMENTATION
#include "mina.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathConstants.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::platform::audio {
namespace {
constexpr const char* kMusicSlot = "BgMusic";
constexpr const char* kRecordSlot = "streaming";
constexpr float kUiSoundScale = 0.25f;
constexpr float kWorldAttenuationDistance = 16.0f;
constexpr float kRecordAttenuationDistance = 64.0f;
constexpr float kRecordVolumeScale = 0.5f;
constexpr mina_u32 kOutputSampleRate = 44100;
constexpr mina_u32 kOutputChannels = 2;
constexpr mina_u32 kMixFrames = 1024;
constexpr mina_u32 kMaxVoices = 64;
constexpr std::size_t kAudioBusCount = 6;
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
struct SoundLookup {
 const RegisteredSound* sound = nullptr;
 const SoundRegistry* registry = nullptr;
};
[[nodiscard]] bool isMusFile(const std::string& path) {
 std::string extension = std::filesystem::path(path).extension().string();
 std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
  return static_cast<char>(std::tolower(c));
 });
 return extension == ".mus";
}
// mina's loader hook: read the file, and undo the .mus stream cipher in place.
// Runs on mina's loader thread, so it touches nothing but its arguments.
mina_u8* loadSoundFile(const char* path, std::size_t* size, void* /*user*/) {
 std::FILE* file = std::fopen(path, "rb");
 if(file == nullptr) {
  return nullptr;
 }
 long length = 0;
 if(std::fseek(file, 0, SEEK_END) != 0 || (length = std::ftell(file)) <= 0 ||
    std::fseek(file, 0, SEEK_SET) != 0) {
  std::fclose(file);
  return nullptr;
 }
 auto* data = static_cast<mina_u8*>(std::malloc(static_cast<std::size_t>(length)));
 if(data == nullptr) {
  std::fclose(file);
  return nullptr;
 }
 const std::size_t read = std::fread(data, 1, static_cast<std::size_t>(length), file);
 std::fclose(file);
 if(read != static_cast<std::size_t>(length)) {
  std::free(data);
  return nullptr;
 }
 if(isMusFile(path)) {
  std::uint32_t hash = 0;
  const std::string name = std::filesystem::path(path).filename().string();
  for(unsigned char c : name) {
   hash = hash * 31u + c;
  }
  for(std::size_t i = 0; i < read; ++i) {
   const auto decrypted = static_cast<mina_u8>(data[i] ^ static_cast<mina_u8>(hash >> 8u));
   data[i] = decrypted;
   hash = hash * 498729871u + 85731u * static_cast<std::uint32_t>(static_cast<std::int8_t>(decrypted));
  }
 }
 *size = read;
 return data;
}
[[nodiscard]] std::string normalizeSoundBaseId(const std::string& soundName, bool stripVariantSuffix) {
 std::string baseId = soundName;
 if(stripVariantSuffix) {
  while(!baseId.empty() && std::isdigit(static_cast<unsigned char>(baseId.back())) != 0) {
   baseId.pop_back();
  }
 }
 for(char& c : baseId) {
  if(c == '/') {
   c = '.';
  }
 }
 return baseId;
}
void registerSound(SoundRegistry& registry, const std::string& soundName, const std::filesystem::path& file) {
 auto sound = std::make_unique<RegisteredSound>();
 sound->id = soundName;
 sound->path = file.generic_string();
 std::string nameWithoutExt = soundName;
 const std::string ext = file.extension().generic_string();
 if(!ext.empty() && nameWithoutExt.size() > ext.size()) {
  const auto pos = nameWithoutExt.rfind(ext);
  if(pos != std::string::npos && pos + ext.size() == nameWithoutExt.size()) {
   nameWithoutExt = nameWithoutExt.substr(0, pos);
  }
 }
 const std::string baseId = normalizeSoundBaseId(nameWithoutExt, registry.pickRandomVariant);
 registry.byBaseId[baseId].push_back(sound.get());
 registry.owned.push_back(std::move(sound));
}
[[nodiscard]] const RegisteredSound* findSound(const SoundRegistry& registry, const std::string& baseId) {
 const std::string normalized = normalizeSoundBaseId(baseId, registry.pickRandomVariant);
 const auto it = registry.byBaseId.find(normalized);
 if(it == registry.byBaseId.end() || it->second.empty()) {
  return nullptr;
 }
 const auto& variants = it->second;
 return variants[static_cast<std::size_t>(registry.random.nextInt(static_cast<int>(variants.size())))];
}
[[nodiscard]] const RegisteredSound* pickRandomSound(const SoundRegistry& registry) {
 if(registry.owned.empty()) {
  return nullptr;
 }
 return registry.owned[static_cast<std::size_t>(registry.random.nextInt(static_cast<int>(registry.owned.size())))]
     .get();
}
[[nodiscard]] SoundLookup findAnySound(const std::string& id,
                                       const SoundRegistry& effects,
                                       const SoundRegistry& streaming,
                                       const SoundRegistry& music,
                                       const SoundRegistry& voice) {
 if(const RegisteredSound* sound = findSound(effects, id)) {
  return {sound, &effects};
 }
 if(const RegisteredSound* sound = findSound(streaming, id)) {
  return {sound, &streaming};
 }
 if(const RegisteredSound* sound = findSound(music, id)) {
  return {sound, &music};
 }
 if(const RegisteredSound* sound = findSound(voice, id)) {
  return {sound, &voice};
 }
 return {};
}
[[nodiscard]] mina_source_params worldParams(float x, float y, float z, float maxDistance, bool loop) {
 mina_source_params params;
 mina_source_params_init(&params);
 params.loop = loop ? 1 : 0;
 params.spatial = 1;
 params.x = x;
 params.y = y;
 params.z = z;
 params.max_distance = maxDistance;
 return params;
}
} // namespace
struct AudioEngine::Impl {
 struct BusState {
  float gain = 1.0f;
  bool muted = false;
 };
 struct RestorableVoice {
  std::string slot;
  std::string path;
  mina_source_params params{};
  float volume = 1.0f;
  float pitch = 1.0f;
 };
 mina_engine* engine = nullptr;
 option::GameOptions* options = nullptr;
 bool started = false;
 SoundRegistry effects;
 SoundRegistry streaming;
 SoundRegistry music;
 SoundRegistry voice;
 std::array<BusState, kAudioBusCount> buses{};
 std::string outputBackend;
 std::string outputId;
 float duckGain = 0.45f;
 std::uint32_t duckHoldTicks = 12;
 std::uint32_t duckTicks = 0;
 std::string musicPath;
 std::string recordPath;
 mina_source_params musicParams{};
 mina_source_params recordParams{};
 std::vector<RestorableVoice> loops;
 int effectSlotSuffix = 0;
 int loopSlotSuffix = 0;
 JavaRandom random;
 int ticksUntilMusic = 0;
 std::mutex mutex;
 Impl() : ticksUntilMusic(random.nextInt(12000)) {
 }
 [[nodiscard]] static std::size_t busIndex(AudioBus bus) {
  return static_cast<std::size_t>(bus);
 }
 [[nodiscard]] static mina_bus nativeBus(AudioBus bus) {
  return static_cast<mina_bus>(busIndex(bus));
 }
 [[nodiscard]] float gain(AudioBus bus) const {
  const BusState& state = buses[busIndex(bus)];
  return state.muted ? 0.0f : state.gain;
 }
 [[nodiscard]] float categoryVolume(AudioBus bus) const {
  if(options == nullptr) {
   return 0.0f;
  }
  return bus == AudioBus::Music ? options->musicVolume : options->soundVolume;
 }
 void duck() {
 }
 void ensureEngine() {
  if(engine != nullptr) {
   return;
  }
  engine = mina_engine_create_output(outputBackend.empty() ? nullptr : outputBackend.c_str(),
                                     outputId.empty() ? nullptr : outputId.c_str(),
                                     kOutputSampleRate,
                                     kOutputChannels,
                                     kMixFrames,
                                     kMaxVoices);
  if(engine == nullptr || !mina_engine_is_real(engine)) {
   mina_engine_destroy(engine);
   engine = nullptr;
   return;
  }
  mina_engine_set_loader(engine, loadSoundFile, nullptr);
  // Without a threading layer the mixer stays caller-driven; there is no
  // pump loop here, so an engine that cannot start its threads is useless.
  if(!mina_engine_start(engine)) {
   mina_engine_destroy(engine);
   engine = nullptr;
   return;
  }
  for(std::size_t i = 0; i < kAudioBusCount; ++i) {
   mina_engine_set_bus_gain(engine, static_cast<mina_bus>(i), buses[i].gain);
   mina_engine_set_bus_muted(engine, static_cast<mina_bus>(i), buses[i].muted ? 1 : 0);
  }
  mina_engine_set_music_duck(engine, duckGain);
  started = true;
 }
 [[nodiscard]] bool ready() const {
  return started && engine != nullptr && options != nullptr && !mina_engine_device_failed(engine);
 }
 std::string nextEffectSlotName() {
  effectSlotSuffix = (effectSlotSuffix + 1) % 256;
  return "sound_" + std::to_string(effectSlotSuffix);
 }
 std::string nextLoopSlotName() {
  loopSlotSuffix = (loopSlotSuffix + 1) % 256;
  return "loop_sound_" + std::to_string(loopSlotSuffix);
 }
};
AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {
}
AudioEngine::~AudioEngine() {
 shutdown();
}
bool AudioEngine::isReady() const {
 return impl_->ready();
}
void AudioEngine::start(option::GameOptions* options) {
 impl_->options = options;
 impl_->streaming.pickRandomVariant = false;
 if(!impl_->started && options != nullptr && (options->soundVolume != 0.0f || options->musicVolume != 0.0f)) {
  impl_->ensureEngine();
 }
}
void AudioEngine::shutdown() {
 if(impl_->engine != nullptr) {
  mina_engine_stop_all(impl_->engine);
  mina_engine_destroy(impl_->engine);
  impl_->engine = nullptr;
 }
 impl_->started = false;
}
void AudioEngine::reset() {
 shutdown();
 const std::scoped_lock lock(impl_->mutex);
 impl_->effects.byBaseId.clear();
 impl_->effects.owned.clear();
 impl_->streaming.byBaseId.clear();
 impl_->streaming.owned.clear();
 impl_->music.byBaseId.clear();
 impl_->music.owned.clear();
 impl_->voice.byBaseId.clear();
 impl_->voice.owned.clear();
 impl_->musicPath.clear();
 impl_->recordPath.clear();
 impl_->loops.clear();
 impl_->effectSlotSuffix = 0;
 impl_->loopSlotSuffix = 0;
 impl_->ticksUntilMusic = impl_->random.nextInt(12000);
}
void AudioEngine::registerEffect(const std::string& id, const std::filesystem::path& file) {
 const std::scoped_lock lock(impl_->mutex);
 registerSound(impl_->effects, id, file);
}
void AudioEngine::registerStreaming(const std::string& id, const std::filesystem::path& file) {
 const std::scoped_lock lock(impl_->mutex);
 registerSound(impl_->streaming, id, file);
}
void AudioEngine::registerMusic(const std::string& id, const std::filesystem::path& file) {
 const std::scoped_lock lock(impl_->mutex);
 registerSound(impl_->music, id, file);
}
void AudioEngine::registerVoice(const std::string& id, const std::filesystem::path& file) {
 const std::scoped_lock lock(impl_->mutex);
 registerSound(impl_->voice, id, file);
}
void AudioEngine::setBusGain(AudioBus bus, float gain) {
 impl_->buses[Impl::busIndex(bus)].gain = std::max(0.0f, gain);
 if(impl_->engine != nullptr) {
  mina_engine_set_bus_gain(impl_->engine, Impl::nativeBus(bus), impl_->buses[Impl::busIndex(bus)].gain);
 }
 refreshMusicVolume();
}
void AudioEngine::setBusMuted(AudioBus bus, bool muted) {
 impl_->buses[Impl::busIndex(bus)].muted = muted;
 if(impl_->engine != nullptr) {
  mina_engine_set_bus_muted(impl_->engine, Impl::nativeBus(bus), muted ? 1 : 0);
 }
 refreshMusicVolume();
}
float AudioEngine::busGain(AudioBus bus) const {
 return impl_->buses[Impl::busIndex(bus)].gain;
}
bool AudioEngine::busMuted(AudioBus bus) const {
 return impl_->buses[Impl::busIndex(bus)].muted;
}
void AudioEngine::setMusicDucking(float gain, std::uint32_t holdTicks) {
 impl_->duckGain = std::clamp(gain, 0.0f, 1.0f);
 impl_->duckHoldTicks = holdTicks;
 if(impl_->engine != nullptr) {
  mina_engine_set_music_duck(impl_->engine, impl_->duckGain);
 }
}
std::vector<AudioOutputDevice> AudioEngine::outputDevices() const {
 const mina_u32 count = mina_device_enumerate(nullptr, 0);
 std::vector<mina_output_device> native(count);
 mina_device_enumerate(native.data(), count);
 std::vector<AudioOutputDevice> devices;
 devices.reserve(native.size());
 for(const mina_output_device& device : native) {
  devices.push_back({device.backend, device.id, device.name, device.is_default != 0});
 }
 return devices;
}
AudioOutputDevice AudioEngine::selectedOutputDevice() const {
 return {impl_->outputBackend, impl_->outputId, impl_->outputId.empty() ? "System default" : impl_->outputId, impl_->outputId.empty() || impl_->outputId == "default"};
}
AudioTelemetry AudioEngine::telemetry() const {
 AudioTelemetry snapshot;
 if(impl_->engine == nullptr) {
  return snapshot;
 }
 mina_engine_telemetry native{};
 mina_engine_get_telemetry(impl_->engine, &native);
 snapshot.underruns = mina_u64_to_size(native.underruns);
 snapshot.activeVoices = native.active_voices;
 snapshot.cacheBytes = native.cache_bytes;
 snapshot.cacheHits = mina_u64_to_size(native.cache_hits);
 snapshot.cacheMisses = mina_u64_to_size(native.cache_misses);
 snapshot.decodeLoads = mina_u64_to_size(native.loads);
 snapshot.decodeMilliseconds = mina_u64_to_size(native.load_milliseconds);
 snapshot.rejected = mina_u64_to_size(native.rejected);
 snapshot.dropped = mina_u64_to_size(native.dropped);
 return snapshot;
}
bool AudioEngine::selectOutputDevice(const std::string& backend, const std::string& id) {
 const std::vector<AudioOutputDevice> devices = outputDevices();
 const auto it = std::find_if(devices.begin(), devices.end(), [&](const AudioOutputDevice& device) {
  return device.backend == backend && device.id == id;
 });
 if(!backend.empty() && it == devices.end()) {
  return false;
 }
 impl_->outputBackend = backend;
 impl_->outputId = id;
 if(impl_->engine == nullptr) {
  return true;
 }
 return mina_engine_reopen_output(impl_->engine,
                                  backend.empty() ? nullptr : backend.c_str(),
                                  id.empty() ? nullptr : id.c_str()) != 0;
}
void AudioEngine::refreshMusicVolume() {
 if(impl_->options == nullptr) {
  return;
 }
 if(!impl_->started && (impl_->options->soundVolume != 0.0f || impl_->options->musicVolume != 0.0f)) {
  impl_->ensureEngine();
 }
 if(impl_->engine == nullptr) {
  return;
 }
 if(impl_->categoryVolume(AudioBus::Music) == 0.0f) {
  mina_engine_stop(impl_->engine, kMusicSlot);
 } else {
  mina_engine_set_volume(impl_->engine, kMusicSlot, impl_->categoryVolume(AudioBus::Music));
 }
}
void AudioEngine::updateListener(entity::LivingEntity* player, float partialTick) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f || player == nullptr) {
  return;
 }
 const float yaw = player->prevYaw + (player->yaw - player->prevYaw) * partialTick;
 const double x = player->prevX + (player->x - player->prevX) * static_cast<double>(partialTick);
 const double y = player->prevY + (player->y - player->prevY) * static_cast<double>(partialTick);
 const double z = player->prevZ + (player->z - player->prevZ) * static_cast<double>(partialTick);
 const float yawRad = -yaw * (util::math::kPiF / 180.0f) - util::math::kPiF;
 const float lookX = MathHelper::sin(yawRad);
 const float lookZ = MathHelper::cos(yawRad);
 mina_engine_listener(impl_->engine,
                      static_cast<float>(x),
                      static_cast<float>(y),
                      static_cast<float>(z),
                      -lookX,
                      0.0f,
                      -lookZ,
                      0.0f,
                      1.0f,
                      0.0f);
}
void AudioEngine::tick() {
 if(impl_->engine != nullptr && mina_engine_device_failed(impl_->engine)) {
  selectOutputDevice(impl_->outputBackend, impl_->outputId);
 }
 if(!impl_->ready()) {
  return;
 }
 refreshMusicVolume();
 if(impl_->categoryVolume(AudioBus::Music) == 0.0f || mina_engine_playing(impl_->engine, kMusicSlot)) {
  return;
 }
 std::string path;
 {
  const std::scoped_lock lock(impl_->mutex);
  if(impl_->ticksUntilMusic > 0) {
   --impl_->ticksUntilMusic;
   return;
  }
  const RegisteredSound* track = pickRandomSound(impl_->music);
  if(track == nullptr) {
   return;
  }
  impl_->ticksUntilMusic = impl_->random.nextInt(12000) + 12000;
  path = track->path;
 }
 mina_source_params params;
 mina_source_params_init(&params);
 params.stream = 1;
 params.bus = MINA_BUS_MUSIC;
 impl_->musicPath = path;
 impl_->musicParams = params;
 mina_engine_play_file(impl_->engine,
                       kMusicSlot,
                       path.c_str(),
                       &params,
                       impl_->categoryVolume(AudioBus::Music),
                       1.0f);
}
bool AudioEngine::playAt(const std::string& id, float x, float y, float z, float volume, float pitch) {
 if(!impl_->ready() || volume <= 0.0f) {
  return false;
 }
 std::string path;
 std::string slot;
 AudioBus bus = AudioBus::Sfx;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music, impl_->voice);
  if(lookup.sound == nullptr) {
   return false;
  }
  bus = lookup.registry == &impl_->effects ? AudioBus::Sfx :
        lookup.registry == &impl_->streaming ? AudioBus::Ambience :
        lookup.registry == &impl_->music ? AudioBus::Music : AudioBus::Voice;
  if(impl_->categoryVolume(bus) == 0.0f) {
   return false;
  }
  path = lookup.sound->path;
  slot = impl_->nextEffectSlotName();
 }
 impl_->duck();
 const float maxDistance = volume > 1.0f ? kWorldAttenuationDistance * volume : kWorldAttenuationDistance;
 mina_source_params params = worldParams(x, y, z, maxDistance, false);
 params.bus = Impl::nativeBus(bus);
 params.duck_music = bus == AudioBus::Sfx || bus == AudioBus::Voice ? 1 : 0;
 return mina_engine_play_file(impl_->engine,
                              slot.c_str(),
                              path.c_str(),
                              &params,
                              std::clamp(volume, 0.0f, 1.0f) * impl_->categoryVolume(bus),
                              pitch) != 0;
}
bool AudioEngine::play(const std::string& id, float volume, float pitch) {
 if(!impl_->ready() || impl_->categoryVolume(AudioBus::Ui) == 0.0f) {
  return false;
 }
 impl_->duck();
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music, impl_->voice);
  if(lookup.sound == nullptr) {
   return false;
  }
  path = lookup.sound->path;
  slot = impl_->nextEffectSlotName();
 }
 mina_source_params params;
 mina_source_params_init(&params);
 params.bus = MINA_BUS_UI;
 params.duck_music = 1;
 return mina_engine_play_file(impl_->engine,
                              slot.c_str(),
                              path.c_str(),
                              &params,
                              std::clamp(volume, 0.0f, 1.0f) * kUiSoundScale * impl_->categoryVolume(AudioBus::Ui),
                              pitch) != 0;
}
bool AudioEngine::playVoice(const std::string& id, float volume, float pitch) {
 if(!impl_->ready() || impl_->categoryVolume(AudioBus::Voice) == 0.0f || volume <= 0.0f) {
  return false;
 }
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const RegisteredSound* sound = findSound(impl_->voice, id);
  if(sound == nullptr) {
   return false;
  }
  path = sound->path;
  slot = impl_->nextEffectSlotName();
 }
 impl_->duck();
 mina_source_params params;
 mina_source_params_init(&params);
 params.bus = MINA_BUS_VOICE;
 params.duck_music = 1;
 return mina_engine_play_file(impl_->engine,
                              slot.c_str(),
                              path.c_str(),
                              &params,
                              std::clamp(volume, 0.0f, 1.0f) * impl_->categoryVolume(AudioBus::Voice),
                              pitch) != 0;
}
bool AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume) {
 if(!impl_->ready() || impl_->categoryVolume(AudioBus::Records) == 0.0f) {
  return false;
 }
 mina_engine_stop(impl_->engine, kRecordSlot);
 if(id.empty()) {
  impl_->recordPath.clear();
  return false;
 }
 std::string path;
 {
  const std::scoped_lock lock(impl_->mutex);
  const RegisteredSound* sound = findSound(impl_->streaming, id);
  if(sound == nullptr || volume <= 0.0f) {
   return false;
  }
  path = sound->path;
 }
 mina_source_params params = worldParams(x, y, z, kRecordAttenuationDistance, false);
 params.stream = 1;
 params.bus = MINA_BUS_RECORDS;
 impl_->recordPath = path;
 impl_->recordParams = params;
 return mina_engine_play_file(impl_->engine,
                              kRecordSlot,
                              path.c_str(),
                              &params,
                              kRecordVolumeScale * impl_->categoryVolume(AudioBus::Records),
                              1.0f) != 0;
}
std::string AudioEngine::playLoopAt(const std::string& id, float x, float y, float z, float volume, float pitch) {
 if(!impl_->ready() || impl_->categoryVolume(AudioBus::Ambience) == 0.0f || volume <= 0.0f) {
  return {};
 }
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music, impl_->voice);
  if(lookup.sound == nullptr) {
   return {};
  }
  path = lookup.sound->path;
  slot = impl_->nextLoopSlotName();
 }
 const float maxDistance = volume > 1.0f ? kWorldAttenuationDistance * volume : kWorldAttenuationDistance;
 mina_source_params params = worldParams(x, y, z, maxDistance, true);
 params.stream = 1;
 params.bus = MINA_BUS_AMBIENCE;
 if(mina_engine_play_file(impl_->engine,
                          slot.c_str(),
                          path.c_str(),
                          &params,
                          std::clamp(volume, 0.0f, 1.0f) * impl_->categoryVolume(AudioBus::Ambience),
                          pitch) == 0) {
  return {};
 }
 impl_->loops.push_back({slot, path, params, std::clamp(volume, 0.0f, 1.0f), pitch});
 return slot;
}
std::string AudioEngine::playLoopRegionAt(const std::string& id,
                                          float x,
                                          float y,
                                          float z,
                                          float volume,
                                          float pitch,
                                          std::uint64_t startFrame,
                                          std::uint64_t endFrame) {
 const std::string slot = playLoopAt(id, x, y, z, volume, pitch);
 if(!slot.empty()) {
  mina_engine_set_loop_region(impl_->engine,
                              slot.c_str(),
                              static_cast<mina_u64>(startFrame),
                              static_cast<mina_u64>(endFrame));
  for(Impl::RestorableVoice& loop : impl_->loops) {
   if(loop.slot == slot) {
    loop.params.loop_start = static_cast<mina_u64>(startFrame);
    loop.params.loop_end = static_cast<mina_u64>(endFrame);
    break;
   }
  }
 }
 return slot;
}
void AudioEngine::stop(const std::string& handle) {
 if(!impl_->ready() || handle.empty()) {
  return;
 }
 mina_engine_stop(impl_->engine, handle.c_str());
 impl_->loops.erase(std::remove_if(impl_->loops.begin(), impl_->loops.end(), [&](const Impl::RestorableVoice& loop) {
  return loop.slot == handle;
 }), impl_->loops.end());
 if(handle == kMusicSlot) {
  impl_->musicPath.clear();
 }
 if(handle == kRecordSlot) {
  impl_->recordPath.clear();
 }
}
bool AudioEngine::isPlaying(const std::string& handle) const {
 if(!impl_->ready() || handle.empty()) {
  return false;
 }
 return mina_engine_playing(impl_->engine, handle.c_str()) != 0;
}
} // namespace net::minecraft::client::platform::audio
