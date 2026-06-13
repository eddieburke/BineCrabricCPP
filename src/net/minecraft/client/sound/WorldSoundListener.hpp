#pragma once

#include "net/minecraft/world/events/GameEventListener.hpp"

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft {
class World;
}

namespace net::minecraft::client::sound {

// Routes world playSound/playStreaming to AudioEngine (not the chunk renderer).
class WorldSoundListener final : public net::minecraft::GameEventListener {
public:
    explicit WorldSoundListener(Minecraft* client);

    void attach(net::minecraft::World* world);
    void detach(net::minecraft::World* world);
    void tickWeather(Minecraft& client);

    void playSound(const std::string& sound, double x, double y, double z, float volume, float pitch) override;
    void playStreaming(const std::string& stream, int x, int y, int z) override;

private:
    void playAt(const std::string& sound, double x, double y, double z, float volume, float pitch);

    Minecraft* client_ = nullptr;
    int rainSoundCounter_ = 0;
};

} // namespace net::minecraft::client::sound
