#pragma once
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::platform {
class Lighting {
public:
  static void turnOff() noexcept {
    gl::setCap(gl::cap::Lighting, false);
    gl::setCap(gl::light::Light0, false);
    gl::setCap(gl::light::Light1, false);
    gl::setCap(gl::cap::ColorMaterial, false);
  }
  static void turnOn() noexcept {
    gl::setCap(gl::cap::Lighting, true);
    gl::setCap(gl::light::Light0, true);
    gl::setCap(gl::light::Light1, true);
    gl::setCap(gl::cap::ColorMaterial, true);
    gl::colorMaterial(gl::face::Front, gl::light::AmbientAndDiffuse);
    constexpr float diffuse = 0.6f;
    constexpr float ambient = 0.4f;
    constexpr float specular = 0.0f;
    static float lightBuffer[4];
    auto setLight = [](int light, int pname, float r, float g, float b, float a) {
      lightBuffer[0] = r;
      lightBuffer[1] = g;
      lightBuffer[2] = b;
      lightBuffer[3] = a;
      gl::lightfv(light, pname, lightBuffer);
    };
    const Vec3d sun0 = Vec3d{0.2, 1.0, -0.7}.normalize();
    setLight(gl::light::Light0, gl::light::Position, static_cast<float>(sun0.x), static_cast<float>(sun0.y),
             static_cast<float>(sun0.z), 0.0f);
    setLight(gl::light::Light0, gl::light::Diffuse, diffuse, diffuse, diffuse, 1.0f);
    setLight(gl::light::Light0, gl::light::Ambient, 0.0f, 0.0f, 0.0f, 1.0f);
    setLight(gl::light::Light0, gl::light::Specular, specular, specular, specular, 1.0f);
    const Vec3d sun1 = Vec3d{-0.2, 1.0, 0.7}.normalize();
    setLight(gl::light::Light1, gl::light::Position, static_cast<float>(sun1.x), static_cast<float>(sun1.y),
             static_cast<float>(sun1.z), 0.0f);
    setLight(gl::light::Light1, gl::light::Diffuse, diffuse, diffuse, diffuse, 1.0f);
    setLight(gl::light::Light1, gl::light::Ambient, 0.0f, 0.0f, 0.0f, 1.0f);
    setLight(gl::light::Light1, gl::light::Specular, specular, specular, specular, 1.0f);
    gl::shadeModel(gl::shade::Flat);
    lightBuffer[0] = ambient;
    lightBuffer[1] = ambient;
    lightBuffer[2] = ambient;
    lightBuffer[3] = 1.0f;
    gl::lightModelfv(gl::light::LightModelAmbient, lightBuffer);
  }
};
class LightingOffGuard {
public:
  LightingOffGuard() { Lighting::turnOff(); }
  ~LightingOffGuard() { Lighting::turnOn(); }
  LightingOffGuard(const LightingOffGuard&) = delete;
  LightingOffGuard& operator=(const LightingOffGuard&) = delete;
};
} // namespace net::minecraft::client::render::platform
