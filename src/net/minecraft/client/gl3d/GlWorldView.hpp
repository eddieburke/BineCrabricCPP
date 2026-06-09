#pragma once

// Real 3D first-person renderer for the C++ client, replacing the isometric GDI
// painter. Fixed-function OpenGL (immediate mode), matching beta 1.7.3 style:
//   - perspective projection + a yaw/pitch/position camera driven by the player
//   - a simple per-frame chunk mesher that emits visible block faces
//   - terrain.png uploaded once to a GL texture, sampled per face
//
// This is a vertical slice: it draws the world the player stands in, in first
// person. Lighting/AO, display-list caching and survival HUD come on top.

#ifdef _WIN32

#include "net/minecraft/client/gl3d/GlContext.hpp"
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Gdiplus { class Bitmap; }

namespace net::minecraft::client::gl3d {

class GlWorldView {
public:
    void init(HWND hwnd)
    {
        context_.create(hwnd);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glClearColor(0.62f, 0.74f, 1.0f, 1.0f); // sky blue
    }

    [[nodiscard]] bool ready() const noexcept { return context_.valid(); }

    // Upload terrain.png (16x16 tile atlas, 256x256) into a GL texture. The
    // Gdiplus::Bitmap pixels are read out as BGRA and handed to glTexImage2D.
    void uploadTerrain(const Gdiplus::Bitmap& bitmap);

    // Render one frame from the player's eye.
    void render(const World& world, double camX, double camY, double camZ,
                float yaw, float pitch, int viewW, int viewH);

private:
    void setupCamera(double camX, double camY, double camZ, float yaw, float pitch, int viewW, int viewH);
    void drawChunks(const World& world, double camX, double camY, double camZ);
    void emitBlockFaces(const World& world, int wx, int wy, int wz, int blockId);
    [[nodiscard]] static std::array<float, 3> blockTint(const World& world, int x, int z, int blockId)
    {
        int rgb = 0xFFFFFF;
        if (blockId == 2) {
            rgb = color::world::GrassColors::getColor(world.getTemperature(x, z), world.getDownfall(x, z));
        } else if (blockId == 18) {
            rgb = color::world::FoliageColors::getColor(world.getTemperature(x, z), world.getDownfall(x, z));
        }
        return {
            static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
            static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
            static_cast<float>(rgb & 0xFF) / 255.0f
        };
    }

    [[nodiscard]] static bool isAir(const World& world, int x, int y, int z)
    {
        if (y < 0 || y >= Chunk::height) {
            return y >= Chunk::height; // above world = air, below = solid
        }
        return world.getBlockId(x, y, z) == 0;
    }

    GlContext context_;
    GLuint terrainTex_ = 0;
    int terrainW_ = 0;
    int terrainH_ = 0;
};

// ---------------------------------------------------------------------------
// terrain.png upload (Gdiplus dependency kept out of the header body via .inl)
// ---------------------------------------------------------------------------

inline void GlWorldView::render(const World& world, double camX, double camY, double camZ,
                                float yaw, float pitch, int viewW, int viewH)
{
    context_.makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera(camX, camY, camZ, yaw, pitch, viewW, viewH);
    drawChunks(world, camX, camY, camZ);
    context_.swap();
}

inline void GlWorldView::setupCamera(double camX, double camY, double camZ,
                                     float yaw, float pitch, int viewW, int viewH)
{
    glViewport(0, 0, viewW, viewH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const double aspect = viewH > 0 ? static_cast<double>(viewW) / viewH : 1.0;
    gluPerspective(70.0, aspect, 0.05, 256.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // Pitch then yaw, then translate world opposite the camera (Minecraft order).
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw + 180.0f, 0.0f, 1.0f, 0.0f);
    glTranslated(-camX, -camY, -camZ);
}

inline void GlWorldView::drawChunks(const World& world, double camX, double camY, double camZ)
{
    if (terrainTex_ != 0) {
        glBindTexture(GL_TEXTURE_2D, terrainTex_);
    }

    const int px = MathHelper::floor(static_cast<double>(camX));
    const int py = MathHelper::floor(static_cast<double>(camY));
    const int pz = MathHelper::floor(static_cast<double>(camZ));
    const int reach = 48; // blocks of horizontal view radius
    const int vReach = 48;

    glBegin(GL_QUADS);
    for (int x = px - reach; x <= px + reach; ++x) {
        for (int z = pz - reach; z <= pz + reach; ++z) {
            const int yLo = std::max(0, py - vReach);
            const int yHi = std::min(Chunk::height - 1, py + vReach);
            for (int y = yLo; y <= yHi; ++y) {
                const int id = world.getBlockId(x, y, z);
                if (id != 0) {
                    emitBlockFaces(world, x, y, z, id);
                }
            }
        }
    }
    glEnd();
}

// Atlas helper: terrain tile index -> UV rect in the 16x16 grid.
namespace detail {
inline void tileUv(int tile, float& u0, float& v0, float& u1, float& v1)
{
    const float cell = 1.0f / 16.0f;
    const int tx = tile % 16;
    const int ty = tile / 16;
    u0 = tx * cell;
    v0 = ty * cell;
    u1 = u0 + cell;
    v1 = v0 + cell;
}

// Minimal beta block-id -> top/side/bottom terrain tile mapping.
inline void blockTiles(int id, int& top, int& side, int& bottom)
{
    switch (id) {
    case 1:  top = side = bottom = 1;  break;            // stone
    case 2:  top = 0; side = 3; bottom = 2; break;        // grass
    case 3:  top = side = bottom = 2;  break;            // dirt
    case 4:  top = side = bottom = 16; break;            // cobblestone
    case 5:  top = side = bottom = 4;  break;            // planks
    case 7:  top = side = bottom = 17; break;            // bedrock
    case 12: top = side = bottom = 18; break;            // sand
    case 13: top = side = bottom = 19; break;            // gravel
    case 17: top = bottom = 21; side = 20; break;         // log
    case 18: top = side = bottom = 52; break;            // leaves
    case 1 + 7 + 1: break;                                // (placeholder)
    default: top = side = bottom = 1; break;             // fallback: stone
    }
}
} // namespace detail

inline void GlWorldView::emitBlockFaces(const World& world, int x, int y, int z, int id)
{
    int topTile = 1, sideTile = 1, bottomTile = 1;
    detail::blockTiles(id, topTile, sideTile, bottomTile);
    const auto tint = blockTint(world, x, z, id);

    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);
    const float fz = static_cast<float>(z);
    float u0, v0, u1, v1;

    // +Y (top)
    if (isAir(world, x, y + 1, z)) {
        glColor3f(tint[0], tint[1], tint[2]);
        detail::tileUv(topTile, u0, v0, u1, v1);
        glTexCoord2f(u0, v0); glVertex3f(fx,     fy + 1, fz);
        glTexCoord2f(u0, v1); glVertex3f(fx,     fy + 1, fz + 1);
        glTexCoord2f(u1, v1); glVertex3f(fx + 1, fy + 1, fz + 1);
        glTexCoord2f(u1, v0); glVertex3f(fx + 1, fy + 1, fz);
    }
    // -Y (bottom)
    if (isAir(world, x, y - 1, z)) {
        glColor3f(1.0f, 1.0f, 1.0f);
        detail::tileUv(bottomTile, u0, v0, u1, v1);
        glTexCoord2f(u0, v0); glVertex3f(fx,     fy, fz);
        glTexCoord2f(u1, v0); glVertex3f(fx + 1, fy, fz);
        glTexCoord2f(u1, v1); glVertex3f(fx + 1, fy, fz + 1);
        glTexCoord2f(u0, v1); glVertex3f(fx,     fy, fz + 1);
    }
    // -Z (north)
    if (isAir(world, x, y, z - 1)) {
        glColor3f(tint[0], tint[1], tint[2]);
        detail::tileUv(sideTile, u0, v0, u1, v1);
        glTexCoord2f(u1, v1); glVertex3f(fx,     fy,     fz);
        glTexCoord2f(u1, v0); glVertex3f(fx,     fy + 1, fz);
        glTexCoord2f(u0, v0); glVertex3f(fx + 1, fy + 1, fz);
        glTexCoord2f(u0, v1); glVertex3f(fx + 1, fy,     fz);
    }
    // +Z (south)
    if (isAir(world, x, y, z + 1)) {
        glColor3f(tint[0], tint[1], tint[2]);
        detail::tileUv(sideTile, u0, v0, u1, v1);
        glTexCoord2f(u0, v1); glVertex3f(fx,     fy,     fz + 1);
        glTexCoord2f(u1, v1); glVertex3f(fx + 1, fy,     fz + 1);
        glTexCoord2f(u1, v0); glVertex3f(fx + 1, fy + 1, fz + 1);
        glTexCoord2f(u0, v0); glVertex3f(fx,     fy + 1, fz + 1);
    }
    // -X (west)
    if (isAir(world, x - 1, y, z)) {
        glColor3f(tint[0], tint[1], tint[2]);
        detail::tileUv(sideTile, u0, v0, u1, v1);
        glTexCoord2f(u0, v1); glVertex3f(fx, fy,     fz);
        glTexCoord2f(u1, v1); glVertex3f(fx, fy,     fz + 1);
        glTexCoord2f(u1, v0); glVertex3f(fx, fy + 1, fz + 1);
        glTexCoord2f(u0, v0); glVertex3f(fx, fy + 1, fz);
    }
    // +X (east)
    if (isAir(world, x + 1, y, z)) {
        glColor3f(tint[0], tint[1], tint[2]);
        detail::tileUv(sideTile, u0, v0, u1, v1);
        glTexCoord2f(u1, v1); glVertex3f(fx + 1, fy,     fz);
        glTexCoord2f(u1, v0); glVertex3f(fx + 1, fy + 1, fz);
        glTexCoord2f(u0, v0); glVertex3f(fx + 1, fy + 1, fz + 1);
        glTexCoord2f(u0, v1); glVertex3f(fx + 1, fy,     fz + 1);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
}

} // namespace net::minecraft::client::gl3d

#endif // _WIN32
