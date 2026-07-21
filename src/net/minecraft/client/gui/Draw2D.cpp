#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::gui::draw {
namespace {
constexpr float kAtlasTexel = 0.00390625f;
} // namespace
void appendQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float z) {
 tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z));
 tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z));
 tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z));
 tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z));
}
void appendColoredQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, int rgb, int alpha, float z) {
 tessellator.color(rgb, alpha);
 appendQuad(tessellator, x1, y1, x2, y2, z);
}
void appendVerticalGradientQuad(render::Tessellator& tessellator,
                                int x1,
                                int y1,
                                int x2,
                                int y2,
                                int topRgb,
                                int topAlpha,
                                int bottomRgb,
                                int bottomAlpha,
                                float z) {
 tessellator.color(bottomRgb, bottomAlpha);
 tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), static_cast<double>(z));
 tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), static_cast<double>(z));
 tessellator.color(topRgb, topAlpha);
 tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), static_cast<double>(z));
 tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(z));
}
void appendTexturedQuad(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float u1, float v1, float u2, float v2, float z) {
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    static_cast<double>(u1),
                    static_cast<double>(v2));
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    static_cast<double>(u2),
                    static_cast<double>(v2));
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    static_cast<double>(u2),
                    static_cast<double>(v1));
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    static_cast<double>(u1),
                    static_cast<double>(v1));
}
void appendAtlasQuad(render::Tessellator& tessellator, int x, int y, int u, int v, int width, int height, float z) {
 appendTexturedQuad(tessellator,
                    x,
                    y,
                    x + width,
                    y + height,
                    static_cast<float>(u) * kAtlasTexel,
                    static_cast<float>(v) * kAtlasTexel,
                    static_cast<float>(u + width) * kAtlasTexel,
                    static_cast<float>(v + height) * kAtlasTexel,
                    z);
}
void appendTiledPanel(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float scrollV, int rgb, float z) {
 constexpr float tile = 32.0f;
 tessellator.color(rgb);
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    0.0,
                    static_cast<double>(y2 + scrollV) / tile);
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    static_cast<double>(x2) / tile,
                    static_cast<double>(y2 + scrollV) / tile);
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    static_cast<double>(x2) / tile,
                    static_cast<double>(y1 + scrollV) / tile);
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    0.0,
                    static_cast<double>(y1 + scrollV) / tile);
}
void tiledPanel(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float scrollV, int rgb, float z) {
 tessellator.startQuads();
 appendTiledPanel(tessellator, x1, y1, x2, y2, scrollV, rgb, z);
 tessellator.draw();
}
void appendProgressBar(render::Tessellator& tessellator, int x, int y, int width, int height, int fillWidth) {
 tessellator.color(0x808080);
 appendQuad(tessellator, x, y, x + width, y + height);
 if(fillWidth > 0) {
  tessellator.color(0x80FF80);
  appendQuad(tessellator, x, y, x + fillWidth, y + height);
 }
}
void texturedAtlasQuads(render::Tessellator& tessellator, std::span<const AtlasRect> rects, float z) {
 if(rects.empty()) {
  return;
 }
 tessellator.startQuads();
 for(const AtlasRect& rect : rects) {
  appendAtlasQuad(tessellator, rect.x, rect.y, rect.u, rect.v, rect.w, rect.h, z);
 }
 tessellator.draw();
}
void quad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float z) {
 tessellator.startQuads();
 appendQuad(tessellator, x1, y1, x2, y2, z);
 tessellator.draw();
}
void coloredQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, int rgb, int alpha, float z) {
 tessellator.startQuads();
 appendColoredQuad(tessellator, x1, y1, x2, y2, rgb, alpha, z);
 tessellator.draw();
}
void verticalGradientQuad(render::Tessellator& tessellator,
                          int x1,
                          int y1,
                          int x2,
                          int y2,
                          int topRgb,
                          int topAlpha,
                          int bottomRgb,
                          int bottomAlpha,
                          float z) {
 tessellator.startQuads();
 appendVerticalGradientQuad(tessellator, x1, y1, x2, y2, topRgb, topAlpha, bottomRgb, bottomAlpha, z);
 tessellator.draw();
}
void texturedQuad(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float u1, float v1, float u2, float v2, float z) {
 tessellator.startQuads();
 appendTexturedQuad(tessellator, x1, y1, x2, y2, u1, v1, u2, v2, z);
 tessellator.draw();
}
void coloredTexturedQuad(render::Tessellator& tessellator,
                         int x1,
                         int y1,
                         int x2,
                         int y2,
                         float u1,
                         float v1,
                         float u2,
                         float v2,
                         int rgb,
                         int alpha,
                         float z) {
 tessellator.startQuads();
 tessellator.color(rgb, alpha);
 appendTexturedQuad(tessellator, x1, y1, x2, y2, u1, v1, u2, v2, z);
 tessellator.draw();
}
void verticalGradientTexturedQuad(render::Tessellator& tessellator,
                                  int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  float u1,
                                  float v1,
                                  float u2,
                                  float v2,
                                  int topRgb,
                                  int topAlpha,
                                  int bottomRgb,
                                  int bottomAlpha,
                                  float z) {
 tessellator.startQuads();
 appendVerticalGradientTexturedQuad(
     tessellator, x1, y1, x2, y2, u1, v1, u2, v2, topRgb, topAlpha, bottomRgb, bottomAlpha, z);
 tessellator.draw();
}
void appendVerticalGradientTexturedQuad(render::Tessellator& tessellator,
                                        int x1,
                                        int y1,
                                        int x2,
                                        int y2,
                                        float u1,
                                        float v1,
                                        float u2,
                                        float v2,
                                        int topRgb,
                                        int topAlpha,
                                        int bottomRgb,
                                        int bottomAlpha,
                                        float z) {
 tessellator.color(bottomRgb, bottomAlpha);
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    static_cast<double>(u1),
                    static_cast<double>(v2));
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y2),
                    static_cast<double>(z),
                    static_cast<double>(u2),
                    static_cast<double>(v2));
 tessellator.color(topRgb, topAlpha);
 tessellator.vertex(static_cast<double>(x2),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    static_cast<double>(u2),
                    static_cast<double>(v1));
 tessellator.vertex(static_cast<double>(x1),
                    static_cast<double>(y1),
                    static_cast<double>(z),
                    static_cast<double>(u1),
                    static_cast<double>(v1));
}
} // namespace net::minecraft::client::gui::draw
