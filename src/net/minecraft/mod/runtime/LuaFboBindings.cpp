#include "net/minecraft/mod/runtime/LuaFboBindings.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/Framebuffer.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
net::minecraft::client::render::FramebufferManager* fboManager() {
  if(client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
    return nullptr;
  }
  return &client::Minecraft::INSTANCE->gameRenderer->fbos();
}
#define LUA_FBO_IMPL(FUNC_NAME, MIN_ARGS, FAIL_RET, BODY) \
  int FUNC_NAME(lua_State* state) {                       \
    LuaApi& api = luaApi();                               \
    auto* m = fboManager();                               \
    if(!m || api.gettop(state) < MIN_ARGS) {              \
      FAIL_RET;                                           \
      return 1;                                           \
    }                                                     \
    BODY return 1;                                        \
  }
LUA_FBO_IMPL(luaFboCreate, 2, api.pushinteger(state, -1), {
  const int w = static_cast<int>(api.tointegerx(state, 1, nullptr));
  const int h = static_cast<int>(api.tointegerx(state, 2, nullptr));
  const int colorCount = api.gettop(state) >= 3 ? static_cast<int>(api.tointegerx(state, 3, nullptr)) : 1;
  const bool useDepthTex = api.gettop(state) >= 4 ? api.toboolean(state, 4) != 0 : false;
  api.pushinteger(state, m->create(w, h, colorCount, useDepthTex));
})
LUA_FBO_IMPL(luaFboCreateDisplaySize, 0, api.pushinteger(state, -1), {
  if(client::Minecraft::INSTANCE == nullptr) {
    api.pushinteger(state, -1);
    return 1;
  }
  const int w = client::Minecraft::INSTANCE->displayWidth;
  const int h = client::Minecraft::INSTANCE->displayHeight;
  const int colorCount = api.gettop(state) >= 1 ? static_cast<int>(api.tointegerx(state, 1, nullptr)) : 1;
  const bool useDepthTex = api.gettop(state) >= 2 ? api.toboolean(state, 2) != 0 : false;
  api.pushinteger(state, m->create(w > 0 ? w : 1, h > 0 ? h : 1, colorCount, useDepthTex));
})
LUA_FBO_IMPL(luaFboDestroy, 1, api.pushboolean(state, 0), {
  api.pushboolean(state, m->destroy(static_cast<int>(api.tointegerx(state, 1, nullptr))) ? 1 : 0);
})
LUA_FBO_IMPL(luaFboResize, 3, api.pushboolean(state, 0), {
  api.pushboolean(state,
                  m->resize(static_cast<int>(api.tointegerx(state, 1, nullptr)),
                            static_cast<int>(api.tointegerx(state, 2, nullptr)),
                            static_cast<int>(api.tointegerx(state, 3, nullptr)))
                      ? 1
                      : 0);
})
LUA_FBO_IMPL(luaFboBind, 1, api.pushboolean(state, 0), {
  api.pushboolean(state, m->bind(static_cast<int>(api.tointegerx(state, 1, nullptr))) ? 1 : 0);
})
LUA_FBO_IMPL(luaFboUnbind, 0, api.pushboolean(state, 0), {
  m->unbind();
  api.pushboolean(state, 1);
})
LUA_FBO_IMPL(luaFboTexture, 1, api.pushinteger(state, -1), {
  const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
  const int attachmentIndex = api.gettop(state) >= 2 ? static_cast<int>(api.tointegerx(state, 2, nullptr)) : 0;
  api.pushinteger(state, m->textureId(handle, attachmentIndex));
})
LUA_FBO_IMPL(luaFboDepthTexture, 1, api.pushinteger(state, -1), {
  api.pushinteger(state, m->depthTextureId(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})
LUA_FBO_IMPL(luaFboWidth, 1, api.pushinteger(state, 0), {
  api.pushinteger(state, m->width(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})
LUA_FBO_IMPL(luaFboHeight, 1, api.pushinteger(state, 0), {
  api.pushinteger(state, m->height(static_cast<int>(api.tointegerx(state, 1, nullptr))));
})
LUA_FBO_IMPL(luaFboBound, 0, api.pushinteger(state, -1), {
  api.pushinteger(state, m->renderingHandle());
})
int luaFboFullscreen(lua_State* state) {
  LuaApi& api = luaApi();
  client::Minecraft* minecraft = client::Minecraft::INSTANCE;
  const int width = api.gettop(state) >= 1 ? static_cast<int>(api.tointegerx(state, 1, nullptr))
                                           : (minecraft != nullptr ? minecraft->displayWidth : 0);
  const int height = api.gettop(state) >= 2 ? static_cast<int>(api.tointegerx(state, 2, nullptr))
                                            : (minecraft != nullptr ? minecraft->displayHeight : 0);
  if(width <= 0 || height <= 0) {
    api.pushboolean(state, 0);
    return 1;
  }
  int viewport[4]{0, 0, width, height};
  int matrixMode = client::gl::matrix_::ModelView;
  float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
  client::gl::getIntegerv(client::gl::query::Viewport, viewport);
  client::gl::getIntegerv(0x0BA0, &matrixMode);
  client::gl::getFloatv(client::gl::query::CurrentColor, color);
  const client::gl::CapScope caps{client::gl::cap::DepthTest,
                                  client::gl::cap::CullFace,
                                  client::gl::cap::Blend,
                                  client::gl::cap::AlphaTest,
                                  client::gl::cap::Fog,
                                  client::gl::cap::Lighting,
                                  client::gl::cap::Texture2D};
  const client::gl::DepthMaskScope depthMask;
  client::gl::viewport(0, 0, width, height);
  client::gl::setCap(client::gl::cap::DepthTest, false);
  client::gl::setCap(client::gl::cap::CullFace, false);
  client::gl::setCap(client::gl::cap::Blend, false);
  client::gl::setCap(client::gl::cap::AlphaTest, false);
  client::gl::setCap(client::gl::cap::Fog, false);
  client::gl::setCap(client::gl::cap::Lighting, false);
  client::gl::setCap(client::gl::cap::Texture2D, true);
  client::gl::depthMask(false);
  client::gl::activeTexture(client::gl::tex::Texture0);
  client::gl::matrixMode(client::gl::matrix_::Projection);
  client::gl::pushMatrix();
  client::gl::loadIdentity();
  client::gl::matrixMode(client::gl::matrix_::ModelView);
  client::gl::pushMatrix();
  client::gl::loadIdentity();
  client::gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  client::gl::begin(client::gl::prim::Quads);
  client::gl::texCoord2d(0.0, 0.0);
  client::gl::vertex3d(-1.0, -1.0, 0.0);
  client::gl::texCoord2d(1.0, 0.0);
  client::gl::vertex3d(1.0, -1.0, 0.0);
  client::gl::texCoord2d(1.0, 1.0);
  client::gl::vertex3d(1.0, 1.0, 0.0);
  client::gl::texCoord2d(0.0, 1.0);
  client::gl::vertex3d(-1.0, 1.0, 0.0);
  client::gl::end();
  client::gl::popMatrix();
  client::gl::matrixMode(client::gl::matrix_::Projection);
  client::gl::popMatrix();
  client::gl::matrixMode(matrixMode);
  client::gl::color4f(color[0], color[1], color[2], color[3]);
  client::gl::viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  api.pushboolean(state, 1);
  return 1;
}
} // namespace
void installFboApi(lua_State* state) {
  LuaApi& api = luaApi();
  // Offscreen targets for custom passes / future shaders. Separate from minecraft.camera
  // (world-to-texture viewfinder targets on GameRenderer::renderTargets).
  pushFunctionTable(state,
                    {
                        {"create", luaFboCreate},
                        {"create_display_size", luaFboCreateDisplaySize},
                        {"destroy", luaFboDestroy},
                        {"resize", luaFboResize},
                        {"bind", luaFboBind},
                        {"unbind", luaFboUnbind},
                        {"texture", luaFboTexture},
                        {"depth_texture", luaFboDepthTexture},
                        {"width", luaFboWidth},
                        {"height", luaFboHeight},
                        {"bound", luaFboBound},
                        {"fullscreen", luaFboFullscreen},
                    });
  api.setfield(state, -2, "fbo");
}
} // namespace net::minecraft::mod::runtime
