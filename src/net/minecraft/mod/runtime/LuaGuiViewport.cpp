#include "net/minecraft/mod/runtime/LuaGuiViewport.hpp"

#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstring>
#endif
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
namespace {
thread_local int g_luaGuiViewportDepth = 0;
thread_local int g_savedViewport[4] = {0, 0, 0, 0};
thread_local LuaGuiViewportParams g_activeViewport{};

[[nodiscard]] float clampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}
#ifdef MINECRAFT_GL_REAL
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
    ::gluPerspective(static_cast<GLdouble>(fovyDeg),
                     static_cast<GLdouble>(aspect),
                     static_cast<GLdouble>(zNear),
                     static_cast<GLdouble>(zFar));
}
#else
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
    (void) fovyDeg;
    (void) aspect;
    (void) zNear;
    (void) zFar;
}
#endif
void applyViewportRotation(float yawDeg, float pitchDeg) {
    client::gl::rotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    client::gl::rotatef(yawDeg, 0.0f, 1.0f, 0.0f);
}

void setupViewportModelView(float yawDeg, float pitchDeg, float camDist) {
    client::gl::loadIdentity();
    client::gl::translatef(0.0f, 0.0f, -camDist);
    applyViewportRotation(yawDeg, pitchDeg);
}

[[nodiscard]] bool computeFramebufferViewport(
    const LuaGuiViewportParams& params, int displayWidth, int displayHeight, int& vx, int& vy, int& vw, int& vh) {
    if (params.width < 2 || params.height < 2 || params.guiWidth < 4 || params.guiHeight < 4 || displayWidth < 4 ||
        displayHeight < 4) {
        return false;
    }
    vx = params.x * displayWidth / params.guiWidth;
    vw = params.width * displayWidth / params.guiWidth;
    vh = params.height * displayHeight / params.guiHeight;
    vy = displayHeight - (params.y + params.height) * displayHeight / params.guiHeight;
    return vw >= 2 && vh >= 2;
}

[[nodiscard]] bool guiMouseToFrame(int mouseX,
                                   int mouseY,
                                   int guiWidth,
                                   int guiHeight,
                                   int displayWidth,
                                   int displayHeight,
                                   double& frameX,
                                   double& frameY) {
    if (guiWidth < 1 || guiHeight < 1 || displayWidth < 1 || displayHeight < 1) {
        return false;
    }
    frameX = static_cast<double>(mouseX) * static_cast<double>(displayWidth) / static_cast<double>(guiWidth);
    frameY = static_cast<double>(displayHeight) -
             (static_cast<double>(mouseY) + 0.5) * static_cast<double>(displayHeight) / static_cast<double>(guiHeight);
    return true;
}

[[nodiscard]] int drawModeFromName(const char* mode) {
    if (mode == nullptr) {
        return -1;
    }
    if (std::strcmp(mode, "lines") == 0) {
        return client::gl::prim::Lines;
    }
    if (std::strcmp(mode, "line_strip") == 0) {
        return client::gl::prim::LineStrip;
    }
    if (std::strcmp(mode, "line_loop") == 0) {
        return client::gl::prim::LineLoop;
    }
    if (std::strcmp(mode, "quads") == 0) {
        return client::gl::prim::Quads;
    }
    if (std::strcmp(mode, "quad_strip") == 0) {
        return client::gl::prim::QuadStrip;
    }
    if (std::strcmp(mode, "points") == 0) {
        return client::gl::prim::Points;
    }
    if (std::strcmp(mode, "triangles") == 0) {
        return client::gl::prim::Triangles;
    }
    return -1;
}

void readColor(lua_State* state, int tableIndex, float& r, float& g, float& b, float& a) {
    const int color = luaIntField(state, tableIndex, "color", -1);
    if (color >= 0) {
        const std::uint32_t argb = static_cast<std::uint32_t>(color);
        a = static_cast<float>((argb >> 24U) & 0xFFU) / 255.0f;
        r = static_cast<float>((argb >> 16U) & 0xFFU) / 255.0f;
        g = static_cast<float>((argb >> 8U) & 0xFFU) / 255.0f;
        b = static_cast<float>(argb & 0xFFU) / 255.0f;
        return;
    }
    r = std::clamp(luaFloatField(state, tableIndex, "r", 1.0f), 0.0f, 1.0f);
    g = std::clamp(luaFloatField(state, tableIndex, "g", 1.0f), 0.0f, 1.0f);
    b = std::clamp(luaFloatField(state, tableIndex, "b", 1.0f), 0.0f, 1.0f);
    a = std::clamp(luaFloatField(state, tableIndex, "a", 1.0f), 0.0f, 1.0f);
}

LuaGuiViewportParams readViewportParams(lua_State* state, int tableIndex) {
    client::Minecraft* client = client::Minecraft::INSTANCE;
    LuaGuiViewportParams params{};
    params.x = static_cast<int>(luaFloatField(state, tableIndex, "x", 0.0f));
    params.y = static_cast<int>(luaFloatField(state, tableIndex, "y", 0.0f));
    const int square = static_cast<int>(luaFloatField(state, tableIndex, "size", 0.0f));
    const float defaultWidth = square > 0 ? static_cast<float>(square) : 0.0f;
    params.width = static_cast<int>(luaFloatField(state, tableIndex, "width", defaultWidth));
    params.height = static_cast<int>(luaFloatField(state, tableIndex, "height", defaultWidth));
    params.guiWidth = static_cast<int>(luaFloatField(
        state, tableIndex, "gui_width", client != nullptr ? static_cast<float>(client->displayWidth) : 0.0f));
    params.guiHeight = static_cast<int>(luaFloatField(
        state, tableIndex, "gui_height", client != nullptr ? static_cast<float>(client->displayHeight) : 0.0f));
    params.yawDeg = luaFloatField(state, tableIndex, "yaw_deg", 0.0f);
    params.pitchDeg = luaFloatField(state, tableIndex, "pitch_deg", 0.0f);
    params.camDist = luaFloatField(state, tableIndex, "distance", luaFloatField(state, tableIndex, "cam_dist", 2.05f));
    params.fovDeg = luaFloatField(state, tableIndex, "fov_deg", 40.0f);
    const int clearColor = luaIntField(state, tableIndex, "clear_color", -1);
    if (clearColor >= 0) {
        const std::uint32_t argb = static_cast<std::uint32_t>(clearColor);
        params.clearA = static_cast<float>((argb >> 24U) & 0xFFU) / 255.0f;
        params.clearR = static_cast<float>((argb >> 16U) & 0xFFU) / 255.0f;
        params.clearG = static_cast<float>((argb >> 8U) & 0xFFU) / 255.0f;
        params.clearB = static_cast<float>(argb & 0xFFU) / 255.0f;
    } else {
        params.clearR = std::clamp(luaFloatField(state, tableIndex, "clear_r", params.clearR), 0.0f, 1.0f);
        params.clearG = std::clamp(luaFloatField(state, tableIndex, "clear_g", params.clearG), 0.0f, 1.0f);
        params.clearB = std::clamp(luaFloatField(state, tableIndex, "clear_b", params.clearB), 0.0f, 1.0f);
        params.clearA = std::clamp(luaFloatField(state, tableIndex, "clear_a", params.clearA), 0.0f, 1.0f);
    }
    return params;
}

void pushVec3(lua_State* state, const char* key, double x, double y, double z) {
    LuaApi& api = luaApi();
    api.createtable(state, 0, 3);
    api.pushnumber(state, x);
    api.setfield(state, -2, "x");
    api.pushnumber(state, y);
    api.setfield(state, -2, "y");
    api.pushnumber(state, z);
    api.setfield(state, -2, "z");
    api.setfield(state, -2, key);
}

int luaGuiBeginViewport(lua_State* state) {
    if (!luaGuiDrawActive() || luaApi().type(state, 1) != kLuaTTable ||
        !beginLuaGuiViewport(readViewportParams(state, 1))) {
        return 0;
    }
    return 0;
}

int luaGuiEndViewport(lua_State* state) {
    (void) state;
    endLuaGuiViewport();
    return 0;
}

int luaGuiDraw3d(lua_State* state) {
    if (!luaGuiDrawActive() || !drawLuaGui3d(state, 1)) {
        return 0;
    }
    return 0;
}

int luaGuiUnproject(lua_State* state) {
    LuaApi& api = luaApi();
    if (api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
        api.pushnil(state);
        return 1;
    }
    LuaGuiViewportParams params = readViewportParams(state, 1);
    const int mouseX = static_cast<int>(luaFloatField(state, 1, "mouse_x", 0.0f));
    const int mouseY = static_cast<int>(luaFloatField(state, 1, "mouse_y", 0.0f));
    LuaGuiViewportRay ray{};
    if (!computeLuaGuiViewportRay(params, mouseX, mouseY, ray)) {
        api.pushnil(state);
        return 1;
    }
    api.createtable(state, 0, 2);
    pushVec3(state, "origin", ray.originX, ray.originY, ray.originZ);
    pushVec3(state, "direction", ray.directionX, ray.directionY, ray.directionZ);
    return 1;
}
}  // namespace

bool luaGuiViewportActive() noexcept {
    return g_luaGuiViewportDepth > 0;
}

bool beginLuaGuiViewport(const LuaGuiViewportParams& params) {
    if (!luaGuiDrawActive()) {
        return false;
    }
    client::Minecraft* client = client::Minecraft::INSTANCE;
    if (client == nullptr) {
        return false;
    }
    client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
    if (tess.drawing()) {
        tess.draw();
    }
    int vx = 0;
    int vy = 0;
    int vw = 0;
    int vh = 0;
    if (!computeFramebufferViewport(params, client->displayWidth, client->displayHeight, vx, vy, vw, vh)) {
        return false;
    }
    g_activeViewport = params;
    ++g_luaGuiViewportDepth;
    client::gl::getIntegerv(client::gl::query::Viewport, g_savedViewport);
    const client::gl::preset::LuaGuiViewportDraw viewportCaps;
    client::gl::scissor(vx, vy, vw, vh);
    client::gl::viewport(vx, vy, vw, vh);
    client::gl::clearColor(params.clearR, params.clearG, params.clearB, params.clearA);
    client::gl::clearDepth(1.0);
    client::gl::clear(client::gl::attrib::DepthBufferBit | client::gl::attrib::ColorBufferBit);
    client::gl::matrixMode(client::gl::matrix_::Projection);
    client::gl::pushMatrix();
    client::gl::loadIdentity();
    const float fovDeg = clampFloat(params.fovDeg, 10.0f, 120.0f);
    gluPerspectiveFov(fovDeg, static_cast<float>(vw) / static_cast<float>(vh), 0.05f, 12.0f);
    client::gl::matrixMode(client::gl::matrix_::ModelView);
    client::gl::pushMatrix();
    const float camDist = clampFloat(params.camDist, 1.5f, 6.0f);
    setupViewportModelView(params.yawDeg, params.pitchDeg, camDist);
    return true;
}

void endLuaGuiViewport() {
    if (g_luaGuiViewportDepth <= 0) {
        return;
    }
    --g_luaGuiViewportDepth;
    client::gl::popMatrix();
    client::gl::matrixMode(client::gl::matrix_::Projection);
    client::gl::popMatrix();
    client::gl::matrixMode(client::gl::matrix_::ModelView);
    client::gl::viewport(g_savedViewport[0], g_savedViewport[1], g_savedViewport[2], g_savedViewport[3]);
}

bool drawLuaGui3d(lua_State* state, int specIndex) {
    if (!luaGuiViewportActive()) {
        return false;
    }
    LuaApi& api = luaApi();
    if (api.type(state, specIndex) != kLuaTTable) {
        return false;
    }
    const std::string modeName = luaStringField(state, specIndex, "mode", "");
    const int mode = drawModeFromName(modeName.c_str());
    if (mode < 0) {
        return false;
    }
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
    readColor(state, specIndex, r, g, b, a);
    const float lineWidth = std::clamp(luaFloatField(state, specIndex, "line_width", 1.0f), 0.5f, 8.0f);
    const float pointSize = std::clamp(luaFloatField(state, specIndex, "point_size", 1.0f), 1.0f, 16.0f);
    api.getfield(state, specIndex, "vertices");
    if (api.type(state, -1) != kLuaTTable) {
        pop(state, 1);
        return false;
    }
    const int verticesIndex = api.gettop(state);
    const std::size_t vertexCount = api.rawlen(state, verticesIndex);
    if (vertexCount == 0) {
        pop(state, 1);
        return false;
    }
    client::gl::color4f(r, g, b, a);
    if (mode == client::gl::prim::Points) {
        client::gl::pointSize(pointSize);
    } else {
        client::gl::lineWidth(lineWidth);
    }
    client::gl::begin(mode);
    for (std::size_t index = 1; index <= vertexCount; ++index) {
        api.rawgeti(state, verticesIndex, static_cast<long long>(index));
        const int vertexIndex = api.gettop(state);
        const double x = luaFloatField(state, vertexIndex, "x", luaFloatAt(state, vertexIndex, 1, 0.0f));
        const double y = luaFloatField(state, vertexIndex, "y", luaFloatAt(state, vertexIndex, 2, 0.0f));
        const double z = luaFloatField(state, vertexIndex, "z", luaFloatAt(state, vertexIndex, 3, 0.0f));
        client::gl::vertex3d(x, y, z);
        pop(state, 1);
    }
    client::gl::end();
    if (mode == client::gl::prim::Points) {
        client::gl::pointSize(1.0f);
    } else {
        client::gl::lineWidth(1.0f);
    }
    pop(state, 1);
    return true;
}

bool computeLuaGuiViewportRay(const LuaGuiViewportParams& params, int mouseX, int mouseY, LuaGuiViewportRay& outRay) {
    client::Minecraft* client = client::Minecraft::INSTANCE;
    if (client == nullptr) {
        return false;
    }
    int vx = 0;
    int vy = 0;
    int vw = 0;
    int vh = 0;
    if (!computeFramebufferViewport(params, client->displayWidth, client->displayHeight, vx, vy, vw, vh)) {
        return false;
    }
    double frameX = 0.0;
    double frameY = 0.0;
    if (!guiMouseToFrame(mouseX,
                         mouseY,
                         params.guiWidth,
                         params.guiHeight,
                         client->displayWidth,
                         client->displayHeight,
                         frameX,
                         frameY)) {
        return false;
    }
    if (frameX < static_cast<double>(vx) || frameX >= static_cast<double>(vx + vw) ||
        frameY < static_cast<double>(vy) || frameY >= static_cast<double>(vy + vh)) {
        return false;
    }
    const float camDist = clampFloat(params.camDist, 1.5f, 6.0f);
    const float fovDeg = clampFloat(params.fovDeg, 10.0f, 120.0f);
#ifdef MINECRAFT_GL_REAL
    GLint viewport[4] = {vx, vy, vw, vh};
    GLdouble modelview[16] = {};
    GLdouble projection[16] = {};
    GLdouble nearX = 0.0;
    GLdouble nearY = 0.0;
    GLdouble nearZ = 0.0;
    GLdouble farX = 0.0;
    GLdouble farY = 0.0;
    GLdouble farZ = 0.0;
    client::gl::matrixMode(client::gl::matrix_::Projection);
    client::gl::pushMatrix();
    client::gl::loadIdentity();
    gluPerspectiveFov(fovDeg, static_cast<float>(vw) / static_cast<float>(vh), 0.05f, 12.0f);
    ::glGetDoublev(client::gl::matrix_::ProjectionMatrix, projection);
    client::gl::popMatrix();
    client::gl::matrixMode(client::gl::matrix_::ModelView);
    client::gl::pushMatrix();
    setupViewportModelView(params.yawDeg, params.pitchDeg, camDist);
    ::glGetDoublev(client::gl::matrix_::ModelViewMatrix, modelview);
    client::gl::popMatrix();
    client::gl::matrixMode(client::gl::matrix_::ModelView);
    if (::gluUnProject(frameX, frameY, 0.0, modelview, projection, viewport, &nearX, &nearY, &nearZ) == GL_FALSE) {
        return false;
    }
    if (::gluUnProject(frameX, frameY, 1.0, modelview, projection, viewport, &farX, &farY, &farZ) == GL_FALSE) {
        return false;
    }
    outRay.originX = nearX;
    outRay.originY = nearY;
    outRay.originZ = nearZ;
    outRay.directionX = farX - nearX;
    outRay.directionY = farY - nearY;
    outRay.directionZ = farZ - nearZ;
    return true;
#else
    (void) camDist;
    (void) fovDeg;
    return false;
#endif
}

void installLuaGuiViewportBindings(lua_State* state) {
    LuaApi& api = luaApi();
    const int guiTable = api.gettop(state);
    api.pushcclosure(state, luaGuiBeginViewport, 0);
    api.setfield(state, guiTable, "begin_3d");
    api.pushcclosure(state, luaGuiEndViewport, 0);
    api.setfield(state, guiTable, "end_3d");
    api.pushcclosure(state, luaGuiDraw3d, 0);
    api.setfield(state, guiTable, "draw_3d");
    api.pushcclosure(state, luaGuiUnproject, 0);
    api.setfield(state, guiTable, "unproject");
}
#else
bool luaGuiViewportActive() noexcept {
    return false;
}

bool beginLuaGuiViewport(const LuaGuiViewportParams&) {
    return false;
}

void endLuaGuiViewport() {
}

bool drawLuaGui3d(lua_State*, int) {
    return false;
}

bool computeLuaGuiViewportRay(const LuaGuiViewportParams&, int, int, LuaGuiViewportRay&) {
    return false;
}

void installLuaGuiViewportBindings(lua_State* state) {
    (void) state;
}
#endif
}  // namespace net::minecraft::mod::runtime
