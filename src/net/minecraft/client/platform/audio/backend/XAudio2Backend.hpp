#pragma once

#include "net/minecraft/client/platform/audio/backend/AudioBackend.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace net::minecraft::client::platform::audio::backend {

class XAudio2Backend final : public AudioBackend {
public:
    XAudio2Backend();
    ~XAudio2Backend() override;

    [[nodiscard]] bool ready() const override;

    void setListener(
        float x, float y, float z,
        float lookX, float lookY, float lookZ,
        float upX, float upY, float upZ) override;

    bool loadSource(const std::string& name, const decode::PcmBuffer& pcm, SourceParams params) override;
    bool loadSourceFile(const std::string& name, const std::string& path, SourceParams params) override;
    bool loadSourceMemory(
        const std::string& name, const std::uint8_t* data, std::size_t size,
        const std::string& pathHint, SourceParams params) override;

    void play(const std::string& name) override;
    void stop(const std::string& name) override;
    void remove(const std::string& name) override;
    void setVolume(const std::string& name, float volume) override;
    void setPitch(const std::string& name, float pitch) override;
    [[nodiscard]] bool playing(const std::string& name) const override;
    void stopAll() override;
    void updateSpatialVolumes() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace net::minecraft::client::platform::audio::backend
