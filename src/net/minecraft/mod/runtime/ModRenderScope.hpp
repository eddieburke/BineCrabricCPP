#pragma once
#include <string_view>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include <optional>
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
enum class ModDrawLayer {
 Auto = 0,
 Entity,
 Terrain,
 Block,
 Sky,
 Basic,
};
[[nodiscard]] inline ModDrawLayer parseModDrawLayer(std::string_view name) noexcept {
 if(name == "entity") {
  return ModDrawLayer::Entity;
 }
 if(name == "terrain") {
  return ModDrawLayer::Terrain;
 }
 if(name == "block") {
  return ModDrawLayer::Block;
 }
 if(name == "sky") {
  return ModDrawLayer::Sky;
 }
 if(name == "basic") {
  return ModDrawLayer::Basic;
 }
 return ModDrawLayer::Auto;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
class ModLuaDrawScope {
 public:
 ModLuaDrawScope(bool textured, bool blend, bool cull, bool depthTest, bool depthWrite,
                 ModDrawLayer layer = ModDrawLayer::Auto)
     : kind_(pickPass(textured, blend, depthTest, depthWrite, layer)),
       pass_(passFor(kind_)) {
  applyOverrides(kind_, blend, cull, depthTest, depthWrite);
  client::render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 }
 ModLuaDrawScope(const ModLuaDrawScope&) = delete;
 ModLuaDrawScope& operator=(const ModLuaDrawScope&) = delete;
 [[nodiscard]] bool usesTerrainProgram() const noexcept {
  return kind_ == PassKind::TerrainCutout || kind_ == PassKind::TerrainTranslucent;
 }
 [[nodiscard]] bool usesEntityLighting() const noexcept {
  return kind_ == PassKind::EntityCutout || kind_ == PassKind::EntityTranslucent ||
         kind_ == PassKind::Block || kind_ == PassKind::BlockTranslucent;
 }

 private:
 enum class PassKind {
  Sky,
  SkyTextured,
  EntityCutout,
  EntityTranslucent,
  ParticlesTranslucent,
  TerrainCutout,
  TerrainTranslucent,
  Block,
  BlockTranslucent,
  Basic
 };
 [[nodiscard]] static PassKind pickPass(bool textured, bool blend, bool depthTest, bool depthWrite,
                                        ModDrawLayer layer) {
  if(!depthTest || layer == ModDrawLayer::Sky) {
   return textured ? PassKind::SkyTextured : PassKind::Sky;
  }
  if(layer == ModDrawLayer::Basic || !textured) {
   return PassKind::Basic;
  }
  if(layer == ModDrawLayer::Terrain) {
   if(blend) {
    return PassKind::TerrainTranslucent;
   }
   return PassKind::TerrainCutout;
  }
  if(layer == ModDrawLayer::Block) {
   return blend ? PassKind::BlockTranslucent : PassKind::Block;
  }
  if(textured) {
   if(blend && !depthWrite) {
    return PassKind::ParticlesTranslucent;
   }
   if(blend) {
    return PassKind::EntityTranslucent;
   }
   return PassKind::EntityCutout;
  }
  return PassKind::Basic;
 }
 [[nodiscard]] static client::render::RenderType& passFor(PassKind kind) {
  using client::render::RenderType;
  switch(kind) {
  case PassKind::Sky:
   return RenderType::sky();
  case PassKind::SkyTextured:
   return RenderType::skyTextured();
  case PassKind::EntityCutout:
   return RenderType::entityCutout();
  case PassKind::EntityTranslucent:
   return RenderType::entityTranslucent();
  case PassKind::ParticlesTranslucent:
   return RenderType::particlesTranslucent();
  case PassKind::TerrainCutout:
   return RenderType::cutout();
  case PassKind::TerrainTranslucent:
   return RenderType::translucent();
  case PassKind::Block:
   return RenderType::block();
  case PassKind::BlockTranslucent:
   return RenderType::blockTranslucent();
  case PassKind::Basic:
   return RenderType::basic();
  }
  return RenderType::entityCutout();
 }
 void applyOverrides(PassKind kind, bool blend, bool cull, bool depthTest, bool depthWrite) {
  namespace core = client::render::core;
  bool passBlend = false;
  bool passDepthTest = true;
  bool passDepthWrite = true;
  bool passCull = false;
  bool passLighting = false;
  switch(kind) {
  case PassKind::Sky:
  case PassKind::SkyTextured:
   passDepthTest = false;
   passDepthWrite = false;
   break;
  case PassKind::EntityCutout:
  case PassKind::Block:
   passCull = true;
   passLighting = true;
   break;
  case PassKind::TerrainCutout:
   passCull = false;
   passLighting = true;
   break;
  case PassKind::EntityTranslucent:
  case PassKind::BlockTranslucent:
  case PassKind::TerrainTranslucent:
   passBlend = true;
   break;
  case PassKind::ParticlesTranslucent:
   passBlend = true;
   passDepthWrite = false;
   break;
  case PassKind::Basic:
   passBlend = true;
   break;
  }
  if(blend != passBlend) {
   blendOverride_.emplace(blend);
  }
  if(cull != passCull) {
   cullOverride_.emplace(cull);
  }
  if(depthTest != passDepthTest || depthWrite != passDepthWrite) {
   depthOverride_.emplace(depthTest, depthWrite);
  }
  if(passLighting) {
   client::render::core::setLightingEnabled(false);
  }
 }
 PassKind kind_;
 client::render::RenderPassScope pass_;
 std::optional<client::render::core::BlendScope> blendOverride_;
 std::optional<client::render::core::CullScope> cullOverride_;
 std::optional<client::render::core::DepthScope> depthOverride_;
};
#endif
} // namespace net::minecraft::mod::runtime
