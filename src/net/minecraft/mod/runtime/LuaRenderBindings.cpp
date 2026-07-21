#include "net/minecraft/mod/runtime/LuaRenderBindings.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#endif
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
using namespace net::minecraft::mod::model;
#ifdef MINECRAFT_NATIVE_EXPORTS
using net::minecraft::client::render::RenderSystem;
namespace {
bool gItemModelRenderOverride = false;
using RenderMatrixGuard = RenderSystem::MatrixScope;
} // namespace
[[nodiscard]] bool itemModelRenderOverrideActive() {
 return gItemModelRenderOverride;
}
void setItemModelRenderOverride(const bool enabled) {
 gItemModelRenderOverride = enabled;
}
int luaRenderSetItemEntityOverride(lua_State* state) {
 LuaApi& api = luaApi();
 gItemModelRenderOverride = api.gettop(state) >= 1 && api.toboolean(state, 1) != 0;
 return 0;
}
namespace {
bool readManualBlockVertex(lua_State* state, int tableIndex, ManualBlockVertex& vertex) {
 LuaApi& api = luaApi();
 if(api.type(state, tableIndex) != kLuaTTable) {
  return false;
 }
 vertex.x = luaFloatField(state, tableIndex, "x", luaFloatAt(state, tableIndex, 1, 0.0f));
 vertex.y = luaFloatField(state, tableIndex, "y", luaFloatAt(state, tableIndex, 2, 0.0f));
 vertex.z = luaFloatField(state, tableIndex, "z", luaFloatAt(state, tableIndex, 3, 0.0f));
 vertex.u = luaFloatField(state, tableIndex, "u", luaFloatAt(state, tableIndex, 4, 0.0f));
 vertex.v = luaFloatField(state, tableIndex, "v", luaFloatAt(state, tableIndex, 5, 0.0f));
 return std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z) && std::isfinite(vertex.u) &&
        std::isfinite(vertex.v);
}
int luaTessellatorQuad(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.type(state, 1) != kLuaTTable) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int tableIndex = 1;
 int textureId = luaIntField(state, tableIndex, "texture_id", -1);
 const std::string texturePath = luaStringField(state, tableIndex, "texture", "");
 if(!texturePath.empty()) {
   textureId = net::minecraft::registry::TextureRegistry::getOrRegisterTexture(texturePath);
 }
 const float red = std::clamp(luaFloatField(state, tableIndex, "r", 1.0f), 0.0f, 1.0f);
 const float green = std::clamp(luaFloatField(state, tableIndex, "g", 1.0f), 0.0f, 1.0f);
 const float blue = std::clamp(luaFloatField(state, tableIndex, "b", 1.0f), 0.0f, 1.0f);
 const float alpha = std::clamp(luaFloatField(state, tableIndex, "a", 1.0f), 0.0f, 1.0f);
 ManualBlockVertex vertices[4];
 api.getfield(state, tableIndex, "vertices");
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, tableIndex);
  api.pushboolean(state, 0);
  return 1;
 }
 const int verticesIndex = api.gettop(state);
 for(int i = 0; i < 4; ++i) {
  api.rawgeti(state, verticesIndex, i + 1);
  const bool ok = readManualBlockVertex(state, api.gettop(state), vertices[i]);
  api.settop(state, -2);
  if(!ok) {
   api.settop(state, tableIndex);
   api.pushboolean(state, 0);
   return 1;
  }
 }
 api.settop(state, tableIndex);
 const bool emitted = emitManualBlockModelQuad(vertices, textureId, red, green, blue, alpha) ||
                      emitManualItemModelQuad(vertices, textureId, red, green, blue, alpha);
 api.pushboolean(state, emitted ? 1 : 0);
 return 1;
}
int luaRenderDrawQuads(lua_State* state) {
 LuaApi& api = luaApi();
 if(!ModWorldDrawContext::active() || api.type(state, 1) != kLuaTTable || client::Minecraft::INSTANCE == nullptr) {
  api.pushinteger(state, 0);
  return 1;
 }
 const int specIndex = 1;
 const std::string texturePath = luaStringField(state, specIndex, "texture", "");
 const int rawTextureId = luaIntField(state, specIndex, "texture_id", -1);
 const bool textured = !texturePath.empty() || rawTextureId > 0;
 const bool blend = luaBoolField(state, specIndex, "blend", true);
 const bool cull = luaBoolField(state, specIndex, "cull", false);
 const bool depthTest = luaBoolField(state, specIndex, "depth_test", true);
 const bool depthWrite = luaBoolField(state, specIndex, "depth_write", true);
 const float defaultR = std::clamp(luaFloatField(state, specIndex, "r", 1.0f), 0.0f, 1.0f);
 const float defaultG = std::clamp(luaFloatField(state, specIndex, "g", 1.0f), 0.0f, 1.0f);
 const float defaultB = std::clamp(luaFloatField(state, specIndex, "b", 1.0f), 0.0f, 1.0f);
 const float defaultA = std::clamp(luaFloatField(state, specIndex, "a", 1.0f), 0.0f, 1.0f);
 double modelX = luaDoubleField(state, specIndex, "x", 0.0);
 double modelY = luaDoubleField(state, specIndex, "y", 0.0);
 double modelZ = luaDoubleField(state, specIndex, "z", 0.0);
 const float modelYaw = luaFloatField(state, specIndex, "yaw", 0.0f);
 const float modelPitch = luaFloatField(state, specIndex, "pitch", 0.0f);
 const float modelRoll = luaFloatField(state, specIndex, "roll", 0.0f);
 const float modelScale = luaFloatField(state, specIndex, "scale", 1.0f);
 // world_space anchors x/y/z (and therefore the vertices) in absolute world
 // coordinates; the active render camera is subtracted here.
 const bool worldSpace = luaBoolField(state, specIndex, "world_space", false);
 if(worldSpace) {
  const client::render::FrameRenderCamera& camera = client::render::RenderCameraState::instance().frame();
  modelX -= camera.x;
  modelY -= camera.y;
  modelZ -= camera.z;
 }
  const bool hasTransform = worldSpace || modelX != 0.0 || modelY != 0.0 || modelZ != 0.0 || modelYaw != 0.0f ||
                            modelPitch != 0.0f || modelRoll != 0.0f || modelScale != 1.0f;
  api.getfield(state, specIndex, "packed");
  const bool usePacked = api.type(state, -1) == kLuaTTable;
  if(!usePacked) {
   pop(state, 1);
  }
  constexpr std::size_t kPackedStride = 9;
  constexpr std::size_t kMaxVerticesPerBatch = 65536;
  int sourceIndex = 0;
  std::size_t vertexCount = 0;
  if(usePacked) {
   sourceIndex = api.gettop(state);
   const std::size_t numberCount = api.rawlen(state, sourceIndex);
   const std::size_t packedVertices = std::min(numberCount / kPackedStride, kMaxVerticesPerBatch);
   vertexCount = packedVertices - packedVertices % 4;
  } else {
   api.getfield(state, specIndex, "vertices");
   if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
   }
   sourceIndex = api.gettop(state);
   const std::size_t rawCount = std::min(api.rawlen(state, sourceIndex), kMaxVerticesPerBatch);
   vertexCount = rawCount - rawCount % 4;
  }
  if(vertexCount == 0) {
   pop(state, 1);
   api.pushinteger(state, 0);
   return 1;
  }
  const ModLuaDrawScope modCaps(textured, blend, cull, depthTest, depthWrite);
  if(textured) {
   int glTexture = -1;
   if(!texturePath.empty()) {
    glTexture = client::Minecraft::INSTANCE->textureManager.getTextureId(texturePath);
   } else if(rawTextureId > 0) {
    if(net::minecraft::registry::TextureRegistry::isCustomTexture(rawTextureId)) {
     glTexture = net::minecraft::registry::TextureRegistry::resolveGlId(
         rawTextureId, client::Minecraft::INSTANCE->textureManager);
    } else {
     glTexture = rawTextureId;
    }
   }
   RenderSystem::bindTexture(glTexture);
  }
  const RenderMatrixGuard modelScope;
  if(hasTransform) {
   RenderSystem::translate(static_cast<float>(modelX), static_cast<float>(modelY), static_cast<float>(modelZ));
   if(modelYaw != 0.0f) {
    RenderSystem::rotate(modelYaw, 0.0f, 1.0f, 0.0f);
   }
   if(modelPitch != 0.0f) {
    RenderSystem::rotate(modelPitch, 1.0f, 0.0f, 0.0f);
   }
   if(modelRoll != 0.0f) {
    RenderSystem::rotate(modelRoll, 0.0f, 0.0f, 1.0f);
   }
   if(modelScale != 1.0f) {
    RenderSystem::scale(modelScale, modelScale, modelScale);
   }
  }
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  std::size_t emitted = 0;
  if(usePacked) {
   for(std::size_t vertex = 0; vertex < vertexCount; ++vertex) {
    const long long base = static_cast<long long>(vertex * kPackedStride);
    double values[kPackedStride];
    for(std::size_t component = 0; component < kPackedStride; ++component) {
     api.rawgeti(state, sourceIndex, base + static_cast<long long>(component) + 1);
     int isNumber = 0;
     values[component] = api.tonumberx(state, -1, &isNumber);
     if(isNumber == 0) {
      values[component] = 0.0;
     }
     api.settop(state, -2);
    }
    tessellator.color(std::clamp(static_cast<float>(values[5]), 0.0f, 1.0f),
                      std::clamp(static_cast<float>(values[6]), 0.0f, 1.0f),
                      std::clamp(static_cast<float>(values[7]), 0.0f, 1.0f),
                      std::clamp(static_cast<float>(values[8]), 0.0f, 1.0f));
    if(textured) {
     tessellator.vertex(values[0], values[1], values[2], values[3], values[4]);
    } else {
     tessellator.vertex(values[0], values[1], values[2]);
    }
    ++emitted;
   }
  } else {
   for(std::size_t index = 1; index <= vertexCount; ++index) {
    api.rawgeti(state, sourceIndex, static_cast<long long>(index));
    const int vertexIndex = api.gettop(state);
    if(api.type(state, vertexIndex) != kLuaTTable) {
     pop(state, 1);
     continue;
    }
    const float r = std::clamp(luaFloatField(state, vertexIndex, "r", defaultR), 0.0f, 1.0f);
    const float g = std::clamp(luaFloatField(state, vertexIndex, "g", defaultG), 0.0f, 1.0f);
    const float b = std::clamp(luaFloatField(state, vertexIndex, "b", defaultB), 0.0f, 1.0f);
    const float a = std::clamp(luaFloatField(state, vertexIndex, "a", defaultA), 0.0f, 1.0f);
    tessellator.color(r, g, b, a);
    const double x = luaFloatField(state, vertexIndex, "x", 0.0f);
    const double y = luaFloatField(state, vertexIndex, "y", 0.0f);
    const double z = luaFloatField(state, vertexIndex, "z", 0.0f);
    if(textured) {
     const double u = luaFloatField(state, vertexIndex, "u", 0.0f);
     const double v = luaFloatField(state, vertexIndex, "v", 0.0f);
     tessellator.vertex(x, y, z, u, v);
    } else {
     tessellator.vertex(x, y, z);
    }
    ++emitted;
    pop(state, 1);
   }
  }
  pop(state, 1);
 if(emitted >= 4) {
  tessellator.draw();
 }
 api.pushinteger(state, static_cast<long long>(emitted / 4));
 return 1;
}
void drawBillboard(client::render::Tessellator& tessellator,
                   float yawDeg,
                   float pitchDeg,
                   double size,
                   float alpha,
                   float brightness,
                   float rotationXRad,
                   float rotationYRad) {
 constexpr float kDegToRad = 0.017453292f;
 const float azimuthRad = yawDeg * kDegToRad;
 const float elevationRad = pitchDeg * kDegToRad;
 const double baseX = std::cos(static_cast<double>(elevationRad)) * std::sin(static_cast<double>(azimuthRad));
 const double baseY = std::sin(static_cast<double>(elevationRad));
 const double baseZ = -std::cos(static_cast<double>(elevationRad)) * std::cos(static_cast<double>(azimuthRad));
 const double rotationYCos = std::cos(static_cast<double>(rotationYRad));
 const double rotationYSin = std::sin(static_cast<double>(rotationYRad));
 const double vecX = baseX * rotationYCos + baseZ * rotationYSin;
 const double yawedZ = -baseX * rotationYSin + baseZ * rotationYCos;
 const double rotationXCos = std::cos(static_cast<double>(rotationXRad));
 const double rotationXSin = std::sin(static_cast<double>(rotationXRad));
 const double vecY = baseY * rotationXCos - yawedZ * rotationXSin;
 const double vecZ = baseY * rotationXSin + yawedZ * rotationXCos;
 const double starX = vecX * 100.0;
 const double starY = vecY * 100.0;
 const double starZ = vecZ * 100.0;
 double viewVec[3] = {-vecX, -vecY, -vecZ};
 double worldUp[3] = {0.0, 1.0, 0.0};
 double rightVec[3] = {viewVec[1] * worldUp[2] - viewVec[2] * worldUp[1],
                       viewVec[2] * worldUp[0] - viewVec[0] * worldUp[2],
                       viewVec[0] * worldUp[1] - viewVec[1] * worldUp[0]};
 double rightMag = std::sqrt(rightVec[0] * rightVec[0] + rightVec[1] * rightVec[1] + rightVec[2] * rightVec[2]);
 if(rightMag < 0.001) {
  worldUp[0] = 1.0;
  worldUp[1] = 0.0;
  worldUp[2] = 0.0;
  rightVec[0] = viewVec[1] * worldUp[2] - viewVec[2] * worldUp[1];
  rightVec[1] = viewVec[2] * worldUp[0] - viewVec[0] * worldUp[2];
  rightVec[2] = viewVec[0] * worldUp[1] - viewVec[1] * worldUp[0];
  rightMag = std::sqrt(rightVec[0] * rightVec[0] + rightVec[1] * rightVec[1] + rightVec[2] * rightVec[2]);
 }
 if(rightMag < 0.001) {
  return;
 }
 rightVec[0] /= rightMag;
 rightVec[1] /= rightMag;
 rightVec[2] /= rightMag;
 const double upVec[3] = {rightVec[1] * viewVec[2] - rightVec[2] * viewVec[1],
                          rightVec[2] * viewVec[0] - rightVec[0] * viewVec[2],
                          rightVec[0] * viewVec[1] - rightVec[1] * viewVec[0]};
 const double rdx = rightVec[0] * size;
 const double rdy = rightVec[1] * size;
 const double rdz = rightVec[2] * size;
 const double udx = upVec[0] * size;
 const double udy = upVec[1] * size;
 const double udz = upVec[2] * size;
 tessellator.color(brightness, brightness, brightness, alpha);
 tessellator.vertex(starX - rdx + udx, starY - rdy + udy, starZ - rdz + udz);
 tessellator.vertex(starX + rdx + udx, starY + rdy + udy, starZ + rdz + udz);
 tessellator.vertex(starX + rdx - udx, starY + rdy - udy, starZ + rdz - udz);
 tessellator.vertex(starX - rdx - udx, starY - rdy - udy, starZ - rdz - udz);
}
int luaRenderDrawBillboards(lua_State* state) {
 LuaApi& api = luaApi();
 if(!ModWorldDrawContext::active() || api.type(state, 1) != kLuaTTable) {
  api.pushinteger(state, 0);
  return 1;
 }
 const int tableIndex = 1;
 const float brightness = luaFloatField(state, tableIndex, "brightness", 1.0f);
 const float rotationXRad = luaFloatField(state, tableIndex, "rotation_x_rad", 0.0f);
 const float rotationYRad = luaFloatField(state, tableIndex, "rotation_y_rad", 0.0f);
 const std::string blendMode = luaStringField(state, tableIndex, "blend", "alpha");
 const bool depthTest = luaBoolField(state, tableIndex, "depth_test", false);
 const bool depthWrite = luaBoolField(state, tableIndex, "depth_write", false);
 if(brightness <= 0.0f) {
  api.pushinteger(state, 0);
  return 1;
 }
  api.getfield(state, tableIndex, "packed");
  const bool usePacked = api.type(state, -1) == kLuaTTable;
  int sourceIndex = 0;
  int billboardCount = 0;
  constexpr int kMaxBillboardsPerBatch = 65536;
  if(usePacked) {
   sourceIndex = api.gettop(state);
   const std::size_t numberCount = api.rawlen(state, sourceIndex);
   billboardCount = static_cast<int>(std::min(numberCount / 5, static_cast<std::size_t>(kMaxBillboardsPerBatch)));
  } else {
   if(!usePacked) {
    pop(state, 1);
   }
   api.getfield(state, tableIndex, "billboards");
   if(api.type(state, -1) != kLuaTTable) {
    api.getfield(state, tableIndex, "points");
   }
   if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
   }
   sourceIndex = api.gettop(state);
   billboardCount = kMaxBillboardsPerBatch;
  }
  // Billboards are untextured, never culled, and always blended; "additive"
  // only swaps the destination factor the shared scope already set up.
  const ModLuaDrawScope modCaps(false, true, false, depthTest, depthWrite);
  if(blendMode == "additive") {
   RenderSystem::blendCustom(client::gl::blend::SrcAlpha, client::gl::blend::One);
  }
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  int emitted = 0;
  if(usePacked) {
   constexpr int kPackedStride = 5;
   for(int index = 0; index < billboardCount; ++index) {
    const long long base = static_cast<long long>(index) * kPackedStride;
    double values[kPackedStride];
    for(int component = 0; component < kPackedStride; ++component) {
     api.rawgeti(state, sourceIndex, base + component + 1);
     int isNumber = 0;
     values[component] = api.tonumberx(state, -1, &isNumber);
     if(isNumber == 0) {
      values[component] = 0.0;
     }
     api.settop(state, -2);
    }
    const float x = static_cast<float>(values[0]);
    const float y = static_cast<float>(values[1]);
    const float z = static_cast<float>(values[2]);
    const double size = values[3] > 0.0 ? values[3] : 0.2;
    const float alpha = static_cast<float>(values[4]);
    float yawDeg;
    float pitchDeg;
    {
     const float xyLen = std::sqrt(x * x + z * z);
     if(xyLen < 0.0001f && std::abs(y) > 0.9999f) {
      yawDeg = 0.0f;
      pitchDeg = (y >= 0.0f) ? 90.0f : -90.0f;
     } else {
      constexpr float kRadToDeg = 57.29578f;
      yawDeg = std::atan2(x, -z) * kRadToDeg;
      pitchDeg = std::atan2(y, xyLen) * kRadToDeg;
     }
    }
    drawBillboard(tessellator, yawDeg, pitchDeg, size, alpha, brightness, rotationXRad, rotationYRad);
    ++emitted;
   }
  } else {
   for(int index = 1; index <= billboardCount; ++index) {
    api.rawgeti(state, sourceIndex, index);
    if(api.type(state, -1) != kLuaTTable) {
     pop(state, 1);
     break;
    }
    const int pointIndex = api.gettop(state);
    const float x = luaFloatField(state, pointIndex, "x", 0.0f);
    const float y = luaFloatField(state, pointIndex, "y", 0.0f);
    const float z = luaFloatField(state, pointIndex, "z", 0.0f);
    float yawDeg;
    float pitchDeg;
    {
     const float xyLen = std::sqrt(x * x + z * z);
     if(xyLen < 0.0001f && std::abs(y) > 0.9999f) {
      yawDeg = 0.0f;
      pitchDeg = (y >= 0.0f) ? 90.0f : -90.0f;
     } else {
      constexpr float kRadToDeg = 57.29578f;
      yawDeg = std::atan2(x, -z) * kRadToDeg;
      pitchDeg = std::atan2(y, xyLen) * kRadToDeg;
     }
    }
    const double size = static_cast<double>(luaFloatField(state, pointIndex, "size", 0.2f));
    const float alpha = luaFloatField(state, pointIndex, "alpha", 1.0f);
    drawBillboard(tessellator, yawDeg, pitchDeg, size, alpha, brightness, rotationXRad, rotationYRad);
    ++emitted;
    pop(state, 1);
   }
  }
  pop(state, 1);
 if(emitted > 0) {
  tessellator.draw();
 }
 api.pushinteger(state, emitted);
 return 1;
}
} // namespace
void installRenderApi(lua_State* state) {
 LuaApi& api = luaApi();
 pushFunctionTable(state,
                   {
                       {"quads", luaRenderDrawQuads},
                       {"billboards", luaRenderDrawBillboards},
                       {"set_item_entity_override", luaRenderSetItemEntityOverride},
                   });
 api.setfield(state, -2, "render");
 pushFunctionTable(state,
                   {
                       {"quad", luaTessellatorQuad},
                   });
 api.setfield(state, -2, "tessellator");
}
#else
void installRenderApi(lua_State* state) {
 (void)state;
}
#endif
} // namespace net::minecraft::mod::runtime
