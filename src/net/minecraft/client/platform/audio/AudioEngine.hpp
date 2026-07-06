#pragma once
// Game-facing audio API. Owns the device backend, resource registry, and playback.
#include <filesystem>
#include <memory>
#include <string>
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::platform::audio {
class AudioEngine {
public:
  AudioEngine();
  ~AudioEngine();
  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;
  [[nodiscard]] bool isReady() const;
  void start(option::GameOptions* options);
  void shutdown();
  void reset();
  void registerEffect(const std::string& id, const std::filesystem::path& file);
  void registerStreaming(const std::string& id, const std::filesystem::path& file);
  void registerMusic(const std::string& id, const std::filesystem::path& file);
  void refreshMusicVolume();
  void updateListener(entity::LivingEntity* player, float partialTick);
  void tick();
  void playAt(const std::string& id, float x, float y, float z, float volume, float pitch);
  void play(const std::string& id, float volume, float pitch);
  void playRecord(const std::string& id, float x, float y, float z, float volume);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace net::minecraft::client::platform::audio
