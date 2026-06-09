#pragma once

namespace net::minecraft::client::render::platform {

// Faithful port of net.minecraft.client.render.platform.Lighting.
class Lighting {
public:
    static void turnOff() noexcept;
    static void turnOn() noexcept;
};

} // namespace net::minecraft::client::render::platform
