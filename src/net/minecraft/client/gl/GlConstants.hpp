#pragma once
#include <cstdint>
#if !defined(_WIN32)
#error "OpenGL rendering requires Windows"
#endif
#define MINECRAFT_GL_REAL 1
#include <GL/gl.h>
#include <windows.h>
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
#undef GL_POLYGON_BIT
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
#undef GL_POINTS
#undef GL_VIEWPORT
#undef GL_TEXTURE0
#undef GL_TEXTURE_BINDING_2D
#undef GL_QUAD_STRIP
#undef GL_LINE_LOOP
namespace net::minecraft::client::gl {
namespace cap {
inline constexpr int Blend = 0x0BE2;
inline constexpr int Texture2D = 0x0DE1;
inline constexpr int AlphaTest = 0x0BC0;
inline constexpr int DepthTest = 0x0B71;
inline constexpr int CullFace = 0x0B44;
inline constexpr int ScissorTest = 0x0C11;
inline constexpr int Lighting = 0x0B50;
inline constexpr int ColorMaterial = 0x0B57;
inline constexpr int Fog = 0x0B60;
inline constexpr int RescaleNormal = 0x803A;
inline constexpr int Normalize = 0x0BA1;
inline constexpr int PolygonOffsetFill = 0x8037;
} // namespace cap
namespace prim {
inline constexpr int Quads = 0x0007;
inline constexpr int Triangles = 0x0004;
inline constexpr int Lines = 0x0001;
inline constexpr int LineStrip = 0x0003;
inline constexpr int TriangleFan = 0x0006;
} // namespace prim
namespace blend {
inline constexpr int SrcAlpha = 0x0302;
inline constexpr int OneMinusSrcAlpha = 0x0303;
inline constexpr int OneMinusSrcColor = 0x0301;
inline constexpr int OneMinusDstColor = 0x0307;
inline constexpr int SrcColor = 0x0300;
inline constexpr int DstAlpha = 0x0304;
inline constexpr int One = 0x0001;
inline constexpr int Zero = 0x0000;
} // namespace blend
namespace matrix_ {
inline constexpr int ModelView = 0x1700;
inline constexpr int Projection = 0x1701;
inline constexpr int ModelViewMatrix = 0x0BA6;
inline constexpr int ProjectionMatrix = 0x0BA7;
} // namespace matrix_
namespace shade {
inline constexpr int Smooth = 0x1D01;
inline constexpr int Flat = 0x1D00;
} // namespace shade
namespace compare {
inline constexpr int Greater = 0x0204;
inline constexpr int Gequal = 0x0206;
inline constexpr int Lequal = 0x0203;
inline constexpr int Equal = 0x0202;
} // namespace compare
namespace face {
inline constexpr int Back = 0x0405;
inline constexpr int Front = 0x0404;
} // namespace face
namespace filter {
inline constexpr int Nearest = 0x2600;
inline constexpr int Linear = 0x2601;
inline constexpr int NearestMipmapLinear = 0x2702;
inline constexpr int LinearMipmapLinear = 0x2703;
} // namespace filter
namespace wrap {
inline constexpr int Clamp = 0x2900;
inline constexpr int ClampToEdge = 0x812F;
inline constexpr int Repeat = 0x2901;
} // namespace wrap
namespace pixel {
inline constexpr int Rgba = 0x1908;
inline constexpr int UnsignedByte = 0x1401;
inline constexpr int UnsignedInt = 0x1405;
inline constexpr int Byte = 0x1400;
inline constexpr int Float = 0x1406;
inline constexpr int Double = 0x140A;
inline constexpr int DepthComponent = 0x1902;
inline constexpr int DepthComponent24 = 0x81A6;
inline constexpr int Rgba32f = 0x8814;
inline constexpr int Rgba32ui = 0x8D70;
inline constexpr int RgbaInteger = 0x8D99;
inline constexpr int Rgba16f = 0x881A;
} // namespace pixel
namespace framebuffer {
inline constexpr int Framebuffer = 0x8D40;
inline constexpr int Renderbuffer = 0x8D41;
inline constexpr int ColorAttachment0 = 0x8CE0;
inline constexpr int DepthAttachment = 0x8D00;
inline constexpr int DepthStencilAttachment = 0x821A;
inline constexpr int Depth24Stencil8 = 0x88F0;
inline constexpr int Complete = 0x8CD5;
} // namespace framebuffer
namespace attrib {
inline constexpr int ColorBufferBit = 0x00004000;
inline constexpr int DepthBufferBit = 0x00000100;
} // namespace attrib
namespace light {
inline constexpr int Ambient = 0x1200;
inline constexpr int AmbientAndDiffuse = 0x1602;
} // namespace light
namespace fog {
inline constexpr int Color = 0x0B66;
inline constexpr int Mode = 0x0B65;
inline constexpr int Density = 0x0B62;
inline constexpr int Start = 0x0B63;
inline constexpr int End = 0x0B64;
} // namespace fog
namespace tex {
inline constexpr int MinFilter = 0x2801;
inline constexpr int MagFilter = 0x2800;
inline constexpr int WrapS = 0x2802;
inline constexpr int WrapT = 0x2803;
inline constexpr int MaxLevel = 0x813D;
inline constexpr int Texture0 = 0x84C0;
inline constexpr int Binding2D = 0x8069;
inline constexpr int ActiveTexture = 0x84E0;
inline constexpr int MaxTextureImageUnits = 0x8872;
} // namespace tex
namespace query {
inline constexpr int CurrentColor = 0x0B00;
inline constexpr int MatrixMode = 0x0BA0;
inline constexpr int Viewport = 0x0BA2;
inline constexpr int FramebufferBinding = 0x8CA6;
inline constexpr int ShadeModel = 0x0B54;
inline constexpr int BlendSrc = 0x0BE1;
inline constexpr int BlendDst = 0x0BE0;
inline constexpr int DepthFunc = 0x0B74;
inline constexpr int AlphaFunc = 0x0BC1;
inline constexpr int AlphaRef = 0x0BC2;
} // namespace query
} // namespace net::minecraft::client::gl
