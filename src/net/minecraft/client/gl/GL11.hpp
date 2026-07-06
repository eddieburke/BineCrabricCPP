#pragma once
// OpenGL 1.1 constants and thin ::gl* wrappers (Windows + opengl32).
#include <cstdint>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#if !defined(_WIN32)
#error "OpenGL rendering requires Windows"
#endif
#define MINECRAFT_GL_REAL 1
#include <windows.h>
#include <GL/gl.h>
// windows.h / gl.h define GL_* macros that collide with constexpr names below.
#undef GL_QUADS
#undef GL_TRIANGLES
#undef GL_LINES
#undef GL_LINE_STRIP
#undef GL_BLEND
#undef GL_TEXTURE_2D
#undef GL_ALPHA_TEST
#undef GL_DEPTH_TEST
#undef GL_CULL_FACE
#undef GL_SCISSOR_TEST
#undef GL_LIGHTING
#undef GL_FOG
#undef GL_SRC_ALPHA
#undef GL_ONE_MINUS_SRC_ALPHA
#undef GL_ONE_MINUS_SRC_COLOR
#undef GL_ONE_MINUS_DST_COLOR
#undef GL_SRC_COLOR
#undef GL_DST_COLOR
#undef GL_DST_ALPHA
#undef GL_ONE
#undef GL_ZERO
#undef GL_COLOR_MATERIAL
#undef GL_RESCALE_NORMAL
#undef GL_SMOOTH
#undef GL_FLAT
#undef GL_MODELVIEW
#undef GL_PROJECTION
#undef GL_COLOR_BUFFER_BIT
#undef GL_DEPTH_BUFFER_BIT
#undef GL_CURRENT_BIT
#undef GL_LIGHTING_BIT
#undef GL_ENABLE_BIT
#undef GL_TRANSFORM_BIT
#undef GL_TEXTURE_BIT
#undef GL_ALL_ATTRIB_BITS
#undef GL_GREATER
#undef GL_GEQUAL
#undef GL_LEQUAL
#undef GL_BACK
#undef GL_NEAREST
#undef GL_LINEAR
#undef GL_CLAMP
#undef GL_REPEAT
#undef GL_TEXTURE_MIN_FILTER
#undef GL_TEXTURE_MAG_FILTER
#undef GL_TEXTURE_WRAP_S
#undef GL_TEXTURE_WRAP_T
#undef GL_RGBA
#undef GL_UNSIGNED_BYTE
#undef GL_FOG_COLOR
#undef GL_FOG_MODE
#undef GL_FOG_DENSITY
#undef GL_FOG_START
#undef GL_FOG_END
#undef GL_FRONT
#undef GL_FRONT_AND_BACK
#undef GL_AMBIENT
#undef GL_AMBIENT_AND_DIFFUSE
#undef GL_NORMALIZE
#undef GL_POLYGON_OFFSET_FILL
#undef GL_DOUBLE
#undef GL_BYTE
#undef GL_FLOAT
#undef GL_VERTEX_ARRAY
#undef GL_NORMAL_ARRAY
#undef GL_COLOR_ARRAY
#undef GL_CURRENT_COLOR
#undef GL_TEXTURE_COORD_ARRAY
#undef GL_TRIANGLE_FAN
#undef GL_EQUAL
#undef GL_LIGHT0
#undef GL_LIGHT1
#undef GL_POSITION
#undef GL_DIFFUSE
#undef GL_SPECULAR
#undef GL_LIGHT_MODEL_AMBIENT
#undef GL_NEAREST_MIPMAP_LINEAR
#undef GL_LINEAR_MIPMAP_LINEAR
#undef GL_PROJECTION_MATRIX
#undef GL_MODELVIEW_MATRIX
namespace net::minecraft::client::gl {
namespace math = net::minecraft::util::math;
struct GL11 {
  static constexpr int GL_QUADS = 0x0007;
  static constexpr int GL_TRIANGLES = 0x0004;
  static constexpr int GL_LINES = 0x0001;
  static constexpr int GL_LINE_STRIP = 0x0003;
  static constexpr int GL_BLEND = 0x0BE2;
  static constexpr int GL_TEXTURE_2D = 0x0DE1;
  static constexpr int GL_ALPHA_TEST = 0x0BC0;
  static constexpr int GL_DEPTH_TEST = 0x0B71;
  static constexpr int GL_CULL_FACE = 0x0B44;
  static constexpr int GL_SCISSOR_TEST = 0x0C11;
  static constexpr int GL_LIGHTING = 0x0B50;
  static constexpr int GL_COLOR_MATERIAL = 0x0B57;
  static constexpr int GL_FOG = 0x0B60;
  static constexpr int GL_RESCALE_NORMAL = 0x803A;
  static constexpr int GL_SRC_ALPHA = 0x0302;
  static constexpr int GL_ONE_MINUS_SRC_ALPHA = 0x0303;
  static constexpr int GL_ONE_MINUS_SRC_COLOR = 0x0301;
  static constexpr int GL_ONE_MINUS_DST_COLOR = 0x0307;
  static constexpr int GL_SRC_COLOR = 0x0300;
  static constexpr int GL_DST_COLOR = 0x0306;
  static constexpr int GL_DST_ALPHA = 0x0304;
  static constexpr int GL_ONE = 0x0001;
  static constexpr int GL_ZERO = 0x0000;
  static constexpr int GL_SMOOTH = 0x1D01;
  static constexpr int GL_FLAT = 0x1D00;
  static constexpr int GL_MODELVIEW = 0x1700;
  static constexpr int GL_PROJECTION = 0x1701;
  static constexpr int GL_COLOR_BUFFER_BIT = 0x00004000;
  static constexpr int GL_DEPTH_BUFFER_BIT = 0x00000100;
  static constexpr int GL_CURRENT_BIT = 0x00000001;
  static constexpr int GL_LIGHTING_BIT = 0x00000040;
  static constexpr int GL_TRANSFORM_BIT = 0x00001000;
  static constexpr int GL_ENABLE_BIT = 0x00002000;
  static constexpr int GL_TEXTURE_BIT = 0x00040000;
  static constexpr int GL_ALL_ATTRIB_BITS = 0x000FFFFF;
  static constexpr int GL_GREATER = 0x0204;
  static constexpr int GL_GEQUAL = 0x0206;
  static constexpr int GL_LEQUAL = 0x0203;
  static constexpr int GL_BACK = 0x0405;
  static constexpr int GL_NEAREST = 0x2600;
  static constexpr int GL_LINEAR = 0x2601;
  static constexpr int GL_CLAMP = 0x2900;
  static constexpr int GL_REPEAT = 0x2901;
  static constexpr int GL_TEXTURE_MIN_FILTER = 0x2801;
  static constexpr int GL_TEXTURE_MAG_FILTER = 0x2800;
  static constexpr int GL_TEXTURE_WRAP_S = 0x2802;
  static constexpr int GL_TEXTURE_WRAP_T = 0x2803;
  static constexpr int GL_RGBA = 0x1908;
  static constexpr int GL_UNSIGNED_BYTE = 0x1401;
  static constexpr int GL_FOG_COLOR = 0x0B66;
  static constexpr int GL_FOG_MODE = 0x0B65;
  static constexpr int GL_FOG_DENSITY = 0x0B62;
  static constexpr int GL_FOG_START = 0x0B63;
  static constexpr int GL_FOG_END = 0x0B64;
  static constexpr int GL_FRONT = 0x0404;
  static constexpr int GL_FRONT_AND_BACK = 0x0408;
  static constexpr int GL_AMBIENT = 0x1200;
  static constexpr int GL_AMBIENT_AND_DIFFUSE = 0x1602;
  static constexpr int GL_NORMALIZE = 0x0BA1;
  static constexpr int GL_POLYGON_OFFSET_FILL = 0x8037;
  static constexpr int GL_BYTE = 0x1400;
  static constexpr int GL_FLOAT = 0x1406;
  static constexpr int GL_DOUBLE = 0x140A;
  static constexpr int GL_VERTEX_ARRAY = 0x8074;
  static constexpr int GL_NORMAL_ARRAY = 0x8075;
  static constexpr int GL_COLOR_ARRAY = 0x8076;
  static constexpr int GL_CURRENT_COLOR = 0x0B01;
  static constexpr int GL_TEXTURE_COORD_ARRAY = 0x8078;
  static constexpr int GL_TRIANGLE_FAN = 0x0006;
  static constexpr int GL_EQUAL = 0x0202;
  static constexpr int GL_LIGHT0 = 0x4000;
  static constexpr int GL_LIGHT1 = 0x4001;
  static constexpr int GL_POSITION = 0x1203;
  static constexpr int GL_DIFFUSE = 0x1201;
  static constexpr int GL_SPECULAR = 0x1202;
  static constexpr int GL_LIGHT_MODEL_AMBIENT = 0x0B53;
  static constexpr int GL_NEAREST_MIPMAP_LINEAR = 0x2702;
  static constexpr int GL_LINEAR_MIPMAP_LINEAR = 0x2703;
  static constexpr int GL_PROJECTION_MATRIX = 0x0BA7;
  static constexpr int GL_MODELVIEW_MATRIX = 0x0BA6;
  static constexpr int GL_TEXTURE0 = 0x84C0;
  // CPU matrix stack tracking — updated alongside GL matrix calls.
  inline static math::MatrixStack* s_matrixStack = &math::g_modelView;
  static void glEnable(int cap) {
    ::glEnable(static_cast<GLenum>(cap));
  }
  static void glDisable(int cap) {
    ::glDisable(static_cast<GLenum>(cap));
  }
  static void glBlendFunc(int src, int dst) {
    ::glBlendFunc(static_cast<GLenum>(src), static_cast<GLenum>(dst));
  }
  static void glColor4f(float r, float g, float b, float a) {
    ::glColor4f(r, g, b, a);
  }
  static void glColor3f(float r, float g, float b) {
    ::glColor3f(r, g, b);
  }
  static void glColor4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    ::glColor4ub(r, g, b, a);
  }
  static void glShadeModel(int mode) {
    ::glShadeModel(static_cast<GLenum>(mode));
  }
  static void glDepthMask(bool enabled) {
    ::glDepthMask(enabled ? GL_TRUE : GL_FALSE);
  }
  static void glColorMask(bool r, bool g, bool b, bool a) {
    ::glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
  }
  static void glPushMatrix() {
    ::glPushMatrix();
    s_matrixStack->push();
  }
  static void glPopMatrix() {
    ::glPopMatrix();
    s_matrixStack->pop();
  }
  static void glLoadIdentity() {
    ::glLoadIdentity();
    s_matrixStack->loadIdentity();
  }
  static void glMatrixMode(int mode) {
    ::glMatrixMode(static_cast<GLenum>(mode));
    s_matrixStack = (mode == GL11::GL_PROJECTION) ? &math::g_projection : &math::g_modelView;
  }
  static void glTranslatef(float x, float y, float z) {
    ::glTranslatef(x, y, z);
    s_matrixStack->translate(x, y, z);
  }
  static void glScalef(float x, float y, float z) {
    ::glScalef(x, y, z);
    s_matrixStack->scale(x, y, z);
  }
  static void glRotatef(float angle, float x, float y, float z) {
    ::glRotatef(angle, x, y, z);
    s_matrixStack->rotate(angle, x, y, z);
  }
  static void glBindTexture(int target, int texture) {
    ::glBindTexture(static_cast<GLenum>(target), static_cast<GLuint>(texture));
  }
  static void glActiveTexture(int texture) {
    if(GLCore::activeTexture) reinterpret_cast<void(APIENTRY*)(unsigned)>(GLCore::activeTexture)(static_cast<unsigned>(texture));
  }
  static void glGenTextures(int n, unsigned int* names) {
    ::glGenTextures(n, names);
  }
  static void glTexParameteri(int target, int pname, int param) {
    ::glTexParameteri(static_cast<GLenum>(target), static_cast<GLenum>(pname), param);
  }
  static void glTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int format,
                           int type, const void* pixels) {
    ::glTexImage2D(static_cast<GLenum>(target), level, internalFormat, width, height, border,
                   static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
  }
  static void glTexSubImage2D(int target, int level, int xOffset, int yOffset, int width, int height, int format,
                              int type, const void* pixels) {
    ::glTexSubImage2D(static_cast<GLenum>(target), level, xOffset, yOffset, width, height,
                      static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
  }
  static void glCopyTexSubImage2D(int target, int level, int xOffset, int yOffset, int x, int y, int width,
                                  int height) {
    ::glCopyTexSubImage2D(static_cast<GLenum>(target), level, xOffset, yOffset, x, y, width, height);
  }
  static void glDeleteTextures(int n, const unsigned int* names) {
    ::glDeleteTextures(n, names);
  }
  static void glAlphaFunc(int func, float ref) {
    ::glAlphaFunc(static_cast<GLenum>(func), ref);
  }
  static void glClear(int mask) {
    ::glClear(static_cast<GLbitfield>(mask));
  }
  static void glClearDepth(double depth) {
    ::glClearDepth(depth);
  }
  static void glClearColor(float r, float g, float b, float a) {
    ::glClearColor(r, g, b, a);
  }
  static void glViewport(int x, int y, int width, int height) {
    ::glViewport(x, y, width, height);
  }
  static void glScissor(int x, int y, int width, int height) {
    ::glScissor(x, y, width, height);
  }
  static void glDrawBuffer(int mode) {
    ::glDrawBuffer(static_cast<GLenum>(mode));
  }
  static void glReadBuffer(int mode) {
    ::glReadBuffer(static_cast<GLenum>(mode));
  }
  static void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar) {
    ::glOrtho(left, right, bottom, top, zNear, zFar);
    math::Matrix4f m;
    m.ortho(static_cast<float>(left), static_cast<float>(right),
            static_cast<float>(bottom), static_cast<float>(top),
            static_cast<float>(zNear), static_cast<float>(zFar));
    s_matrixStack->multiply(m);
  }
  static void glBegin(int mode) {
    ::glBegin(static_cast<GLenum>(mode));
  }
  static void glEnd() {
    ::glEnd();
  }
  static void glTexCoord2d(double u, double v) {
    ::glTexCoord2d(u, v);
  }
  static void glVertex3d(double x, double y, double z) {
    ::glVertex3d(x, y, z);
  }
  static void glNormal3b(std::int8_t x, std::int8_t y, std::int8_t z) {
    ::glNormal3b(x, y, z);
  }
  static void glGetFloatv(int pname, float* params) {
    ::glGetFloatv(static_cast<GLenum>(pname), params);
  }
  static void glGetIntegerv(int pname, int* params) {
    ::glGetIntegerv(static_cast<GLenum>(pname), params);
  }
  static void glScaled(double x, double y, double z) {
    ::glScaled(x, y, z);
    s_matrixStack->scale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  }
  static void glNormal3f(float x, float y, float z) {
    ::glNormal3f(x, y, z);
  }
  static void glFogi(int pname, int param) {
    ::glFogi(static_cast<GLenum>(pname), param);
  }
  static void glFogf(int pname, float param) {
    ::glFogf(static_cast<GLenum>(pname), param);
  }
  static void glFogfv(int pname, const float* params) {
    ::glFogfv(static_cast<GLenum>(pname), params);
  }
  static void glColorMaterial(int face, int mode) {
    ::glColorMaterial(static_cast<GLenum>(face), static_cast<GLenum>(mode));
  }
  static void glLightfv(int light, int pname, const float* params) {
    ::glLightfv(static_cast<GLenum>(light), static_cast<GLenum>(pname), params);
  }
  static void glLightModelfv(int pname, const float* params) {
    ::glLightModelfv(static_cast<GLenum>(pname), params);
  }
  static void glEnableClientState(int array) {
    ::glEnableClientState(static_cast<GLenum>(array));
  }
  static void glDisableClientState(int array) {
    ::glDisableClientState(static_cast<GLenum>(array));
  }
  static void glVertexPointer(int size, int type, int stride, const void* pointer) {
    ::glVertexPointer(size, static_cast<GLenum>(type), stride, pointer);
  }
  static void glTexCoordPointer(int size, int type, int stride, const void* pointer) {
    ::glTexCoordPointer(size, static_cast<GLenum>(type), stride, pointer);
  }
  static void glColorPointer(int size, int type, int stride, const void* pointer) {
    ::glColorPointer(size, static_cast<GLenum>(type), stride, pointer);
  }
  static void glNormalPointer(int type, int stride, const void* pointer) {
    ::glNormalPointer(static_cast<GLenum>(type), stride, pointer);
  }
  static void glDrawArrays(int mode, int first, int count) {
    ::glDrawArrays(static_cast<GLenum>(mode), first, count);
  }
  static void glPolygonOffset(float factor, float units) {
    ::glPolygonOffset(factor, units);
  }
  static void glLineWidth(float width) {
    ::glLineWidth(width);
  }
  static void glDepthFunc(int func) {
    ::glDepthFunc(static_cast<GLenum>(func));
  }
  static void glCullFace(int mode) {
    ::glCullFace(static_cast<GLenum>(mode));
  }
  static int glGetError() {
    return static_cast<int>(::glGetError());
  }
  static void glPushAttrib(int mask) {
    ::glPushAttrib(static_cast<GLbitfield>(mask));
  }
  static void glPopAttrib() {
    ::glPopAttrib();
  }
};
class MatrixGuard {
public:
  MatrixGuard() { GL11::glPushMatrix(); }
  ~MatrixGuard() { GL11::glPopMatrix(); }
  MatrixGuard(const MatrixGuard&) = delete;
  MatrixGuard& operator=(const MatrixGuard&) = delete;
};
class AttribGuard {
public:
  explicit AttribGuard(int mask) { GL11::glPushAttrib(mask); }
  ~AttribGuard() { GL11::glPopAttrib(); }
  AttribGuard(const AttribGuard&) = delete;
  AttribGuard& operator=(const AttribGuard&) = delete;
};
class DisableGuard {
public:
  explicit DisableGuard(int capability) : capability_(capability) { GL11::glDisable(capability_); }
  ~DisableGuard() { GL11::glEnable(capability_); }
  DisableGuard(const DisableGuard&) = delete;
  DisableGuard& operator=(const DisableGuard&) = delete;

private:
  int capability_;
};
} // namespace net::minecraft::client::gl
namespace gl = net::minecraft::client::gl;
