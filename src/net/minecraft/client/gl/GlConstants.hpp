#pragma once
#include "net/minecraft/client/gl/GlPlatform.hpp"
#include <cstdint>
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
inline constexpr int Points = 0x0000;
inline constexpr int LineStrip = 0x0003;
inline constexpr int LineLoop = 0x0002;
inline constexpr int QuadStrip = 0x0008;
inline constexpr int TriangleFan = 0x0006;
} // namespace prim
namespace blend {
inline constexpr int SrcAlpha = 0x0302;
inline constexpr int OneMinusSrcAlpha = 0x0303;
inline constexpr int OneMinusSrcColor = 0x0301;
inline constexpr int OneMinusDstColor = 0x0307;
inline constexpr int SrcColor = 0x0300;
inline constexpr int DstColor = 0x0306;
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
inline constexpr int FrontAndBack = 0x0408;
} // namespace face
namespace filter {
inline constexpr int Nearest = 0x2600;
inline constexpr int Linear = 0x2601;
inline constexpr int NearestMipmapLinear = 0x2702;
inline constexpr int LinearMipmapLinear = 0x2703;
} // namespace filter
namespace wrap {
inline constexpr int Clamp = 0x2900;
inline constexpr int Repeat = 0x2901;
} // namespace wrap
namespace pixel {
inline constexpr int Rgba = 0x1908;
inline constexpr int Rgb = 0x1907;
inline constexpr int UnsignedByte = 0x1401;
inline constexpr int Byte = 0x1400;
inline constexpr int Float = 0x1406;
inline constexpr int Double = 0x140A;
} // namespace pixel
namespace framebuffer {
inline constexpr int Framebuffer = 0x8D40;
inline constexpr int Renderbuffer = 0x8D41;
inline constexpr int ColorAttachment0 = 0x8CE0;
inline constexpr int DepthStencilAttachment = 0x821A;
inline constexpr int Depth24Stencil8 = 0x88F0;
inline constexpr int Complete = 0x8CD5;
} // namespace framebuffer
namespace attrib {
inline constexpr int ColorBufferBit = 0x00004000;
inline constexpr int DepthBufferBit = 0x00000100;
inline constexpr int CurrentBit = 0x00000001;
inline constexpr int PolygonBit = 0x00000008;
inline constexpr int LightingBit = 0x00000040;
inline constexpr int TransformBit = 0x00001000;
inline constexpr int EnableBit = 0x00002000;
inline constexpr int TextureBit = 0x00040000;
inline constexpr int AllAttribBits = 0x000FFFFF;
} // namespace attrib
namespace client_array {
inline constexpr int VertexArray = 0x8074;
inline constexpr int NormalArray = 0x8075;
inline constexpr int ColorArray = 0x8076;
inline constexpr int TextureCoordArray = 0x8078;
} // namespace client_array
namespace light {
inline constexpr int Light0 = 0x4000;
inline constexpr int Light1 = 0x4001;
inline constexpr int Position = 0x1203;
inline constexpr int Diffuse = 0x1201;
inline constexpr int Specular = 0x1202;
inline constexpr int Ambient = 0x1200;
inline constexpr int AmbientAndDiffuse = 0x1602;
inline constexpr int LightModelAmbient = 0x0B53;
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
inline constexpr int Texture0 = 0x84C0;
inline constexpr int Binding2D = 0x8069;
} // namespace tex
namespace query {
inline constexpr int CurrentColor = 0x0B01;
inline constexpr int Viewport = 0x0BA2;
inline constexpr int ShadeModel = 0x0B54;
inline constexpr int BlendSrc = 0x0BE1;
inline constexpr int BlendDst = 0x0BE0;
inline constexpr int DepthFunc = 0x0B74;
inline constexpr int AlphaFunc = 0x0BC1;
inline constexpr int AlphaRef = 0x0BC2;
} // namespace query
} // namespace net::minecraft::client::gl
