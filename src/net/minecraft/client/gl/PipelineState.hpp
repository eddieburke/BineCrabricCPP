#pragma once
// PipelineState — CPU shadow of the fixed-function draw state that no longer exists
// in a core profile. The legacy free functions in GlDraw.hpp (setCap for the FF-only
// caps, color4f, alphaFunc, fog*, shadeModel, colorMaterial) write here instead of
// calling real GL; Tessellator::drawMesh and the fullscreen blit read this state and
// feed the engine ubershader (assets/shaders/engine/legacy.{vsh,fsh}).
//
// Blend / DepthTest / CullFace / ScissorTest / PolygonOffsetFill remain real GL state
// and are NOT mirrored here — they are still valid in core.
#include <cstdint>
namespace net::minecraft::client::gl {
// Fog mode as consumed by the shader: 0 off, 1 linear, 2 exp, 3 exp2.
namespace fog_mode {
inline constexpr int Off = 0;
inline constexpr int Linear = 1;
inline constexpr int Exp = 2;
inline constexpr int Exp2 = 3;
} // namespace fog_mode
// Translates a GL fog-mode enum (GL_LINEAR/GL_EXP/GL_EXP2) to the shader's 1/2/3.
inline int translateGlFogMode(int glMode) {
  switch(glMode) {
  case 0x2601: return fog_mode::Linear; // GL_LINEAR
  case 0x0800: return fog_mode::Exp;    // GL_EXP
  case 0x0801: return fog_mode::Exp2;   // GL_EXP2
  default: return fog_mode::Linear;
  }
}
struct PipelineState {
  // Fixed-function capability mirrors (only the ones removed from core GL).
  bool texture2D = false;
  bool alphaTest = false;
  bool fogEnabled = false;
  bool lighting = false;
  bool colorMaterial = false;
  bool rescaleNormal = false;
  bool normalize = false;
  bool smoothShading = true;
  // Constant/current color (glColor4f). Multiplied with texture and vertex color.
  float constColor[4]{1.0f, 1.0f, 1.0f, 1.0f};
  // Alpha test: discard when texel.a * color.a <= alphaRef (func fixed to Greater).
  float alphaRef = 0.1f;
  // Fog. mode uses fog_mode:: constants. color is rgba.
  int fogMode = fog_mode::Linear;
  float fogDensity = 1.0f;
  float fogStart = 0.0f;
  float fogEnd = 1.0f;
  float fogColor[4]{0.0f, 0.0f, 0.0f, 1.0f};
  // Current normal (glNormal3f) used for draws that lack a per-vertex normal array.
  float currentNormal[3]{0.0f, 1.0f, 0.0f};
  // Set whenever any mirrored field changes; engine_pipeline::bindAndUploadUniforms
  // re-uploads the pipeline uniform group and clears it.
  bool dirty = true;
};
// The single active pipeline state. Written by GlDraw.hpp shims, read by draw paths.
inline PipelineState g_pipeline;
} // namespace net::minecraft::client::gl
