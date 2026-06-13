#pragma once

namespace net::minecraft::client::render {
class Tessellator;
}

namespace net::minecraft::client::gui::draw {

// Standard GUI quad winding used throughout the port:
//   v0 = (x1,y2)  bottom-left
//   v1 = (x2,y2)  bottom-right
//   v2 = (x2,y1)  top-right
//   v3 = (x1,y1)  top-left
// Every helper below performs its own startQuads()..draw(); callers must NOT
// bracket. Blend/texture-enable GL state remains the caller's responsibility.

// --- Untextured ----------------------------------------------------------

// Uses whatever color the caller already set on the tessellator (or glColor).
void quad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2, float z = 0.0f);

// Single flat color applied to all four corners. Color is 0xRRGGBB; alpha 0..255.
void coloredQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    int rgb, int alpha = 255, float z = 0.0f);

// Vertical gradient: topRgb/topAlpha drive the top edge (y1), bottomRgb/bottomAlpha
// the bottom edge (y2). Each color is 0xRRGGBB, alpha 0..255.
void verticalGradientQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    int topRgb, int topAlpha, int bottomRgb, int bottomAlpha, float z = 0.0f);

// --- Textured ------------------------------------------------------------

// Per-corner texture coordinates (already in 0..1 texel space), pairing with
// vertices in winding order:
//   v0->(u1,v2)  v1->(u2,v2)  v2->(u2,v1)  v3->(u1,v1)
// Uses whatever color the caller already set.
void texturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2, float z = 0.0f);

// Textured quad with a single flat color applied to all four corners.
void coloredTexturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2, int rgb, int alpha = 255, float z = 0.0f);

// Textured quad with a vertical color gradient (top edge = y1, bottom edge = y2).
void verticalGradientTexturedQuad(render::Tessellator& tessellator, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2,
    int topRgb, int topAlpha, int bottomRgb, int bottomAlpha, float z = 0.0f);

} // namespace net::minecraft::client::gui::draw
