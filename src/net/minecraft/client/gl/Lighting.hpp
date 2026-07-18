#pragma once
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::gl {
// Core-profile item lighting. The fixed-function two-directional-light + ambient model
// is reproduced by writing EnginePipeline's EngineLighting, which the ubershader reads.
class Lighting {
public:
  static void turnOff() noexcept {
    gl::setCap(gl::cap::Lighting, false);
    gl::setCap(gl::cap::ColorMaterial, false);
  }
  static void turnOn() noexcept {
    gl::setCap(gl::cap::Lighting, true);
    gl::setCap(gl::cap::ColorMaterial, true);
    gl::colorMaterial(gl::face::Front, gl::light::AmbientAndDiffuse);
    constexpr float diffuse = 0.6f;
    constexpr float ambient = 0.4f;
    const Vec3d sun0 = Vec3d{0.2, 1.0, -0.7}.normalize();
    const Vec3d sun1 = Vec3d{-0.2, 1.0, 0.7}.normalize();
    gl::EngineLighting& light = gl::g_engineLighting;
    setEyeSpaceDirection(light.dir0, sun0);
    setEyeSpaceDirection(light.dir1, sun1);
    light.color0[0] = light.color0[1] = light.color0[2] = diffuse;
    light.color1[0] = light.color1[1] = light.color1[2] = diffuse;
    light.ambient[0] = light.ambient[1] = light.ambient[2] = ambient;
    gl::shadeModel(gl::shade::Flat);
  }

private:
  static void setEyeSpaceDirection(float out[3], const Vec3d& direction) noexcept {
    const float* m = net::minecraft::util::math::g_modelView.top().data();
    out[0] = m[0] * static_cast<float>(direction.x) + m[4] * static_cast<float>(direction.y) +
             m[8] * static_cast<float>(direction.z);
    out[1] = m[1] * static_cast<float>(direction.x) + m[5] * static_cast<float>(direction.y) +
             m[9] * static_cast<float>(direction.z);
    out[2] = m[2] * static_cast<float>(direction.x) + m[6] * static_cast<float>(direction.y) +
             m[10] * static_cast<float>(direction.z);
  }
};
class LightingOffGuard {
public:
  LightingOffGuard() {
    Lighting::turnOff();
  }
  ~LightingOffGuard() {
    Lighting::turnOn();
  }
  LightingOffGuard(const LightingOffGuard&) = delete;
  LightingOffGuard& operator=(const LightingOffGuard&) = delete;
};
} // namespace net::minecraft::client::gl
