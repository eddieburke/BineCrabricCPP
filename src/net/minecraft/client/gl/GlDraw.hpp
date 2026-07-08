#pragma once
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#include <cstdint>
#include <initializer_list>
namespace net::minecraft::client::gl {
namespace math = net::minecraft::util::math;
namespace matrix {
inline math::MatrixStack* active = &math::g_modelView;
inline void push() {
  ::glPushMatrix();
  active->push();
}
inline void pop() {
  ::glPopMatrix();
  active->pop();
}
inline void loadIdentity() {
  ::glLoadIdentity();
  active->loadIdentity();
}
inline void mode(int matrixMode) {
  ::glMatrixMode(static_cast<GLenum>(matrixMode));
  active = (matrixMode == matrix_::Projection) ? &math::g_projection : &math::g_modelView;
}
inline void translate(float x, float y, float z) {
  ::glTranslatef(x, y, z);
  active->translate(x, y, z);
}
inline void scale(float x, float y, float z) {
  ::glScalef(x, y, z);
  active->scale(x, y, z);
}
inline void rotate(float angle, float x, float y, float z) {
  ::glRotatef(angle, x, y, z);
  active->rotate(angle, x, y, z);
}
inline void scale(double x, double y, double z) {
  ::glScaled(x, y, z);
  active->scale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}
inline void ortho(double left, double right, double bottom, double top, double zNear, double zFar) {
  ::glOrtho(left, right, bottom, top, zNear, zFar);
  math::Matrix4f m;
  m.ortho(static_cast<float>(left), static_cast<float>(right), static_cast<float>(bottom),
          static_cast<float>(top), static_cast<float>(zNear), static_cast<float>(zFar));
  active->multiply(m);
}
class Guard {
public:
  Guard() { push(); }
  ~Guard() { pop(); }
  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;
};
} // namespace matrix
using MatrixGuard = matrix::Guard;
[[nodiscard]] inline bool capEnabled(int cap) {
  return ::glIsEnabled(static_cast<GLenum>(cap)) == GL_TRUE;
}
inline void enable(int cap) {
  ::glEnable(static_cast<GLenum>(cap));
}
inline void disable(int cap) {
  ::glDisable(static_cast<GLenum>(cap));
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
  ::glColor4f(r, g, b, a);
}
inline void color3f(float r, float g, float b) {
  ::glColor3f(r, g, b);
}
inline void color4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
  ::glColor4ub(r, g, b, a);
}
inline void shadeModel(int mode) {
  ::glShadeModel(static_cast<GLenum>(mode));
}
inline void depthMask(bool enabled) {
  ::glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}
inline void colorMask(bool r, bool g, bool b, bool a) {
  ::glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
}
inline void pushMatrix() { matrix::push(); }
inline void popMatrix() { matrix::pop(); }
inline void loadIdentity() { matrix::loadIdentity(); }
inline void matrixMode(int mode) { matrix::mode(mode); }
inline void translatef(float x, float y, float z) { matrix::translate(x, y, z); }
inline void scalef(float x, float y, float z) { matrix::scale(x, y, z); }
inline void rotatef(float angle, float x, float y, float z) { matrix::rotate(angle, x, y, z); }
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
inline void texImage2D(int target, int level, int internalFormat, int width, int height, int border, int format,
                       int type, const void* pixels) {
  ::glTexImage2D(static_cast<GLenum>(target), level, internalFormat, width, height, border,
                 static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
}
inline void texSubImage2D(int target, int level, int xOffset, int yOffset, int width, int height, int format, int type,
                          const void* pixels) {
  ::glTexSubImage2D(static_cast<GLenum>(target), level, xOffset, yOffset, width, height, static_cast<GLenum>(format),
                    static_cast<GLenum>(type), pixels);
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
    GLCore::framebufferTexture2D(static_cast<unsigned>(target), static_cast<unsigned>(attachment),
                                 static_cast<unsigned>(textarget), texture, level);
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
    GLCore::renderbufferStorage(static_cast<unsigned>(target), static_cast<unsigned>(internalFormat), width, height);
  }
}
inline void framebufferRenderbuffer(int target, int attachment, int renderbuffertarget, unsigned int renderbuffer) {
  if(GLCore::framebufferRenderbuffer != nullptr) {
    GLCore::framebufferRenderbuffer(static_cast<unsigned>(target), static_cast<unsigned>(attachment),
                                    static_cast<unsigned>(renderbuffertarget), renderbuffer);
  }
}
inline void alphaFunc(int func, float ref) {
  ::glAlphaFunc(static_cast<GLenum>(func), ref);
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
inline void readBuffer(int mode) {
  ::glReadBuffer(static_cast<GLenum>(mode));
}
inline void ortho(double left, double right, double bottom, double top, double zNear, double zFar) {
  matrix::ortho(left, right, bottom, top, zNear, zFar);
}
inline void begin(int mode) {
  ::glBegin(static_cast<GLenum>(mode));
}
inline void end() {
  ::glEnd();
}
inline void texCoord2d(double u, double v) {
  ::glTexCoord2d(u, v);
}
inline void vertex3d(double x, double y, double z) {
  ::glVertex3d(x, y, z);
}
inline void normal3b(std::int8_t x, std::int8_t y, std::int8_t z) {
  ::glNormal3b(x, y, z);
}
inline void getFloatv(int pname, float* params) {
  ::glGetFloatv(static_cast<GLenum>(pname), params);
}
inline void getIntegerv(int pname, int* params) {
  ::glGetIntegerv(static_cast<GLenum>(pname), params);
}
inline void scaled(double x, double y, double z) {
  matrix::scale(x, y, z);
}
inline void normal3f(float x, float y, float z) {
  ::glNormal3f(x, y, z);
}
inline void fogi(int pname, int param) {
  ::glFogi(static_cast<GLenum>(pname), param);
}
inline void fogf(int pname, float param) {
  ::glFogf(static_cast<GLenum>(pname), param);
}
inline void fogfv(int pname, const float* params) {
  ::glFogfv(static_cast<GLenum>(pname), params);
}
inline void colorMaterial(int face, int mode) {
  ::glColorMaterial(static_cast<GLenum>(face), static_cast<GLenum>(mode));
}
inline void lightfv(int light, int pname, const float* params) {
  ::glLightfv(static_cast<GLenum>(light), static_cast<GLenum>(pname), params);
}
inline void lightModelfv(int pname, const float* params) {
  ::glLightModelfv(static_cast<GLenum>(pname), params);
}
inline void enableClientState(int array) {
  ::glEnableClientState(static_cast<GLenum>(array));
}
inline void disableClientState(int array) {
  ::glDisableClientState(static_cast<GLenum>(array));
}
inline void vertexPointer(int size, int type, int stride, const void* pointer) {
  ::glVertexPointer(size, static_cast<GLenum>(type), stride, pointer);
}
inline void texCoordPointer(int size, int type, int stride, const void* pointer) {
  ::glTexCoordPointer(size, static_cast<GLenum>(type), stride, pointer);
}
inline void colorPointer(int size, int type, int stride, const void* pointer) {
  ::glColorPointer(size, static_cast<GLenum>(type), stride, pointer);
}
inline void normalPointer(int type, int stride, const void* pointer) {
  ::glNormalPointer(static_cast<GLenum>(type), stride, pointer);
}
inline void drawArrays(int mode, int first, int count) {
  ::glDrawArrays(static_cast<GLenum>(mode), first, count);
}
inline void polygonOffset(float factor, float units) {
  ::glPolygonOffset(factor, units);
}
inline void lineWidth(float width) {
  ::glLineWidth(width);
}
inline void pointSize(float size) {
  ::glPointSize(size);
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
