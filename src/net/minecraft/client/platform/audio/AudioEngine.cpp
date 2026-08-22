#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#define MINA_IMPLEMENTATION
#include "mina.h"
#include <algorithm>
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
                                       const SoundRegistry& music) {
 if(const RegisteredSound* sound = findSound(effects, id)) {
  return {sound, &effects};
 }
 if(const RegisteredSound* sound = findSound(streaming, id)) {
  return {sound, &streaming};
 }
 if(const RegisteredSound* sound = findSound(music, id)) {
  return {sound, &music};
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
 mina_engine* engine = nullptr;
 option::GameOptions* options = nullptr;
 bool started = false;
 SoundRegistry effects;
 SoundRegistry streaming;
 SoundRegistry music;
 int effectSlotSuffix = 0;
 int loopSlotSuffix = 0;
 JavaRandom random;
 int ticksUntilMusic = 0;
 std::mutex mutex;
 Impl() : ticksUntilMusic(random.nextInt(12000)) {
 }
 void ensureEngine() {
  if(engine != nullptr) {
   return;
  }
  engine = mina_engine_create(nullptr, kOutputSampleRate, kOutputChannels, kMixFrames, kMaxVoices);
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
 if(impl_->options->musicVolume == 0.0f) {
  mina_engine_stop(impl_->engine, kMusicSlot);
 } else {
  mina_engine_set_volume(impl_->engine, kMusicSlot, impl_->options->musicVolume);
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
 if(!impl_->ready() || impl_->options->musicVolume == 0.0f) {
  return;
 }
 if(mina_engine_playing(impl_->engine, kMusicSlot) || mina_engine_playing(impl_->engine, kRecordSlot)) {
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
 params.stream = 1; // a whole track decoded up front would cost ~60 MB
 mina_engine_play_file(impl_->engine, kMusicSlot, path.c_str(), &params, impl_->options->musicVolume, 1.0f);
}
bool AudioEngine::playAt(const std::string& id, float x, float y, float z, float volume, float pitch) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f || volume <= 0.0f) {
  return false;
 }
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music);
  if(lookup.sound == nullptr) {
   return false;
  }
  path = lookup.sound->path;
  slot = impl_->nextEffectSlotName();
 }
 const float maxDistance = volume > 1.0f ? kWorldAttenuationDistance * volume : kWorldAttenuationDistance;
 const mina_source_params params = worldParams(x, y, z, maxDistance, false);
 return mina_engine_play_file(impl_->engine,
                              slot.c_str(),
                              path.c_str(),
                              &params,
                              std::clamp(volume, 0.0f, 1.0f) * impl_->options->soundVolume,
                              pitch) != 0;
}
bool AudioEngine::play(const std::string& id, float volume, float pitch) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f) {
  return false;
 }
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music);
  if(lookup.sound == nullptr) {
   return false;
  }
  path = lookup.sound->path;
  slot = impl_->nextEffectSlotName();
 }
 mina_source_params params;
 mina_source_params_init(&params);
 return mina_engine_play_file(impl_->engine,
                              slot.c_str(),
                              path.c_str(),
                              &params,
                              std::clamp(volume, 0.0f, 1.0f) * kUiSoundScale * impl_->options->soundVolume,
                              pitch) != 0;
}
bool AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f) {
  return false;
 }
 mina_engine_stop(impl_->engine, kRecordSlot);
 if(id.empty()) {
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
 mina_engine_stop(impl_->engine, kMusicSlot);
 mina_source_params params = worldParams(x, y, z, kRecordAttenuationDistance, false);
 params.stream = 1;
 return mina_engine_play_file(impl_->engine,
                              kRecordSlot,
                              path.c_str(),
                              &params,
                              kRecordVolumeScale * impl_->options->soundVolume,
                              1.0f) != 0;
}
std::string AudioEngine::playLoopAt(const std::string& id, float x, float y, float z, float volume, float pitch) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f || volume <= 0.0f) {
  return {};
 }
 std::string path;
 std::string slot;
 {
  const std::scoped_lock lock(impl_->mutex);
  const SoundLookup lookup = findAnySound(id, impl_->effects, impl_->streaming, impl_->music);
  if(lookup.sound == nullptr) {
   return {};
  }
  path = lookup.sound->path;
  slot = impl_->nextLoopSlotName();
 }
 const float maxDistance = volume > 1.0f ? kWorldAttenuationDistance * volume : kWorldAttenuationDistance;
 const mina_source_params params = worldParams(x, y, z, maxDistance, true);
 if(mina_engine_play_file(impl_->engine,
                          slot.c_str(),
                          path.c_str(),
                          &params,
                          std::clamp(volume, 0.0f, 1.0f) * impl_->options->soundVolume,
                          pitch) == 0) {
  return {};
 }
 return slot;
}
void AudioEngine::stop(const std::string& handle) {
 if(!impl_->ready() || handle.empty()) {
  return;
 }
 mina_engine_stop(impl_->engine, handle.c_str());
}
bool AudioEngine::isPlaying(const std::string& handle) const {
 if(!impl_->ready() || handle.empty()) {
  return false;
 }
 return mina_engine_playing(impl_->engine, handle.c_str()) != 0;
}
} // namespace net::minecraft::client::platform::audio
