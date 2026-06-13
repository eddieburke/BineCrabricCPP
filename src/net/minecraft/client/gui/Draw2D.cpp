#include "net/minecraft/client/gui/Draw2D.hpp"

#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::gui::draw {

void quad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float z)
{
    tessellator.startQuads();
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z));
    tessellator.draw();
}

void coloredQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    int rgb, int alpha, float z)
{
    tessellator.startQuads();
    tessellator.color(rgb, alpha);
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z));
    tessellator.draw();
}

void verticalGradientQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    int topRgb, int topAlpha, int bottomRgb, int bottomAlpha, float z)
{
    tessellator.startQuads();
    tessellator.color(bottomRgb, bottomAlpha);
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z));
    tessellator.color(topRgb, topAlpha);
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z));
    tessellator.draw();
}

void texturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2, float z)
{
    tessellator.startQuads();
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v2));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v2));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v1));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v1));
    tessellator.draw();
}

void coloredTexturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2, int rgb, int alpha, float z)
{
    tessellator.startQuads();
    tessellator.color(rgb, alpha);
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v2));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v2));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v1));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v1));
    tessellator.draw();
}

void verticalGradientTexturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2,
    int topRgb, int topAlpha, int bottomRgb, int bottomAlpha, float z)
{
    tessellator.startQuads();
    tessellator.color(bottomRgb, bottomAlpha);
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v2));
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v2));
    tessellator.color(topRgb, topAlpha);
    tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u2), static_cast<double>(v1));
    tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z),
        static_cast<double>(u1), static_cast<double>(v1));
    tessellator.draw();
}

} // namespace net::minecraft::client::gui::draw
