#include "net/minecraft/client/platform/audio/AudioEngine.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/platform/audio/backend/AudioBackend.hpp"
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
} // namespace
struct AudioEngine::Impl {
 std::unique_ptr<backend::XAudio2Backend> backend;
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
 void ensureBackend() {
  if(backend) {
   return;
  }
  backend = std::make_unique<backend::XAudio2Backend>();
  if(!backend->ready()) {
   backend.reset();
   return;
  }
  started = true;
 }
 [[nodiscard]] bool ready() const {
  return started && backend != nullptr && options != nullptr;
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
AudioEngine::~AudioEngine() = default;
bool AudioEngine::isReady() const {
 return impl_->ready();
}
void AudioEngine::start(option::GameOptions* options) {
 impl_->options = options;
 impl_->streaming.pickRandomVariant = false;
 if(!impl_->started && options != nullptr && (options->soundVolume != 0.0f || options->musicVolume != 0.0f)) {
  impl_->ensureBackend();
 }
}
void AudioEngine::shutdown() {
 if(impl_->backend) {
  impl_->backend->stopAll();
  impl_->backend.reset();
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
  impl_->ensureBackend();
 }
 if(!impl_->backend) {
  return;
 }
 if(impl_->options->musicVolume == 0.0f) {
  impl_->backend->stop(kMusicSlot);
 } else {
  impl_->backend->setVolume(kMusicSlot, impl_->options->musicVolume);
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
 impl_->backend->setListener(
     static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), -lookX, 0.0f, -lookZ, 0.0f, 1.0f, 0.0f);
}
void AudioEngine::tick() {
 if(!impl_->ready() || impl_->options->musicVolume == 0.0f) {
  return;
 }
 if(impl_->backend->playing(kMusicSlot) || impl_->backend->playing(kRecordSlot)) {
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
 impl_->backend->playSourceFile(kMusicSlot, path, {}, impl_->options->musicVolume, 1.0f);
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
 return impl_->backend->playSourceFile(slot,
                                      path,
                                      {false, true, x, y, z, maxDistance},
                                      std::clamp(volume, 0.0f, 1.0f) * impl_->options->soundVolume,
                                      pitch);
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
 return impl_->backend->playSourceFile(slot,
                                      path,
                                      {},
                                      std::clamp(volume, 0.0f, 1.0f) * kUiSoundScale * impl_->options->soundVolume,
                                      pitch);
}
bool AudioEngine::playRecord(const std::string& id, float x, float y, float z, float volume) {
 if(!impl_->ready() || impl_->options->soundVolume == 0.0f) {
  return false;
 }
 if(impl_->backend->playing(kRecordSlot)) {
  impl_->backend->stop(kRecordSlot);
 }
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
 if(impl_->backend->playing(kMusicSlot)) {
  impl_->backend->stop(kMusicSlot);
 }
 return impl_->backend->playSourceFile(kRecordSlot,
                                       path,
                                       {false, true, x, y, z, kRecordAttenuationDistance},
                                       kRecordVolumeScale * impl_->options->soundVolume,
                                       1.0f);
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
 if(!impl_->backend->playSourceFile(slot,
                                    path,
                                    {true, true, x, y, z, maxDistance},
                                    std::clamp(volume, 0.0f, 1.0f) * impl_->options->soundVolume,
                                    pitch)) {
  return {};
 }
 return slot;
}
void AudioEngine::stop(const std::string& handle) {
 if(!impl_->ready() || handle.empty()) {
  return;
 }
 impl_->backend->stop(handle);
}
bool AudioEngine::isPlaying(const std::string& handle) const {
 if(!impl_->ready() || handle.empty()) {
  return false;
 }
 return impl_->backend->playing(handle);
}
} // namespace net::minecraft::client::platform::audio
