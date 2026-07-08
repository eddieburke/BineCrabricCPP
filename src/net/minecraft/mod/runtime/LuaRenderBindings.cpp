#include "net/minecraft/mod/runtime/LuaRenderBindings.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#endif
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
using net::minecraft::mod::texture;
namespace {
bool gItemModelRenderOverride = false;
}
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
  return std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z) &&
         std::isfinite(vertex.u) && std::isfinite(vertex.v);
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
    textureId = texture(texturePath.c_str());
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
  if(!ModWorldDrawContext::active() || api.type(state, 1) != kLuaTTable ||
     client::Minecraft::INSTANCE == nullptr) {
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
  const double modelX = luaFloatField(state, specIndex, "x", 0.0f);
  const double modelY = luaFloatField(state, specIndex, "y", 0.0f);
  const double modelZ = luaFloatField(state, specIndex, "z", 0.0f);
  const float modelYaw = luaFloatField(state, specIndex, "yaw", 0.0f);
  const float modelPitch = luaFloatField(state, specIndex, "pitch", 0.0f);
  const float modelRoll = luaFloatField(state, specIndex, "roll", 0.0f);
  const float modelScale = luaFloatField(state, specIndex, "scale", 1.0f);
  const bool hasTransform = modelX != 0.0 || modelY != 0.0 || modelZ != 0.0 || modelYaw != 0.0f ||
                            modelPitch != 0.0f || modelRoll != 0.0f || modelScale != 1.0f;
  api.getfield(state, specIndex, "vertices");
  if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  const int verticesIndex = api.gettop(state);
  constexpr std::size_t kMaxVerticesPerBatch = 65536;
  const std::size_t rawCount = std::min(api.rawlen(state, verticesIndex), kMaxVerticesPerBatch);
  const std::size_t vertexCount = rawCount - rawCount % 4;
  if(vertexCount == 0) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  for(std::size_t index = 1; index <= vertexCount; ++index) {
    api.rawgeti(state, verticesIndex, static_cast<long long>(index));
    const bool valid = api.type(state, -1) == kLuaTTable;
    pop(state, 1);
    if(!valid) {
      pop(state, 1);
      api.pushinteger(state, 0);
      return 1;
    }
  }
  const client::gl::preset::ModLuaDraw modCaps(textured, blend, cull, depthTest, depthWrite);
  client::render::platform::Lighting::turnOff();
  if(textured) {
    const int glTexture = rawTextureId > 0 && texturePath.empty()
                              ? rawTextureId
                              : client::Minecraft::INSTANCE->textureManager.getTextureId(texturePath);
    client::gl::bindTexture(client::gl::cap::Texture2D, glTexture);
  }
  client::gl::MatrixGuard modelScope;
  if(hasTransform) {
    client::gl::translatef(static_cast<float>(modelX), static_cast<float>(modelY), static_cast<float>(modelZ));
    if(modelYaw != 0.0f) {
      client::gl::rotatef(modelYaw, 0.0f, 1.0f, 0.0f);
    }
    if(modelPitch != 0.0f) {
      client::gl::rotatef(modelPitch, 1.0f, 0.0f, 0.0f);
    }
    if(modelRoll != 0.0f) {
      client::gl::rotatef(modelRoll, 0.0f, 0.0f, 1.0f);
    }
    if(modelScale != 1.0f) {
      client::gl::scaled(modelScale, modelScale, modelScale);
    }
  }
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  std::size_t emitted = 0;
  for(std::size_t index = 1; index <= vertexCount; ++index) {
    api.rawgeti(state, verticesIndex, static_cast<long long>(index));
    const int vertexIndex = api.gettop(state);
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
  pop(state, 1);
  if(emitted >= 4) {
    tessellator.draw();
  }
  api.pushinteger(state, static_cast<long long>(emitted / 4));
  return 1;
}
void drawSphericalBillboard(client::render::Tessellator& tessellator, float yawDeg, float pitchDeg, double size,
                            float alpha, float brightness, float rotationXRad, float rotationYRad) {
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
  api.getfield(state, tableIndex, "billboards");
  if(api.type(state, -1) != kLuaTTable) {
    api.getfield(state, tableIndex, "points");
  }
  if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  const int pointsIndex = api.gettop(state);
  const client::gl::preset::ModLuaBillboardDraw modCaps(blendMode == "additive", depthTest, depthWrite);
  client::render::platform::Lighting::turnOff();
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  int emitted = 0;
  constexpr int kMaxBillboardsPerBatch = 65536;
  for(int index = 1; index <= kMaxBillboardsPerBatch; ++index) {
    api.rawgeti(state, pointsIndex, index);
    if(api.type(state, -1) != kLuaTTable) {
      pop(state, 1);
      break;
    }
    const int pointIndex = api.gettop(state);
    const float yawDeg = luaFloatField(state, pointIndex, "yaw_deg", luaFloatField(state, pointIndex, "az", 0.0f));
    const float pitchDeg = luaFloatField(state, pointIndex, "pitch_deg", luaFloatField(state, pointIndex, "el", 0.0f));
    const double size = static_cast<double>(luaFloatField(state, pointIndex, "size", 0.2f));
    const float alpha = luaFloatField(state, pointIndex, "alpha", 1.0f);
    drawSphericalBillboard(tessellator, yawDeg, pitchDeg, size, alpha, brightness, rotationXRad, rotationYRad);
    ++emitted;
    pop(state, 1);
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
  pushFunctionTable(state, {
                                {"quads", luaRenderDrawQuads},
                                {"billboards", luaRenderDrawBillboards},
                                {"set_item_entity_override", luaRenderSetItemEntityOverride},
                            });
  api.setfield(state, -2, "render");
  pushFunctionTable(state, {
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
