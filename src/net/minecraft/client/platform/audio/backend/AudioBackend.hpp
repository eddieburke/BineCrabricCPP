#pragma once
#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#include <memory>
#include <string>
namespace net::minecraft::client::platform::audio::backend {
struct SourceParams {
  bool loop = false;
  bool spatial = false;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float maxDistance = 16.0f;
};
class XAudio2Backend {
public:
  XAudio2Backend();
  ~XAudio2Backend();
  XAudio2Backend(const XAudio2Backend&) = delete;
  XAudio2Backend& operator=(const XAudio2Backend&) = delete;
  [[nodiscard]] bool ready() const;
  void setListener(float x, float y, float z, float lookX, float lookY, float lookZ, float upX, float upY, float upZ);
  bool loadSourceFile(const std::string& name, const std::string& path, SourceParams params);
  void play(const std::string& name);
  void stop(const std::string& name);
  void setVolume(const std::string& name, float volume);
  void setPitch(const std::string& name, float pitch);
  [[nodiscard]] bool playing(const std::string& name) const;
  void stopAll();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace net::minecraft::client::platform::audio::backend
