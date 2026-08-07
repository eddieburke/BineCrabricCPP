#pragma once
#include <cstddef>
namespace net::minecraft::client::render {
class Tessellator;
}
namespace net::minecraft::client::gui::draw {
struct AtlasRect {
 int x = 0;
 int y = 0;
 int u = 0;
 int v = 0;
 int w = 0;
 int h = 0;
};
void appendQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float z = 0.0f);
void appendColoredQuad(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, int rgb, int alpha = 255, float z = 0.0f);
void appendVerticalGradientQuad(render::Tessellator& tessellator,
                                int x1,
                                int y1,
                                int x2,
                                int y2,
                                int topRgb,
                                int topAlpha,
                                int bottomRgb,
                                int bottomAlpha,
                                float z = 0.0f);
void appendTexturedQuad(render::Tessellator& tessellator,
                        int x1,
                        int y1,
                        int x2,
                        int y2,
                        float u1,
                        float v1,
                        float u2,
                        float v2,
                        float z = 0.0f);
void appendAtlasQuad(render::Tessellator& tessellator, int x, int y, int u, int v, int width, int height, float z);
void appendTiledPanel(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float scrollV, int rgb, float z = 0.0f);
void tiledPanel(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float scrollV, int rgb, float z = 0.0f);
void appendProgressBar(render::Tessellator& tessellator, int x, int y, int width, int height, int fillWidth);
void coloredQuad(
    render::Tessellator& tessellator, int x1, int y1, int x2, int y2, int rgb, int alpha = 255, float z = 0.0f);
void verticalGradientQuad(render::Tessellator& tessellator,
                          int x1,
                          int y1,
                          int x2,
                          int y2,
                          int topRgb,
                          int topAlpha,
                          int bottomRgb,
                          int bottomAlpha,
                          float z = 0.0f);
void texturedQuad(render::Tessellator& tessellator,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  float u1,
                  float v1,
                  float u2,
                  float v2,
                  float z = 0.0f);
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
                         int alpha = 255,
                         float z = 0.0f);
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
                                        float z = 0.0f);
} // namespace net::minecraft::client::gui::draw
