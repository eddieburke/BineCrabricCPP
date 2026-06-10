#pragma once

#include "net/minecraft/client/platform/audio/decode/PcmBuffer.hpp"

#include <cstddef>
#include <cstdint>
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

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    [[nodiscard]] virtual bool ready() const = 0;

    virtual void setListener(
        float x, float y, float z,
        float lookX, float lookY, float lookZ,
        float upX, float upY, float upZ) = 0;

    virtual bool loadSource(const std::string& name, const decode::PcmBuffer& pcm, SourceParams params) = 0;
    virtual bool loadSourceFile(const std::string& name, const std::string& path, SourceParams params) = 0;
    virtual bool loadSourceMemory(
        const std::string& name, const std::uint8_t* data, std::size_t size,
        const std::string& pathHint, SourceParams params) = 0;

    virtual void play(const std::string& name) = 0;
    virtual void stop(const std::string& name) = 0;
    virtual void remove(const std::string& name) = 0;
    virtual void setVolume(const std::string& name, float volume) = 0;
    virtual void setPitch(const std::string& name, float pitch) = 0;
    [[nodiscard]] virtual bool playing(const std::string& name) const = 0;
    virtual void stopAll() = 0;
    virtual void updateSpatialVolumes() = 0;

    static std::unique_ptr<AudioBackend> create();
};

} // namespace net::minecraft::client::platform::audio::backend
