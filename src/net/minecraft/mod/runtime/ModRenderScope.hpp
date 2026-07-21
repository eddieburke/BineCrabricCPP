#pragma once
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#endif
namespace net::minecraft {
class World;
}
namespace net::minecraft::mod::runtime {
class ModWorldDrawContext {
 public:
 static void begin(net::minecraft::World* world, float tickDelta) noexcept;
 static void end() noexcept;
 [[nodiscard]] static net::minecraft::World* world() noexcept;
 [[nodiscard]] static float tickDelta() noexcept;
 [[nodiscard]] static bool active() noexcept;
};
class ScopedModWorldDrawContext {
 public:
 ScopedModWorldDrawContext(net::minecraft::World* world, float tickDelta) noexcept;
 ~ScopedModWorldDrawContext();
 ScopedModWorldDrawContext(const ScopedModWorldDrawContext&) = delete;
 ScopedModWorldDrawContext& operator=(const ScopedModWorldDrawContext&) = delete;

 private:
 bool entered_ = false;
};
#ifdef MINECRAFT_NATIVE_EXPORTS
// State for one Lua-issued world draw. Opening the pass is what binds a shader
// program: without one every draw in engine_pipeline silently returns and
// nothing reaches the screen. The pass sets the base state and restores it (and
// the previous program) on exit; the per-draw options here layer on top of it.
class ModLuaDrawScope {
 public:
 ModLuaDrawScope(bool textured, bool blend, bool cull, bool depthTest, bool depthWrite)
     : saved_(client::render::RenderSystem::getShadow()) {
  using client::render::RenderSystem;
  RenderSystem::alphaTest(0.1f);
  if(textured) {
   RenderSystem::enableTexture();
  } else {
   RenderSystem::disableTexture();
  }
  if(blend) {
   RenderSystem::enableBlend();
   RenderSystem::blendAlpha();
  } else {
   RenderSystem::disableBlend();
  }
  if(cull) {
   RenderSystem::enableCull();
  } else {
   RenderSystem::disableCull();
  }
  if(depthTest) {
   // Also pins the depth func to LEQUAL rather than inheriting the stage's.
   RenderSystem::depthTestWrite(depthWrite);
  } else {
   RenderSystem::disableDepthTest();
   RenderSystem::depthMask(depthWrite);
  }
  RenderSystem::disableLighting();
  // The entity pass leaves uConstColor at the last entity's brightness; without
  // this reset every Lua model would be multiplied by it a second time.
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 }
 ~ModLuaDrawScope() {
  client::render::RenderSystem::setShadow(saved_);
 }
 ModLuaDrawScope(const ModLuaDrawScope&) = delete;
 ModLuaDrawScope& operator=(const ModLuaDrawScope&) = delete;

 private:
 // Declared first so the pass opens before the state below is captured and
 // overridden, and closes last — restoring the program the caller had bound.
 client::render::RenderPassScope pass_{client::render::RenderType::entityCutout()};
 client::render::RenderSystem::StateShadow saved_;
};
#endif
} // namespace net::minecraft::mod::runtime
