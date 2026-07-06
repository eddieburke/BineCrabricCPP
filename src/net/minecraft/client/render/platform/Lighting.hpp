#pragma once
namespace net::minecraft::client::render::platform {
// Faithful port of net.minecraft.client.render.platform.Lighting.
class Lighting {
public:
  static void turnOff() noexcept;
  static void turnOn() noexcept;
};
class LightingOffGuard {
public:
  LightingOffGuard() { Lighting::turnOff(); }
  ~LightingOffGuard() { Lighting::turnOn(); }
  LightingOffGuard(const LightingOffGuard&) = delete;
  LightingOffGuard& operator=(const LightingOffGuard&) = delete;
};
} // namespace net::minecraft::client::render::platform
