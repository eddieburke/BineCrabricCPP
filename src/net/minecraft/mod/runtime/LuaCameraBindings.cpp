#include "net/minecraft/mod/runtime/LuaCameraBindings.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;

namespace {

net::minecraft::client::render::FramebufferManager* renderTargets() {
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
  return nullptr;
 }
 return &client::Minecraft::INSTANCE->gameRenderer->renderTargets();
}

#define LUA_CAM_IMPL(FUNC_NAME, MIN_ARGS, FAIL_RET, BODY) \
 int FUNC_NAME(lua_State* state) {                        \
  LuaApi& api = luaApi();                                 \
  auto* m = renderTargets();                              \
  if(m == nullptr || api.gettop(state) < MIN_ARGS) {      \
   FAIL_RET;                                              \
   return 1;                                              \
  }                                                       \
  BODY return 1;                                          \
 }

LUA_CAM_IMPL(luaCameraCreate, 2, api.pushinteger(state, -1), {
 const int w = static_cast<int>(api.tointegerx(state, 1, nullptr));
 const int h = static_cast<int>(api.tointegerx(state, 2, nullptr));
 const int colorCount = api.gettop(state) >= 3 ? static_cast<int>(api.tointegerx(state, 3, nullptr)) : 1;
 const bool useDepthTex = api.gettop(state) >= 4 ? api.toboolean(state, 4) != 0 : false;
 api.pushinteger(state, m->create(w, h, colorCount, useDepthTex));
})

LUA_CAM_IMPL(luaCameraCreateDisplaySize, 0, api.pushinteger(state, -1), {
 const int w = client::Minecraft::INSTANCE->displayWidth;
 const int h = client::Minecraft::INSTANCE->displayHeight;
 const int colorCount = api.gettop(state) >= 1 ? static_cast<int>(api.tointegerx(state, 1, nullptr)) : 1;
 const bool useDepthTex = api.gettop(state) >= 2 ? api.toboolean(state, 2) != 0 : false;
 api.pushinteger(state, m->create(w > 0 ? w : 1, h > 0 ? h : 1, colorCount, useDepthTex));
})

LUA_CAM_IMPL(luaCameraDestroy, 1, api.pushboolean(state, 0), {
 api.pushboolean(state, m->destroy(static_cast<int>(api.tointegerx(state, 1, nullptr))) ? 1 : 0);
})

LUA_CAM_IMPL(luaCameraResize, 3, api.pushboolean(state, 0), {
 api.pushboolean(state,
                 m->resize(static_cast<int>(api.tointegerx(state, 1, nullptr)),
                           static_cast<int>(api.tointegerx(state, 2, nullptr)),
                           static_cast<int>(api.tointegerx(state, 3, nullptr)))
                     ? 1
                     : 0);
})

LUA_CAM_IMPL(luaCameraWidth, 1, api.pushinteger(state, 0), {
 api.pushinteger(state, m->width(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})

LUA_CAM_IMPL(luaCameraHeight, 1, api.pushinteger(state, 0), {
 api.pushinteger(state, m->height(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})

// Shared body for the perspective render bindings: `render` and
// `render_shadow_perspective` take the same eight leading arguments and differ
// only in the shadow-pass flag and its optional near/far/include-entities tail.
int renderPerspectiveImpl(lua_State* state,
                          LuaApi& api,
                          net::minecraft::client::render::FramebufferManager* m,
                          bool shadowPass) {
 const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
 const double x = api.tonumberx(state, 2, nullptr);
 const double y = api.tonumberx(state, 3, nullptr);
 const double z = api.tonumberx(state, 4, nullptr);
 const float yaw = static_cast<float>(api.tonumberx(state, 5, nullptr));
 const float pitch = static_cast<float>(api.tonumberx(state, 6, nullptr));
 const float roll = static_cast<float>(api.tonumberx(state, 7, nullptr));
 const float fov = static_cast<float>(api.tonumberx(state, 8, nullptr));
 float nearPlane = 0.0f;
 float farPlane = 0.0f;
 bool includeEntities = true;
 int tickDeltaArg = 9;
 if(shadowPass) {
  nearPlane = api.gettop(state) >= 9 ? static_cast<float>(api.tonumberx(state, 9, nullptr)) : 0.0f;
  farPlane = api.gettop(state) >= 10 ? static_cast<float>(api.tonumberx(state, 10, nullptr)) : 0.0f;
  includeEntities = api.gettop(state) >= 11 ? api.toboolean(state, 11) != 0 : true;
  tickDeltaArg = 12;
 }
 const float tickDelta =
     api.gettop(state) >= tickDeltaArg ? static_cast<float>(api.tonumberx(state, tickDeltaArg, nullptr)) : 1.0f;
 api.pushboolean(state,
                 m->renderWorldTo(handle,
                                  *client::Minecraft::INSTANCE->gameRenderer,
                                  tickDelta,
                                  x,
                                  y,
                                  z,
                                  yaw,
                                  pitch,
                                  roll,
                                  fov,
                                  false,
                                  1.0f,
                                  1.0f,
                                  -1.0f,
                                  1.0f,
                                  shadowPass,
                                  includeEntities,
                                  nearPlane,
                                  farPlane)
                     ? 1
                     : 0);
 return 1;
}

// Shared body for the plain and shadow orthographic render bindings; they
// take the same 11 leading arguments and differ only in the shadow pass flag
// and its optional include-entities argument.
int renderOrthographicImpl(lua_State* state,
                           LuaApi& api,
                           net::minecraft::client::render::FramebufferManager* m,
                           bool shadowPass) {
 const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
 const double x = api.tonumberx(state, 2, nullptr);
 const double y = api.tonumberx(state, 3, nullptr);
 const double z = api.tonumberx(state, 4, nullptr);
 const float yaw = static_cast<float>(api.tonumberx(state, 5, nullptr));
 const float pitch = static_cast<float>(api.tonumberx(state, 6, nullptr));
 const float roll = static_cast<float>(api.tonumberx(state, 7, nullptr));
 const float halfWidth = static_cast<float>(api.tonumberx(state, 8, nullptr));
 const float halfHeight = static_cast<float>(api.tonumberx(state, 9, nullptr));
 const float nearPlane = static_cast<float>(api.tonumberx(state, 10, nullptr));
 const float farPlane = static_cast<float>(api.tonumberx(state, 11, nullptr));
 bool includeEntities = true;
 int tickDeltaArg = 12;
 if(shadowPass) {
  includeEntities = api.gettop(state) >= 12 ? api.toboolean(state, 12) != 0 : true;
  tickDeltaArg = 13;
 }
 const float tickDelta =
     api.gettop(state) >= tickDeltaArg ? static_cast<float>(api.tonumberx(state, tickDeltaArg, nullptr)) : 1.0f;
 const bool valid = halfWidth > 0.0f && halfHeight > 0.0f && nearPlane != farPlane;
 api.pushboolean(state,
                 valid && m->renderWorldTo(handle,
                                           *client::Minecraft::INSTANCE->gameRenderer,
                                           tickDelta,
                                           x,
                                           y,
                                           z,
                                           yaw,
                                           pitch,
                                           roll,
                                           70.0f,
                                           true,
                                           halfWidth,
                                           halfHeight,
                                           nearPlane,
                                           farPlane,
                                           shadowPass,
                                           includeEntities)
                     ? 1
                     : 0);
 return 1;
}

LUA_CAM_IMPL(luaCameraRender, 8, api.pushboolean(state, 0), { renderPerspectiveImpl(state, api, m, false); })

LUA_CAM_IMPL(luaCameraRenderShadowPerspective, 8, api.pushboolean(state, 0), {
 renderPerspectiveImpl(state, api, m, true);
})

LUA_CAM_IMPL(luaCameraRenderOrthographic, 11, api.pushboolean(state, 0), {
 renderOrthographicImpl(state, api, m, false);
})

LUA_CAM_IMPL(luaCameraRenderShadowOrthographic, 11, api.pushboolean(state, 0), {
 renderOrthographicImpl(state, api, m, true);
})

LUA_CAM_IMPL(luaCameraUnbind, 0, api.pushboolean(state, 0), {
 m->unbind();
 api.pushboolean(state, 1);
})

LUA_CAM_IMPL(luaCameraTexture, 1, api.pushinteger(state, -1), {
 const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
 const int attachmentIndex = api.gettop(state) >= 2 ? static_cast<int>(api.tointegerx(state, 2, nullptr)) : 0;
 api.pushinteger(state, m->textureId(handle, attachmentIndex));
})

LUA_CAM_IMPL(luaCameraDepthTexture, 1, api.pushinteger(state, -1), {
 api.pushinteger(state, m->depthTextureId(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})

LUA_CAM_IMPL(luaCameraRendering, 0, api.pushinteger(state, -1), { api.pushinteger(state, m->renderingHandle()); })

int luaCameraFarPlane(lua_State* state) {
 LuaApi& api = luaApi();
 if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
  api.pushnumber(state, 192.0);
  return 1;
 }
 api.pushnumber(state, client::Minecraft::INSTANCE->gameRenderer->farPlaneBlocks());
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
                   });
 api.setfield(state, -2, "camera");
}

} // namespace net::minecraft::mod::runtime
