#include "net/minecraft/client/render/atmosphere/SkyDomeRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::render::atmosphere {

namespace {
constexpr int kGlCompile = 0x1300;
} // namespace

void SkyDomeRenderer::build()
{
    if (lightList_ == 0) {
        lightList_ = gl::GL11::glGenLists(1);
    }
    if (darkList_ == 0) {
        darkList_ = gl::GL11::glGenLists(1);
    }

    Tessellator& tessellator = INSTANCE;

    constexpr int tile = 64;
    constexpr int tiles = 256 / tile + 2;
    constexpr float height = 16.0f;

    gl::GL11::glNewList(lightList_, kGlCompile);
    for (int x = -tile * tiles; x <= tile * tiles; x += tile) {
        for (int z = -tile * tiles; z <= tile * tiles; z += tile) {
            tessellator.startQuads();
            tessellator.vertex(x + 0, height, z + 0);
            tessellator.vertex(x + tile, height, z + 0);
            tessellator.vertex(x + tile, height, z + tile);
            tessellator.vertex(x + 0, height, z + tile);
            tessellator.draw();
        }
    }
    gl::GL11::glEndList();

    constexpr float bottom = -16.0f;
    gl::GL11::glNewList(darkList_, kGlCompile);
    tessellator.startQuads();
    for (int x = -tile * tiles; x <= tile * tiles; x += tile) {
        for (int z = -tile * tiles; z <= tile * tiles; z += tile) {
            tessellator.vertex(x + tile, bottom, z + 0);
            tessellator.vertex(x + 0, bottom, z + 0);
            tessellator.vertex(x + 0, bottom, z + tile);
            tessellator.vertex(x + tile, bottom, z + tile);
        }
    }
    tessellator.draw();
    gl::GL11::glEndList();
}

void SkyDomeRenderer::drawLight() const
{
    if (lightList_ > 0) {
        gl::GL11::glCallList(lightList_);
    }
}

void SkyDomeRenderer::drawDark(float r, float g, float b) const
{
    if (darkList_ == 0) {
        return;
    }
    gl::GL11::glColor3f(r, g, b);
    gl::GL11::glCallList(darkList_);
}

void SkyDomeRenderer::release()
{
    if (lightList_ > 0) {
        gl::GL11::glDeleteLists(lightList_, 1);
        lightList_ = 0;
    }
    if (darkList_ > 0) {
        gl::GL11::glDeleteLists(darkList_, 1);
        darkList_ = 0;
    }
}

} // namespace net::minecraft::client::render::atmosphere
