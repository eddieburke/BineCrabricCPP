#pragma once
// Game-facing audio API. Owns the device backend, resource registry, and playback.
#include <filesystem>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::platform::audio {
enum class AudioBus : std::uint8_t {
 Music,
 Ambience,
 Ui,
 Sfx,
 Records,
 Voice
};
struct AudioOutputDevice {
 std::string backend;
 std::string id;
 std::string name;
 bool isDefault = false;
};
struct AudioTelemetry {
 std::uint64_t underruns = 0;
 std::uint64_t activeVoices = 0;
 std::uint64_t cacheBytes = 0;
 std::uint64_t cacheHits = 0;
 std::uint64_t cacheMisses = 0;
 std::uint64_t decodeLoads = 0;
 std::uint64_t decodeMilliseconds = 0;
 std::uint64_t rejected = 0;
 std::uint64_t dropped = 0;
};
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
 void registerVoice(const std::string& id, const std::filesystem::path& file);
 void setBusGain(AudioBus bus, float gain);
 void setBusMuted(AudioBus bus, bool muted);
 [[nodiscard]] float busGain(AudioBus bus) const;
 [[nodiscard]] bool busMuted(AudioBus bus) const;
 void setMusicDucking(float gain, std::uint32_t holdTicks = 12);
 [[nodiscard]] std::vector<AudioOutputDevice> outputDevices() const;
 bool selectOutputDevice(const std::string& backend, const std::string& id);
 [[nodiscard]] AudioOutputDevice selectedOutputDevice() const;
 [[nodiscard]] AudioTelemetry telemetry() const;
 void refreshMusicVolume();
 void updateListener(entity::LivingEntity* player, float partialTick);
 void tick();
 bool playAt(const std::string& id, float x, float y, float z, float volume, float pitch);
 bool play(const std::string& id, float volume, float pitch);
 bool playVoice(const std::string& id, float volume, float pitch);
 bool playRecord(const std::string& id, float x, float y, float z, float volume);
 [[nodiscard]] std::string playLoopAt(const std::string& id, float x, float y, float z, float volume, float pitch);
 [[nodiscard]] std::string playLoopRegionAt(const std::string& id,
                                             float x,
                                             float y,
                                             float z,
                                             float volume,
                                             float pitch,
                                             std::uint64_t startFrame,
                                             std::uint64_t endFrame);
 void stop(const std::string& handle);
 [[nodiscard]] bool isPlaying(const std::string& handle) const;

 private:
 struct Impl;
 std::unique_ptr<Impl> impl_;
};
} // namespace net::minecraft::client::platform::audio
