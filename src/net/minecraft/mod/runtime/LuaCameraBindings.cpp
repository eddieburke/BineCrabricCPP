#include "net/minecraft/mod/runtime/LuaCameraBindings.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderTargetManager.hpp"

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;

namespace {
net::minecraft::client::render::RenderTargetManager* renderTargets() {
    if (client::Minecraft::INSTANCE == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr) {
        return nullptr;
    }
    return &client::Minecraft::INSTANCE->gameRenderer->renderTargets();
}

int luaCameraCreate(lua_State* state) {
    LuaApi& api = luaApi();
    auto* manager = renderTargets();
    if (manager == nullptr || api.gettop(state) < 2) {
        api.pushinteger(state, -1);
        return 1;
    }
    const int width = static_cast<int>(api.tointegerx(state, 1, nullptr));
    const int height = static_cast<int>(api.tointegerx(state, 2, nullptr));
    api.pushinteger(state, manager->create(width, height));
    return 1;
}

int luaCameraDestroy(lua_State* state) {
    LuaApi& api = luaApi();
    auto* manager = renderTargets();
    if (manager == nullptr || api.gettop(state) < 1) {
        api.pushboolean(state, 0);
        return 1;
    }
    const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
    api.pushboolean(state, manager->destroy(handle) ? 1 : 0);
    return 1;
}

int luaCameraRender(lua_State* state) {
    LuaApi& api = luaApi();
    auto* manager = renderTargets();
    if (manager == nullptr || client::Minecraft::INSTANCE->gameRenderer == nullptr || api.gettop(state) < 8) {
        api.pushboolean(state, 0);
        return 1;
    }
    const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
    const double x = api.tonumberx(state, 2, nullptr);
    const double y = api.tonumberx(state, 3, nullptr);
    const double z = api.tonumberx(state, 4, nullptr);
    const float yaw = static_cast<float>(api.tonumberx(state, 5, nullptr));
    const float pitch = static_cast<float>(api.tonumberx(state, 6, nullptr));
    const float roll = static_cast<float>(api.tonumberx(state, 7, nullptr));
    const float fov = static_cast<float>(api.tonumberx(state, 8, nullptr));
    const float tickDelta = api.gettop(state) >= 9 ? static_cast<float>(api.tonumberx(state, 9, nullptr)) : 1.0f;
    const bool ok =
        manager->render(handle, *client::Minecraft::INSTANCE->gameRenderer, tickDelta, x, y, z, yaw, pitch, roll, fov);
    api.pushboolean(state, ok ? 1 : 0);
    return 1;
}

int luaCameraTexture(lua_State* state) {
    LuaApi& api = luaApi();
    auto* manager = renderTargets();
    if (manager == nullptr || api.gettop(state) < 1) {
        api.pushinteger(state, -1);
        return 1;
    }
    const int handle = static_cast<int>(api.tointegerx(state, 1, nullptr));
    api.pushinteger(state, manager->textureId(handle));
    return 1;
}

int luaCameraRendering(lua_State* state) {
    LuaApi& api = luaApi();
    auto* manager = renderTargets();
    api.pushinteger(state, manager == nullptr ? -1 : manager->renderingHandle());
    return 1;
}
}  // namespace

void installCameraApi(lua_State* state) {
    LuaApi& api = luaApi();
    pushFunctionTable(state,
                      {
                          {"create", luaCameraCreate},
                          {"destroy", luaCameraDestroy},
                          {"render", luaCameraRender},
                          {"texture", luaCameraTexture},
                          {"rendering", luaCameraRendering},
                      });
    api.setfield(state, -2, "camera");
}
}  // namespace net::minecraft::mod::runtime
