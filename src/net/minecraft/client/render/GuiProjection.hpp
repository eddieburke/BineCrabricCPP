#pragma once
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render::gui_proj {
// Sole 2D ortho path (splash / HUD / screen / profiler / toast / loading).
inline void load(float rawW,
                 float rawH,
                 float modelViewZ = -2000.0f,
                 float nearPlane = 1000.0f,
                 float farPlane = 3000.0f) {
 net::minecraft::util::math::Matrix4f proj;
 proj.ortho(0.0f, rawW, rawH, 0.0f, nearPlane, farPlane);
 core::projectionStack().load(proj);
 core::modelViewStack().loadIdentity();
 core::modelViewStack().translate(0.0f, 0.0f, modelViewZ);
}
inline void load(const util::UiScale& scale,
                 float modelViewZ = -2000.0f,
                 float nearPlane = 1000.0f,
                 float farPlane = 3000.0f) {
 load(static_cast<float>(scale.rawWidth),
      static_cast<float>(scale.rawHeight),
      modelViewZ,
      nearPlane,
      farPlane);
}
// screen=true: disable lighting (menu overlays). One depth clear.
inline void begin(const util::UiScale& scale, int viewportW, int viewportH, bool screen = false) {
 core::viewport(0, 0, viewportW, viewportH);
 core::setGuiScaleFactor(scale.factor);
 core::disableCull();
 if(screen) {
  core::setLightingEnabled(false);
 }
 core::setFogEnabled(false);
 core::setAlphaTestRef(0.1f);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 core::clear(gl::attrib::DepthBufferBit);
 load(scale);
}
} // namespace net::minecraft::client::render::gui_proj
