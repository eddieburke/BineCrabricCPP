#pragma once

// OpenGL 1.1 constants and thin ::gl* wrappers.
// On non-Windows or in test builds (MINECRAFT_GL_REAL_NO_GL) calls compile to no-ops.

#include <cstdint>

#if defined(_WIN32) && !defined(MINECRAFT_GL_REAL_NO_GL)
#define MINECRAFT_GL_REAL 1
#include <windows.h>
#include <GL/gl.h>
#endif

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
#undef GL_LIGHTING
#undef GL_FOG
#undef GL_SRC_ALPHA
#undef GL_ONE_MINUS_SRC_ALPHA
#undef GL_SRC_COLOR
#undef GL_DST_COLOR
#undef GL_ONE
#undef GL_ZERO
#undef GL_SMOOTH
#undef GL_FLAT
#undef GL_MODELVIEW
#undef GL_PROJECTION
#undef GL_COLOR_BUFFER_BIT
#undef GL_DEPTH_BUFFER_BIT
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
#undef GL_COMPILE
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
#undef GL_TEXTURE_COORD_ARRAY

namespace net::minecraft::client::gl {

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
    static constexpr int GL_LIGHTING = 0x0B50;
    static constexpr int GL_FOG = 0x0B60;

    static constexpr int GL_SRC_ALPHA = 0x0302;
    static constexpr int GL_ONE_MINUS_SRC_ALPHA = 0x0303;
    static constexpr int GL_SRC_COLOR = 0x0300;
    static constexpr int GL_DST_COLOR = 0x0306;
    static constexpr int GL_ONE = 0x0001;
    static constexpr int GL_ZERO = 0x0000;

    static constexpr int GL_SMOOTH = 0x1D01;
    static constexpr int GL_FLAT = 0x1D00;

    static constexpr int GL_MODELVIEW = 0x1700;
    static constexpr int GL_PROJECTION = 0x1701;

    static constexpr int GL_COLOR_BUFFER_BIT = 0x00004000;
    static constexpr int GL_DEPTH_BUFFER_BIT = 0x00000100;

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
    static constexpr int GL_COMPILE = 0x1300;
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
    static constexpr int GL_TEXTURE_COORD_ARRAY = 0x8078;

#ifdef MINECRAFT_GL_REAL
    static void glEnable(int cap) { ::glEnable(static_cast<GLenum>(cap)); }
    static void glDisable(int cap) { ::glDisable(static_cast<GLenum>(cap)); }
    static void glBlendFunc(int src, int dst) { ::glBlendFunc(static_cast<GLenum>(src), static_cast<GLenum>(dst)); }
    static void glColor4f(float r, float g, float b, float a) { ::glColor4f(r, g, b, a); }
    static void glColor3f(float r, float g, float b) { ::glColor3f(r, g, b); }
    static void glCallLists(int count, unsigned int type, const void* lists)
    {
        ::glCallLists(count, static_cast<GLenum>(type), lists);
    }
    static void glColor4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) { ::glColor4ub(r, g, b, a); }
    static void glShadeModel(int mode) { ::glShadeModel(static_cast<GLenum>(mode)); }
    static void glDepthMask(bool enabled) { ::glDepthMask(enabled ? GL_TRUE : GL_FALSE); }
    static void glColorMask(bool r, bool g, bool b, bool a)
    {
        ::glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
    }
    static void glPushMatrix() { ::glPushMatrix(); }
    static void glPopMatrix() { ::glPopMatrix(); }
    static void glLoadIdentity() { ::glLoadIdentity(); }
    static void glMatrixMode(int mode) { ::glMatrixMode(static_cast<GLenum>(mode)); }
    static void glTranslatef(float x, float y, float z) { ::glTranslatef(x, y, z); }
    static void glScalef(float x, float y, float z) { ::glScalef(x, y, z); }
    static void glRotatef(float angle, float x, float y, float z) { ::glRotatef(angle, x, y, z); }
    static void glBindTexture(int target, int texture) { ::glBindTexture(static_cast<GLenum>(target), static_cast<GLuint>(texture)); }
    static void glGenTextures(int n, unsigned int* names) { ::glGenTextures(n, names); }
    static void glTexParameteri(int target, int pname, int param)
    {
        ::glTexParameteri(static_cast<GLenum>(target), static_cast<GLenum>(pname), param);
    }
    static void glTexImage2D(int target, int level, int internalFormat, int width, int height, int border,
                             int format, int type, const void* pixels)
    {
        ::glTexImage2D(static_cast<GLenum>(target), level, internalFormat, width, height, border,
                       static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
    }
    static void glTexSubImage2D(int target, int level, int xOffset, int yOffset, int width, int height,
                                int format, int type, const void* pixels)
    {
        ::glTexSubImage2D(static_cast<GLenum>(target), level, xOffset, yOffset, width, height,
                          static_cast<GLenum>(format), static_cast<GLenum>(type), pixels);
    }
    static void glDeleteTextures(int n, const unsigned int* names) { ::glDeleteTextures(n, names); }
    static int glGenLists(int range) { return static_cast<int>(::glGenLists(range)); }
    static void glNewList(int list, int mode) { ::glNewList(static_cast<GLuint>(list), static_cast<GLenum>(mode)); }
    static void glEndList() { ::glEndList(); }
    static void glCallList(int list) { ::glCallList(static_cast<GLuint>(list)); }
    static void glDeleteLists(int list, int range) { ::glDeleteLists(static_cast<GLuint>(list), range); }
    static void glAlphaFunc(int func, float ref) { ::glAlphaFunc(static_cast<GLenum>(func), ref); }
    static void glClear(int mask) { ::glClear(static_cast<GLbitfield>(mask)); }
    static void glClearDepth(double depth) { ::glClearDepth(depth); }
    static void glClearColor(float r, float g, float b, float a) { ::glClearColor(r, g, b, a); }
    static void glViewport(int x, int y, int width, int height) { ::glViewport(x, y, width, height); }
    static void glDrawBuffer(int mode) { ::glDrawBuffer(static_cast<GLenum>(mode)); }
    static void glReadBuffer(int mode) { ::glReadBuffer(static_cast<GLenum>(mode)); }
    static void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar)
    {
        ::glOrtho(left, right, bottom, top, zNear, zFar);
    }
    static void glBegin(int mode) { ::glBegin(static_cast<GLenum>(mode)); }
    static void glEnd() { ::glEnd(); }
    static void glTexCoord2d(double u, double v) { ::glTexCoord2d(u, v); }
    static void glVertex3d(double x, double y, double z) { ::glVertex3d(x, y, z); }
    static void glNormal3b(std::int8_t x, std::int8_t y, std::int8_t z) { ::glNormal3b(x, y, z); }
    static void glGetFloatv(int pname, float* params) { ::glGetFloatv(static_cast<GLenum>(pname), params); }
    static void glScaled(double x, double y, double z) { ::glScaled(x, y, z); }
    static void glNormal3f(float x, float y, float z) { ::glNormal3f(x, y, z); }
    static void glFogi(int pname, int param) { ::glFogi(static_cast<GLenum>(pname), param); }
    static void glFogf(int pname, float param) { ::glFogf(static_cast<GLenum>(pname), param); }
    static void glFogfv(int pname, const float* params) { ::glFogfv(static_cast<GLenum>(pname), params); }
    static void glColorMaterial(int face, int mode)
    {
        ::glColorMaterial(static_cast<GLenum>(face), static_cast<GLenum>(mode));
    }
    static void glLightfv(int light, int pname, const float* params)
    {
        ::glLightfv(static_cast<GLenum>(light), static_cast<GLenum>(pname), params);
    }
    static void glLightModelfv(int pname, const float* params)
    {
        ::glLightModelfv(static_cast<GLenum>(pname), params);
    }
    static void glEnableClientState(int array) { ::glEnableClientState(static_cast<GLenum>(array)); }
    static void glDisableClientState(int array) { ::glDisableClientState(static_cast<GLenum>(array)); }
    static void glVertexPointer(int size, int type, int stride, const void* pointer)
    {
        ::glVertexPointer(size, static_cast<GLenum>(type), stride, pointer);
    }
    static void glTexCoordPointer(int size, int type, int stride, const void* pointer)
    {
        ::glTexCoordPointer(size, static_cast<GLenum>(type), stride, pointer);
    }
    static void glColorPointer(int size, int type, int stride, const void* pointer)
    {
        ::glColorPointer(size, static_cast<GLenum>(type), stride, pointer);
    }
    static void glNormalPointer(int type, int stride, const void* pointer)
    {
        ::glNormalPointer(static_cast<GLenum>(type), stride, pointer);
    }
    static void glDrawArrays(int mode, int first, int count)
    {
        ::glDrawArrays(static_cast<GLenum>(mode), first, count);
    }
    static void glPolygonOffset(float factor, float units) { ::glPolygonOffset(factor, units); }
    static void glLineWidth(float width) { ::glLineWidth(width); }
    static void glDepthFunc(int func) { ::glDepthFunc(static_cast<GLenum>(func)); }
    static void glCullFace(int mode) { ::glCullFace(static_cast<GLenum>(mode)); }
    static int glGetError() { return static_cast<int>(::glGetError()); }
#else
    static void glEnable(int) {}
    static void glDisable(int) {}
    static void glBlendFunc(int, int) {}
    static void glColor4f(float, float, float, float) {}
    static void glColor3f(float, float, float) {}
    static void glCallLists(int, unsigned int, const void*) {}
    static void glColor4ub(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) {}
    static void glShadeModel(int) {}
    static void glDepthMask(bool) {}
    static void glColorMask(bool, bool, bool, bool) {}
    static void glPushMatrix() {}
    static void glPopMatrix() {}
    static void glLoadIdentity() {}
    static void glMatrixMode(int) {}
    static void glTranslatef(float, float, float) {}
    static void glScalef(float, float, float) {}
    static void glRotatef(float, float, float, float) {}
    static void glBindTexture(int, int) {}
    static void glGenTextures(int, unsigned int*) {}
    static void glTexParameteri(int, int, int) {}
    static void glTexImage2D(int, int, int, int, int, int, int, int, const void*) {}
    static void glTexSubImage2D(int, int, int, int, int, int, int, int, const void*) {}
    static void glDeleteTextures(int, const unsigned int*) {}
    static int glGenLists(int) { return 0; }
    static void glNewList(int, int) {}
    static void glEndList() {}
    static void glCallList(int) {}
    static void glDeleteLists(int, int) {}
    static void glAlphaFunc(int, float) {}
    static void glClear(int) {}
    static void glClearDepth(double) {}
    static void glClearColor(float, float, float, float) {}
    static void glViewport(int, int, int, int) {}
    static void glDrawBuffer(int) {}
    static void glReadBuffer(int) {}
    static void glOrtho(double, double, double, double, double, double) {}
    static void glBegin(int) {}
    static void glEnd() {}
    static void glTexCoord2d(double, double) {}
    static void glVertex3d(double, double, double) {}
    static void glNormal3b(std::int8_t, std::int8_t, std::int8_t) {}
    static void glGetFloatv(int, float*) {}
    static void glScaled(double, double, double) {}
    static void glNormal3f(float, float, float) {}
    static void glFogi(int, int) {}
    static void glFogf(int, float) {}
    static void glFogfv(int, const float*) {}
    static void glColorMaterial(int, int) {}
    static void glLightfv(int, int, const float*) {}
    static void glLightModelfv(int, const float*) {}
    static void glEnableClientState(int) {}
    static void glDisableClientState(int) {}
    static void glVertexPointer(int, int, int, const void*) {}
    static void glTexCoordPointer(int, int, int, const void*) {}
    static void glColorPointer(int, int, int, const void*) {}
    static void glNormalPointer(int, int, const void*) {}
    static void glDrawArrays(int, int, int) {}
    static void glPolygonOffset(float, float) {}
    static void glLineWidth(float) {}
    static void glDepthFunc(int) {}
    static void glCullFace(int) {}
    static int glGetError() { return 0; }
#endif
};

} // namespace net::minecraft::client::gl

namespace gl = net::minecraft::client::gl;
