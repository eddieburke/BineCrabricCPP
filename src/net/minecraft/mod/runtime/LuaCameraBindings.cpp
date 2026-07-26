#include "net/minecraft/mod/runtime/LuaCameraBindings.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/Screenshot.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
net::minecraft::client::render::FramebufferManager* renderTargets() {
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
  return nullptr;
 }
 return &client::Minecraft::INSTANCE->gameRenderer->renderTargets();
}
bool optionalInteger(const LuaArgs& args, int index, int& value) {
 return args.count() < index || args.integer(index, value);
}
bool optionalBoolean(lua_State* state, const LuaArgs& args, int index, bool& value) {
 if(args.count() < index) {
  return true;
 }
 LuaApi& api = luaApi();
 if(api.type(state, index) != kLuaTBoolean) {
  return false;
 }
 value = api.toboolean(state, index) != 0;
 return true;
}
int luaCameraCreate(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 2) {
  api.pushinteger(state, -1);
  return 1;
 }
 LuaArgs args(state);
 int w = 0;
 int h = 0;
 int colorCount = 1;
 bool useDepthTex = false;
 if(!args.integer(1, w) || !args.integer(2, h) || !optionalInteger(args, 3, colorCount) ||
    !optionalBoolean(state, args, 4, useDepthTex)) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, m->create(w, h, colorCount, useDepthTex));
 return 1;
}
int luaCameraCreateDisplaySize(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 0) {
  api.pushinteger(state, -1);
  return 1;
 }
 LuaArgs args(state);
 const int w = client::Minecraft::INSTANCE->displayWidth;
 const int h = client::Minecraft::INSTANCE->displayHeight;
 int colorCount = 1;
 bool useDepthTex = false;
 if(!optionalInteger(args, 1, colorCount) || !optionalBoolean(state, args, 2, useDepthTex)) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, m->create(w > 0 ? w : 1, h > 0 ? h : 1, colorCount, useDepthTex));
 return 1;
}
int luaCameraDestroy(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 1) {
  api.pushboolean(state, 0);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 if(!args.integer(1, handle)) {
  api.pushboolean(state, 0);
  return 1;
 }
 api.pushboolean(state, m->destroy(handle) ? 1 : 0);
 return 1;
}
int luaCameraResize(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 3) {
  api.pushboolean(state, 0);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 int width = 0;
 int height = 0;
 if(!args.integer(1, handle) || !args.integer(2, width) || !args.integer(3, height)) {
  api.pushboolean(state, 0);
  return 1;
 }
 api.pushboolean(state, m->resize(handle, width, height) ? 1 : 0);
 return 1;
}
int luaCameraWidth(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 1) {
  api.pushinteger(state, 0);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 if(!args.integer(1, handle)) {
  api.pushinteger(state, 0);
  return 1;
 }
 api.pushinteger(state, m->width(handle));
 return 1;
}
int luaCameraHeight(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 1) {
  api.pushinteger(state, 0);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 if(!args.integer(1, handle)) {
  api.pushinteger(state, 0);
  return 1;
 }
 api.pushinteger(state, m->height(handle));
 return 1;
}
int renderPerspectiveImpl(lua_State* state,
                           LuaApi& api,
                           net::minecraft::client::render::FramebufferManager* m,
                           bool shadowPass) {
 LuaArgs args(state);
 int handle = 0;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double yaw = 0.0;
 double pitch = 0.0;
 double roll = 0.0;
 double fov = 0.0;
 if(!args.integer(1, handle) || !args.number(2, x) || !args.number(3, y) || !args.number(4, z) ||
    !args.number(5, yaw) || !args.number(6, pitch) || !args.number(7, roll) || !args.number(8, fov)) {
  api.pushboolean(state, 0);
  return 1;
 }
 float nearPlane = 0.0f;
 float farPlane = 0.0f;
 bool includeEntities = true;
 int tickDeltaArg = 9;
 if(shadowPass) {
  double nearValue = 0.0;
  double farValue = 0.0;
  if(!args.optionalNumber(9, nearValue) || !args.optionalNumber(10, farValue) ||
     !optionalBoolean(state, args, 11, includeEntities)) {
   api.pushboolean(state, 0);
   return 1;
  }
  nearPlane = static_cast<float>(nearValue);
  farPlane = static_cast<float>(farValue);
  tickDeltaArg = 12;
 }
 double tickDelta = 1.0;
 int excludedEntityId = -1;
 if(!args.optionalNumber(tickDeltaArg, tickDelta) || !optionalInteger(args, tickDeltaArg + 1, excludedEntityId)) {
  api.pushboolean(state, 0);
  return 1;
 }
 api.pushboolean(state,
                 m->renderWorldTo(handle,
                                  *client::Minecraft::INSTANCE->gameRenderer,
                                  static_cast<float>(tickDelta),
                                  x,
                                  y,
                                  z,
                                  static_cast<float>(yaw),
                                  static_cast<float>(pitch),
                                  static_cast<float>(roll),
                                  static_cast<float>(fov),
                                  false,
                                  1.0f,
                                  1.0f,
                                  -1.0f,
                                  1.0f,
                                  shadowPass,
                                  includeEntities,
                                  nearPlane,
                                  farPlane,
                                  excludedEntityId)
                     ? 1
                     : 0);
 return 1;
}
int renderOrthographicImpl(lua_State* state,
                           LuaApi& api,
                           net::minecraft::client::render::FramebufferManager* m,
                           bool shadowPass) {
 LuaArgs args(state);
 int handle = 0;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double yaw = 0.0;
 double pitch = 0.0;
 double roll = 0.0;
 double halfWidth = 0.0;
 double halfHeight = 0.0;
 double nearPlane = 0.0;
 double farPlane = 0.0;
 if(!args.integer(1, handle) || !args.number(2, x) || !args.number(3, y) || !args.number(4, z) ||
    !args.number(5, yaw) || !args.number(6, pitch) || !args.number(7, roll) ||
    !args.number(8, halfWidth) || !args.number(9, halfHeight) || !args.number(10, nearPlane) ||
    !args.number(11, farPlane)) {
  api.pushboolean(state, 0);
  return 1;
 }
 bool includeEntities = true;
 int tickDeltaArg = 12;
 if(shadowPass) {
  if(!optionalBoolean(state, args, 12, includeEntities)) {
   api.pushboolean(state, 0);
   return 1;
  }
  tickDeltaArg = 13;
 }
 double tickDelta = 1.0;
 int excludedEntityId = -1;
 if(!args.optionalNumber(tickDeltaArg, tickDelta) || !optionalInteger(args, tickDeltaArg + 1, excludedEntityId)) {
  api.pushboolean(state, 0);
  return 1;
 }
 const bool valid = halfWidth > 0.0f && halfHeight > 0.0f && nearPlane != farPlane;
 api.pushboolean(state,
                 valid && m->renderWorldTo(handle,
                                           *client::Minecraft::INSTANCE->gameRenderer,
                                           static_cast<float>(tickDelta),
                                           x,
                                           y,
                                           z,
                                           static_cast<float>(yaw),
                                           static_cast<float>(pitch),
                                           static_cast<float>(roll),
                                           70.0f,
                                           true,
                                           static_cast<float>(halfWidth),
                                           static_cast<float>(halfHeight),
                                           static_cast<float>(nearPlane),
                                           static_cast<float>(farPlane),
                                           shadowPass,
                                           includeEntities,
                                           0.0f,
                                           0.0f,
                                           excludedEntityId)
                     ? 1
                     : 0);
 return 1;
}
int luaCameraRender(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 8) {
  api.pushboolean(state, 0);
  return 1;
 }
 renderPerspectiveImpl(state, api, m, false);
 return 1;
}
int luaCameraRenderShadowPerspective(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 8) {
  api.pushboolean(state, 0);
  return 1;
 }
 renderPerspectiveImpl(state, api, m, true);
 return 1;
}
int luaCameraRenderOrthographic(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 11) {
  api.pushboolean(state, 0);
  return 1;
 }
 renderOrthographicImpl(state, api, m, false);
 return 1;
}
int luaCameraRenderShadowOrthographic(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 11) {
  api.pushboolean(state, 0);
  return 1;
 }
 renderOrthographicImpl(state, api, m, true);
 return 1;
}
int luaCameraUnbind(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 0) {
  api.pushboolean(state, 0);
  return 1;
 }
 m->unbind();
 api.pushboolean(state, 1);
 return 1;
}
int luaCameraTexture(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 1) {
  api.pushinteger(state, -1);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 int attachmentIndex = 0;
 if(!args.integer(1, handle) || !optionalInteger(args, 2, attachmentIndex)) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, m->textureId(handle, attachmentIndex));
 return 1;
}
int luaCameraDepthTexture(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 1) {
  api.pushinteger(state, -1);
  return 1;
 }
 LuaArgs args(state);
 int handle = 0;
 if(!args.integer(1, handle)) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, m->depthTextureId(handle));
 return 1;
}
int luaCameraRendering(lua_State* state) {
 LuaApi& api = luaApi();
 auto* m = renderTargets();
 if(m == nullptr || api.gettop(state) < 0) {
  api.pushinteger(state, -1);
  return 1;
 }
 api.pushinteger(state, m->renderingHandle());
 return 1;
}
int luaCameraFarPlane(lua_State* state) {
 LuaApi& api = luaApi();
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
  api.pushnumber(state, 192.0);
  return 1;
 }
 api.pushnumber(state, client::Minecraft::INSTANCE->gameRenderer->farPlaneBlocks());
 return 1;
}
int luaCameraSaveScreenshot(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 auto* m = renderTargets();
 if(m == nullptr) {
  api.pushstring(state, "Failed: no render targets");
  return 1;
 }
 int handle = 0;
 if(!args.integer(1, handle)) {
  api.pushstring(state, "Failed: invalid camera handle");
  return 1;
 }
 int previousFbo = 0;
 ::glGetIntegerv(0x8CA6, &previousFbo);
 const client::render::RenderSystem::StateShadow previousState = client::render::RenderSystem::getShadow();
 if(!m->bind(handle)) {
  api.pushstring(state, "Failed: invalid camera handle");
  return 1;
 }
 const int w = m->width(handle);
 const int h = m->height(handle);
 if(w <= 0 || h <= 0) {
  client::gl::GLCore::bindFramebuffer(client::gl::framebuffer::Framebuffer, static_cast<unsigned>(previousFbo));
  client::render::RenderSystem::setShadow(previousState);
  api.pushstring(state, "Failed: invalid dimensions");
  return 1;
 }
 const std::string result = client::Screenshot::take(client::Minecraft::getRunDirectory(), w, h);
 client::gl::GLCore::bindFramebuffer(client::gl::framebuffer::Framebuffer, static_cast<unsigned>(previousFbo));
 client::render::RenderSystem::setShadow(previousState);
 api.pushstring(state, result.c_str());
 return 1;
}
} // namespace
void installCameraApi(lua_State* state) {
 LuaApi& api = luaApi();
 pushFunctionTable(state,
                   {
                       {"create", luaCameraCreate},
                       {"create_display_size", luaCameraCreateDisplaySize},
                       {"destroy", luaCameraDestroy},
                       {"resize", luaCameraResize},
                       {"width", luaCameraWidth},
                       {"height", luaCameraHeight},
                       {"render", luaCameraRender},
                       {"render_orthographic", luaCameraRenderOrthographic},
                       {"render_shadow_orthographic", luaCameraRenderShadowOrthographic},
                       {"render_shadow_perspective", luaCameraRenderShadowPerspective},
                       {"unbind", luaCameraUnbind},
                       {"texture", luaCameraTexture},
                       {"depth_texture", luaCameraDepthTexture},
                       {"rendering", luaCameraRendering},
                        {"far_plane", luaCameraFarPlane},
                        {"save_screenshot", luaCameraSaveScreenshot},
                    });
 api.setfield(state, -2, "camera");
}
} // namespace net::minecraft::mod::runtime
