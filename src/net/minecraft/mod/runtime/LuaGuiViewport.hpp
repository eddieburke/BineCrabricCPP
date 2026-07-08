#pragma once
struct lua_State;

namespace net::minecraft::mod::runtime {
struct LuaGuiViewportRay {
    double originX = 0.0;
    double originY = 0.0;
    double originZ = 0.0;
    double directionX = 0.0;
    double directionY = 0.0;
    double directionZ = 0.0;
};

struct LuaGuiViewportParams {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int guiWidth = 0;
    int guiHeight = 0;
    float yawDeg = 0.0f;
    float pitchDeg = 0.0f;
    float camDist = 2.05f;
    float fovDeg = 40.0f;
    float clearR = 0.11f;
    float clearG = 0.13f;
    float clearB = 0.17f;
    float clearA = 1.0f;
};

[[nodiscard]] bool luaGuiViewportActive() noexcept;
[[nodiscard]] bool beginLuaGuiViewport(const LuaGuiViewportParams& params);
void endLuaGuiViewport();
[[nodiscard]] bool drawLuaGui3d(lua_State* state, int specIndex);
[[nodiscard]] bool computeLuaGuiViewportRay(const LuaGuiViewportParams& params,
                                            int mouseX,
                                            int mouseY,
                                            LuaGuiViewportRay& outRay);
void installLuaGuiViewportBindings(lua_State* state);
}  // namespace net::minecraft::mod::runtime
