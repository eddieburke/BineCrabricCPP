#pragma once
#include <string_view>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#endif
namespace net::minecraft {
class World;
}
namespace net::minecraft::mod::runtime {
enum class ModDrawLayer {
 Auto = 0,
 Entity,
 Terrain,
 Block,
 Sky,
 Clouds,
 Basic,
};
class ModWorldDrawContext {
 public:
 static void begin(net::minecraft::World* world, float tickDelta, ModDrawLayer layer = ModDrawLayer::Auto) noexcept;
 static void end() noexcept;
 [[nodiscard]] static net::minecraft::World* world() noexcept;
 [[nodiscard]] static float tickDelta() noexcept;
 [[nodiscard]] static ModDrawLayer layer() noexcept;
 [[nodiscard]] static bool active() noexcept;
};
class ScopedModWorldDrawContext {
 public:
 ScopedModWorldDrawContext(net::minecraft::World* world,
                           float tickDelta,
                           ModDrawLayer layer = ModDrawLayer::Auto) noexcept;
 ~ScopedModWorldDrawContext();
 ScopedModWorldDrawContext(const ScopedModWorldDrawContext&) = delete;
 ScopedModWorldDrawContext& operator=(const ScopedModWorldDrawContext&) = delete;

 private:
 bool entered_ = false;
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
 if(name == "clouds") {
  return ModDrawLayer::Clouds;
 }
 if(name == "basic") {
  return ModDrawLayer::Basic;
 }
 return ModDrawLayer::Auto;
}
[[nodiscard]] inline std::string_view modDrawProgramKey(bool textured,
                                                        bool blend,
                                                        bool depthTest,
                                                        bool depthWrite,
                                                        ModDrawLayer layer) noexcept {
 if(layer == ModDrawLayer::Clouds) {
  return "gbuffers_clouds";
 }
 if(!depthTest || layer == ModDrawLayer::Sky) {
  return textured ? "gbuffers_skytextured" : "gbuffers_skybasic";
 }
 if(layer == ModDrawLayer::Basic || !textured) {
  return "gbuffers_basic";
 }
 if(layer == ModDrawLayer::Terrain) {
  return blend ? "gbuffers_water" : "gbuffers_terrain_cutout";
 }
 if(layer == ModDrawLayer::Block) {
  return blend ? "gbuffers_block_translucent" : "gbuffers_block";
 }
 if(blend && !depthWrite) {
  return "gbuffers_particles_translucent";
 }
 return blend ? "gbuffers_entities_translucent" : "gbuffers_entities";
}
#ifdef MINECRAFT_NATIVE_EXPORTS
class ModLuaDrawScope {
 public:
  ModLuaDrawScope(bool textured, bool blend, bool cull, bool depthTest, bool depthWrite,
                  ModDrawLayer layer = ModDrawLayer::Auto, int blendSrc = 0x0302, int blendDst = 0x0303)
      : spec_(composePass(textured, blend, cull, depthTest, depthWrite, layer, blendSrc, blendDst)),
        type_(buildPass(spec_)),
        pass_(type_),
        programKey_(spec_.programKey) {
   client::render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
  }
  ModLuaDrawScope(const ModLuaDrawScope&) = delete;
  ModLuaDrawScope& operator=(const ModLuaDrawScope&) = delete;
  [[nodiscard]] bool usesTerrainProgram() const noexcept {
   return programKey_ == "gbuffers_terrain_cutout" || programKey_ == "gbuffers_water";
  }
  [[nodiscard]] bool usesEntityLighting() const noexcept {
   return programKey_ == "gbuffers_entities" || programKey_ == "gbuffers_entities_translucent" ||
          programKey_ == "gbuffers_block" || programKey_ == "gbuffers_block_translucent";
  }

 private:
   struct PassSpec {
    std::string_view programKey;
    bool hasTexture = false;
    client::render::RenderType::State state;
   };
   [[nodiscard]] static PassSpec composePass(bool textured, bool blend, bool cull, bool depthTest,
                                            bool depthWrite, ModDrawLayer layer, int blendSrc, int blendDst) {
   const bool sky = !depthTest || layer == ModDrawLayer::Sky;
   const bool basic = !sky && (layer == ModDrawLayer::Basic || !textured);
   PassSpec spec;
   spec.programKey = modDrawProgramKey(textured, blend, depthTest, depthWrite, layer);
    if(layer == ModDrawLayer::Clouds) {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
    } else if(sky) {
     spec.hasTexture = textured;
     spec.state.depthTest = true;
     spec.state.depthWrite = depthTest && depthWrite;
    } else if(basic) {
     spec.hasTexture = false;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
    } else if(layer == ModDrawLayer::Terrain) {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
     if(!blend) {
      spec.state.alphaTest = true;
      spec.state.alphaRef = 0.1f;
     }
    } else if(layer == ModDrawLayer::Block) {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
     if(!blend) {
      spec.state.alphaTest = true;
      spec.state.alphaRef = 0.1f;
     }
    } else if(blend && !depthWrite) {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = false;
     spec.state.alphaTest = true;
     spec.state.alphaRef = 0.1f;
    } else if(blend) {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
    } else {
     spec.hasTexture = true;
     spec.state.depthTest = depthTest;
     spec.state.depthWrite = depthWrite;
     spec.state.alphaTest = true;
     spec.state.alphaRef = 0.1f;
    }
   spec.state.blend = blend;
   spec.state.blendSrc = blendSrc;
   spec.state.blendDst = blendDst;
   spec.state.cull = cull;
   spec.state.cullMode = 0x0405;
   spec.state.lighting = false;
   return spec;
  }
  [[nodiscard]] static client::render::RenderType buildPass(const PassSpec& spec) {
   return client::render::RenderType("mod_draw", 0x0004, spec.hasTexture, {},
                                     spec.state, client::render::worldProgramId(spec.programKey));
  }
  PassSpec spec_;
  client::render::RenderType type_;
  client::render::RenderPassScope pass_;
  std::string_view programKey_;
};
#endif
} // namespace net::minecraft::mod::runtime
