#pragma once
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/PipelineState.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
namespace net::minecraft::client::gl {
namespace math = net::minecraft::util::math;
// Core profile: the CPU MatrixStacks are the sole source of truth. No fixed-function
// matrix calls survive; draw paths upload MatrixStack tops as uniforms.
namespace matrix {
inline math::MatrixStack* active = &math::g_modelView;
inline void push() {
  active->push();
}
inline void pop() {
  active->pop();
}
inline void loadIdentity() {
  active->loadIdentity();
}
inline void mode(int matrixMode) {
  active = (matrixMode == matrix_::Projection) ? &math::g_projection : &math::g_modelView;
}
inline void translate(float x, float y, float z) {
  active->translate(x, y, z);
}
inline void scale(float x, float y, float z) {
  active->scale(x, y, z);
}
inline void rotate(float angle, float x, float y, float z) {
  active->rotate(angle, x, y, z);
}
inline void scale(double x, double y, double z) {
  active->scale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}
inline void ortho(double left, double right, double bottom, double top, double zNear, double zFar) {
  math::Matrix4f m;
  m.ortho(static_cast<float>(left),
          static_cast<float>(right),
          static_cast<float>(bottom),
          static_cast<float>(top),
          static_cast<float>(zNear),
          static_cast<float>(zFar));
  active->multiply(m);
}
inline void multiply(const math::Matrix4f& m) {
  active->multiply(m);
}
inline void load(const math::Matrix4f& m) {
  active->load(m);
}
inline void perspective(float fovYDeg, float aspect, float zNear, float zFar) {
  math::Matrix4f m;
  m.perspective(fovYDeg, aspect, zNear, zFar);
  active->multiply(m);
}
class Guard {
public:
  Guard() {
    push();
  }
  ~Guard() {
    pop();
  }
  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;
};
} // namespace matrix
using MatrixGuard = matrix::Guard;
// Caps that no longer exist in a core profile are mirrored into PipelineState instead
// of calling real GL. Blend / DepthTest / CullFace / ScissorTest / PolygonOffsetFill
// remain valid core state and pass through.
inline bool isPipelineStateCap(int value) {
  switch(value) {
  case cap::Texture2D:
  case cap::AlphaTest:
  case cap::Fog:
  case cap::Lighting:
  case cap::ColorMaterial:
  case cap::RescaleNormal:
  case cap::Normalize:
    return true;
  default:
    return false;
  }
}
inline bool applyFixedFunctionCap(int cap, bool enabled) {
  switch(cap) {
  case cap::Texture2D:
    g_pipeline.texture2D = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::AlphaTest:
    g_pipeline.alphaTest = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::Fog:
    g_pipeline.fogEnabled = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::Lighting:
    g_pipeline.lighting = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::ColorMaterial:
    g_pipeline.colorMaterial = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::RescaleNormal:
    g_pipeline.rescaleNormal = enabled;
    g_pipeline.dirty = true;
    return true;
  case cap::Normalize:
    g_pipeline.normalize = enabled;
    g_pipeline.dirty = true;
    return true;
  default: return false;
  }
}
[[nodiscard]] inline bool capEnabled(int cap) {
  switch(cap) {
  case cap::Texture2D: return g_pipeline.texture2D;
  case cap::AlphaTest: return g_pipeline.alphaTest;
  case cap::Fog: return g_pipeline.fogEnabled;
  case cap::Lighting: return g_pipeline.lighting;
  case cap::ColorMaterial: return g_pipeline.colorMaterial;
  case cap::RescaleNormal: return g_pipeline.rescaleNormal;
  case cap::Normalize: return g_pipeline.normalize;
  default: return ::glIsEnabled(static_cast<GLenum>(cap)) == GL_TRUE;
  }
}
inline void enable(int cap) {
  if(!applyFixedFunctionCap(cap, true)) {
    ::glEnable(static_cast<GLenum>(cap));
  }
}
inline void disable(int cap) {
  if(!applyFixedFunctionCap(cap, false)) {
    ::glDisable(static_cast<GLenum>(cap));
  }
}
inline void setCap(int cap, bool enabled) {
  if(enabled) {
    enable(cap);
  } else {
    disable(cap);
  }
}
inline void blendFunc(int src, int dst) {
  ::glBlendFunc(static_cast<GLenum>(src), static_cast<GLenum>(dst));
}
inline void blendAlpha() {
  enable(cap::Blend);
  blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
}
inline void color4f(float r, float g, float b, float a) {
  g_pipeline.constColor[0] = r;
  g_pipeline.constColor[1] = g;
  g_pipeline.constColor[2] = b;
  g_pipeline.constColor[3] = a;
  g_pipeline.dirty = true;
}
inline void color3f(float r, float g, float b) {
  color4f(r, g, b, 1.0f);
}
inline void color4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
  color4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
inline void shadeModel(int mode) {
  g_pipeline.smoothShading = (mode == shade::Smooth);
  g_pipeline.dirty = true;
}
inline void depthMask(bool enabled) {
  ::glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}
inline void colorMask(bool r, bool g, bool b, bool a) {
  ::glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
}
inline void pushMatrix() {
  matrix::push();
}
inline void popMatrix() {
  matrix::pop();
}
inline void loadIdentity() {
  matrix::loadIdentity();
}
inline void matrixMode(int mode) {
  matrix::mode(mode);
}
inline void translatef(float x, float y, float z) {
  matrix::translate(x, y, z);
}
inline void scalef(float x, float y, float z) {
  matrix::scale(x, y, z);
}
inline void rotatef(float angle, float x, float y, float z) {
  matrix::rotate(angle, x, y, z);
}
inline void bindTexture(int target, int texture) {
  ::glBindTexture(static_cast<GLenum>(target), static_cast<GLuint>(texture));
}
inline void activeTexture(int texture) {
  if(GLCore::activeTexture) {
    reinterpret_cast<void(APIENTRY*)(unsigned)>(GLCore::activeTexture)(static_cast<unsigned>(texture));
  }
}
inline void genTextures(int n, unsigned int* names) {
  ::glGenTextures(n, names);
}
inline void texParameteri(int target, int pname, int param) {
  ::glTexParameteri(static_cast<GLenum>(target), static_cast<GLenum>(pname), param);
}
inline void texImage2D(int target,
                       int level,
                       int internalFormat,
                       int width,
                       int height,
                       int border,
                       int format,
                       int type,
                       const void* pixels) {
  ::glTexImage2D(static_cast<GLenum>(target),
                 level,
                 internalFormat,
                 width,
                 height,
                 border,
                 static_cast<GLenum>(format),
                 static_cast<GLenum>(type),
                 pixels);
}
inline void texSubImage2D(
    int target, int level, int xOffset, int yOffset, int width, int height, int format, int type, const void* pixels) {
  ::glTexSubImage2D(static_cast<GLenum>(target),
                    level,
                    xOffset,
                    yOffset,
                    width,
                    height,
                    static_cast<GLenum>(format),
                    static_cast<GLenum>(type),
                    pixels);
}
inline void copyTexSubImage2D(int target, int level, int xOffset, int yOffset, int x, int y, int width, int height) {
  ::glCopyTexSubImage2D(static_cast<GLenum>(target), level, xOffset, yOffset, x, y, width, height);
}
inline void deleteTextures(int n, const unsigned int* names) {
  ::glDeleteTextures(n, names);
}
inline void genFramebuffers(int n, unsigned int* names) {
  if(GLCore::genFramebuffers != nullptr) {
    GLCore::genFramebuffers(n, names);
  }
}
inline void bindFramebuffer(int target, unsigned int framebuffer) {
  if(GLCore::bindFramebuffer != nullptr) {
    GLCore::bindFramebuffer(static_cast<unsigned>(target), framebuffer);
  }
}
inline void deleteFramebuffers(int n, const unsigned int* names) {
  if(GLCore::deleteFramebuffers != nullptr) {
    GLCore::deleteFramebuffers(n, names);
  }
}
inline unsigned int checkFramebufferStatus(int target) {
  if(GLCore::checkFramebufferStatus != nullptr) {
    return GLCore::checkFramebufferStatus(static_cast<unsigned>(target));
  }
  return 0;
}
inline void framebufferTexture2D(int target, int attachment, int textarget, unsigned int texture, int level) {
  if(GLCore::framebufferTexture2D != nullptr) {
    GLCore::framebufferTexture2D(static_cast<unsigned>(target),
                                 static_cast<unsigned>(attachment),
                                 static_cast<unsigned>(textarget),
                                 texture,
                                 level);
  }
}
inline void genRenderbuffers(int n, unsigned int* names) {
  if(GLCore::genRenderbuffers != nullptr) {
    GLCore::genRenderbuffers(n, names);
  }
}
inline void bindRenderbuffer(int target, unsigned int renderbuffer) {
  if(GLCore::bindRenderbuffer != nullptr) {
    GLCore::bindRenderbuffer(static_cast<unsigned>(target), renderbuffer);
  }
}
inline void deleteRenderbuffers(int n, const unsigned int* names) {
  if(GLCore::deleteRenderbuffers != nullptr) {
    GLCore::deleteRenderbuffers(n, names);
  }
}
inline void renderbufferStorage(int target, int internalFormat, int width, int height) {
  if(GLCore::renderbufferStorage != nullptr) {
    GLCore::renderbufferStorage(
        static_cast<unsigned>(target), static_cast<unsigned>(internalFormat), width, height);
  }
}
inline void framebufferRenderbuffer(int target, int attachment, int renderbuffertarget, unsigned int renderbuffer) {
  if(GLCore::framebufferRenderbuffer != nullptr) {
    GLCore::framebufferRenderbuffer(static_cast<unsigned>(target),
                                    static_cast<unsigned>(attachment),
                                    static_cast<unsigned>(renderbuffertarget),
                                    renderbuffer);
  }
}
inline void alphaFunc(int /*func*/, float ref) {
  // The ubershader implements a fixed "discard when alpha <= ref" test; only the
  // reference value varies across call sites.
  g_pipeline.alphaRef = ref;
  g_pipeline.dirty = true;
}
inline void clear(int mask) {
  ::glClear(static_cast<GLbitfield>(mask));
}
inline void clearDepth(double depth) {
  ::glClearDepth(depth);
}
inline void clearColor(float r, float g, float b, float a) {
  ::glClearColor(r, g, b, a);
}
inline void viewport(int x, int y, int width, int height) {
  ::glViewport(x, y, width, height);
}
inline void scissor(int x, int y, int width, int height) {
  ::glScissor(x, y, width, height);
}
inline void drawBuffer(int mode) {
  ::glDrawBuffer(static_cast<GLenum>(mode));
}
inline void ortho(double left, double right, double bottom, double top, double zNear, double zFar) {
  matrix::ortho(left, right, bottom, top, zNear, zFar);
}
inline void getFloatv(int pname, float* params) {
  switch(pname) {
  case query::CurrentColor:
    std::memcpy(params, g_pipeline.constColor, sizeof(float) * 4);
    return;
  case query::AlphaRef:
    params[0] = g_pipeline.alphaRef;
    return;
  case matrix_::ModelViewMatrix:
    std::memcpy(params, math::g_modelView.top().data(), sizeof(float) * 16);
    return;
  case matrix_::ProjectionMatrix:
    std::memcpy(params, math::g_projection.top().data(), sizeof(float) * 16);
    return;
  default:
    ::glGetFloatv(static_cast<GLenum>(pname), params);
  }
}
inline void getIntegerv(int pname, int* params) {
  switch(pname) {
  case query::MatrixMode:
    params[0] = (matrix::active == &math::g_projection) ? matrix_::Projection : matrix_::ModelView;
    return;
  case query::ShadeModel:
    params[0] = g_pipeline.smoothShading ? shade::Smooth : shade::Flat;
    return;
  case query::AlphaFunc:
    params[0] = compare::Greater;
    return;
  default:
    ::glGetIntegerv(static_cast<GLenum>(pname), params);
  }
}
inline void scaled(double x, double y, double z) {
  matrix::scale(x, y, z);
}
inline void normal3f(float x, float y, float z) {
  g_pipeline.currentNormal[0] = x;
  g_pipeline.currentNormal[1] = y;
  g_pipeline.currentNormal[2] = z;
  g_pipeline.dirty = true;
}
inline void fogi(int pname, int param) {
  if(pname == fog::Mode) {
    g_pipeline.fogMode = translateGlFogMode(param);
    g_pipeline.dirty = true;
  }
}
inline void fogf(int pname, float param) {
  switch(pname) {
  case fog::Density:
    g_pipeline.fogDensity = param;
    g_pipeline.dirty = true;
    break;
  case fog::Start:
    g_pipeline.fogStart = param;
    g_pipeline.dirty = true;
    break;
  case fog::End:
    g_pipeline.fogEnd = param;
    g_pipeline.dirty = true;
    break;
  default: return;
  }
}
inline void fogfv(int pname, const float* params) {
  if(pname == fog::Color) {
    std::memcpy(g_pipeline.fogColor, params, sizeof(float) * 4);
    g_pipeline.dirty = true;
  } else {
    fogf(pname, params[0]);
  }
}
// The ubershader only supports the ambient-and-diffuse color-material model, so the
// mode is ignored; only the ColorMaterial cap (mirrored in PipelineState) matters.
inline void colorMaterial(int /*face*/, int /*mode*/) {}
inline void drawArrays(int mode, int first, int count) {
  ::glDrawArrays(static_cast<GLenum>(mode), first, count);
}
inline void polygonOffset(float factor, float units) {
  ::glPolygonOffset(factor, units);
}
inline void lineWidth(float width) {
  // Core profile guarantees only a max line width of 1.0; wider values are undefined.
  ::glLineWidth(width > 1.0f ? 1.0f : width);
}
inline void depthFunc(int func) {
  ::glDepthFunc(static_cast<GLenum>(func));
}
inline void cullFace(int mode) {
  ::glCullFace(static_cast<GLenum>(mode));
}
inline int getError() {
  return static_cast<int>(::glGetError());
}
} // namespace net::minecraft::client::gl
