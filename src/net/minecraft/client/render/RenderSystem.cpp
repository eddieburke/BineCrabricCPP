#include "net/minecraft/client/render/RenderSystem.hpp"
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
namespace math = net::minecraft::util::math;
static RenderSystem::StateShadow g_shadow;
static math::MatrixStack* g_activeMatrix = &math::g_modelView;
static std::mutex g_textureMutex;
static std::vector<unsigned int> g_allocatedTextures;
static std::unordered_map<unsigned, int> g_textureUnitOf;
RenderSystem::StateShadow RenderSystem::getShadow() {
 return g_shadow;
}
void RenderSystem::setShadow(const RenderSystem::StateShadow& shadow) {
 if(shadow.blend)
  enableBlend();
 else
  disableBlend();
 blendFunc(shadow.blendSrc, shadow.blendDst);
 if(shadow.depthTest)
  enableDepthTest();
 else
  disableDepthTest();
 depthFunc(shadow.depthFunc);
 depthMask(shadow.depthWrite);
 if(shadow.cullFace)
  enableCull();
 else
  disableCull();
 cullFace(shadow.cullFaceMode);
 if(shadow.polygonOffset)
  enablePolygonOffset();
 else
  disablePolygonOffset();
 polygonOffset(shadow.polygonFactor, shadow.polygonUnits);
 colorMask(shadow.colorMaskR, shadow.colorMaskG, shadow.colorMaskB, shadow.colorMaskA);
 viewport(shadow.viewport[0], shadow.viewport[1], shadow.viewport[2], shadow.viewport[3]);
 for(int i = 0; i < 32; ++i) {
  const unsigned int tex = shadow.boundTextures[i];
  activeTexture(0x84C0 + i);
  if(tex != 0 && ::glIsTexture(tex) != 0) {
   ::glBindTexture(0x0DE1, tex);
   g_shadow.boundTextures[i] = tex;
  } else {
   ::glBindTexture(0x0DE1, 0);
   g_shadow.boundTextures[i] = 0;
  }
 }
 activeTexture(0x84C0 + shadow.activeTexture);
 color4f(shadow.constColor[0], shadow.constColor[1], shadow.constColor[2], shadow.constColor[3]);
 alphaFunc(shadow.alphaFunc, shadow.alphaRef);
 g_shadow.texture2D = shadow.texture2D;
 g_shadow.fogEnabled = shadow.fogEnabled;
 auto fog = gl::engine_pipeline::fog();
 if(shadow.fogEnabled != fog.enabled) {
  fog.enabled = shadow.fogEnabled;
  gl::engine_pipeline::setFog(fog);
 }
}
void RenderSystem::pushMatrix() {
 g_activeMatrix->push();
}
void RenderSystem::popMatrix() {
 g_activeMatrix->pop();
}
void RenderSystem::loadIdentity() {
 g_activeMatrix->loadIdentity();
}
void RenderSystem::translate(float x, float y, float z) {
 g_activeMatrix->translate(x, y, z);
}
void RenderSystem::scale(float x, float y, float z) {
 g_activeMatrix->scale(x, y, z);
}
void RenderSystem::rotate(float angle, float x, float y, float z) {
 g_activeMatrix->rotate(angle, x, y, z);
}
void RenderSystem::matrixMode(int mode) {
  // Only the modelview and projection stacks exist in this port. GL_TEXTURE_MATRIX
  // (0x170B, used by some legacy callers e.g. the charged-creeper aura) has no
  // equivalent here — silently ignore it rather than misrouting it onto the
  // modelview stack, where a follow-up loadIdentity()/translate() would corrupt
  // every subsequent world draw (water/clouds rendering locked to an entity).
  if(mode == 0x170B) {
    return;
  }
  g_activeMatrix = (mode == 0x1701) ? &math::g_projection : &math::g_modelView; // GL_PROJECTION
}
void RenderSystem::ortho(double left, double right, double bottom, double top, double zNear, double zFar) {
 math::Matrix4f m;
 m.ortho(static_cast<float>(left),
         static_cast<float>(right),
         static_cast<float>(bottom),
         static_cast<float>(top),
         static_cast<float>(zNear),
         static_cast<float>(zFar));
 g_activeMatrix->multiply(m);
}
void RenderSystem::enableBlend() {
 if(!g_shadow.blend) {
  g_shadow.blend = true;
  ::glEnable(0x0BE2); // GL_BLEND
 }
}
void RenderSystem::disableBlend() {
 if(g_shadow.blend) {
  g_shadow.blend = false;
  ::glDisable(0x0BE2);
 }
}
void RenderSystem::blendFunc(int src, int dst) {
 if(g_shadow.blendSrc != src || g_shadow.blendDst != dst) {
  g_shadow.blendSrc = src;
  g_shadow.blendDst = dst;
  ::glBlendFunc(static_cast<unsigned>(src), static_cast<unsigned>(dst));
 }
}
void RenderSystem::defaultBlendFunc() {
 blendFunc(0x0302, 0x0303); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
}
void RenderSystem::blendAlpha() {
 enableBlend();
 blendFunc(0x0302, 0x0303); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
}
void RenderSystem::blendAdditive() {
 enableBlend();
 blendFunc(0x0001, 0x0001); // GL_ONE, GL_ONE
}
void RenderSystem::blendCustom(int src, int dst) {
 enableBlend();
 blendFunc(src, dst);
}
void RenderSystem::blendDstAlpha() {
 enableBlend();
 blendFunc(0x0302, 0x0304); // GL_SRC_ALPHA, GL_DST_ALPHA
}
void RenderSystem::blendInverseColor() {
 enableBlend();
 blendFunc(0x0000, 0x0301); // GL_ZERO, GL_ONE_MINUS_SRC_COLOR
}
void RenderSystem::blendMultiply() {
 enableBlend();
 blendFunc(0x0307, 0x0301); // GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR
}
void RenderSystem::enableDepthTest() {
 if(!g_shadow.depthTest) {
  g_shadow.depthTest = true;
  ::glEnable(0x0B71); // GL_DEPTH_TEST
 }
}
void RenderSystem::depthTest() {
 enableDepthTest();
 depthFunc(0x0203); // GL_LEQUAL
}
void RenderSystem::depthTestWrite(bool write) {
 enableDepthTest();
 depthFunc(0x0203); // GL_LEQUAL
 depthMask(write);
}
void RenderSystem::disableDepthTest() {
 if(g_shadow.depthTest) {
  g_shadow.depthTest = false;
  ::glDisable(0x0B71);
 }
}
void RenderSystem::depthFunc(int func) {
 if(g_shadow.depthFunc != func) {
  g_shadow.depthFunc = func;
  ::glDepthFunc(static_cast<unsigned>(func));
 }
}
void RenderSystem::depthMask(bool enabled) {
 if(g_shadow.depthWrite != enabled) {
  g_shadow.depthWrite = enabled;
  ::glDepthMask(enabled ? 1 : 0);
 }
}
void RenderSystem::enableCull() {
 if(!g_shadow.cullFace) {
  g_shadow.cullFace = true;
  ::glEnable(0x0B44); // GL_CULL_FACE
 }
}
void RenderSystem::disableCull() {
 if(g_shadow.cullFace) {
  g_shadow.cullFace = false;
  ::glDisable(0x0B44);
 }
}
void RenderSystem::cullFace(int mode) {
 if(g_shadow.cullFaceMode != mode) {
  g_shadow.cullFaceMode = mode;
  ::glCullFace(static_cast<unsigned>(mode));
 }
}
void RenderSystem::cullBackFaces() {
 enableCull();
 cullFace(0x0405); // GL_BACK
}
void RenderSystem::cullFrontFaces() {
 enableCull();
 cullFace(0x0404); // GL_FRONT
}
void RenderSystem::enablePolygonOffset() {
 if(!g_shadow.polygonOffset) {
  g_shadow.polygonOffset = true;
  ::glEnable(0x8037); // GL_POLYGON_OFFSET_FILL
 }
}
void RenderSystem::disablePolygonOffset() {
 if(g_shadow.polygonOffset) {
  g_shadow.polygonOffset = false;
  ::glDisable(0x8037);
 }
}
void RenderSystem::polygonOffset(float factor, float units) {
 if(g_shadow.polygonFactor != factor || g_shadow.polygonUnits != units) {
  g_shadow.polygonFactor = factor;
  g_shadow.polygonUnits = units;
  ::glPolygonOffset(factor, units);
 }
}
void RenderSystem::clearColor(float r, float g, float b, float a) {
 if(g_shadow.clearColor[0] != r || g_shadow.clearColor[1] != g || g_shadow.clearColor[2] != b || g_shadow.clearColor[3] != a) {
  g_shadow.clearColor[0] = r;
  g_shadow.clearColor[1] = g;
  g_shadow.clearColor[2] = b;
  g_shadow.clearColor[3] = a;
  ::glClearColor(r, g, b, a);
 }
}
void RenderSystem::clear(int mask) {
 ::glClear(static_cast<unsigned>(mask));
}
void RenderSystem::clearDepth(double depth) {
 ::glClearDepth(depth);
}
void RenderSystem::activeTexture(int texture) {
 int unit = texture - 0x84C0; // GL_TEXTURE0
 if(g_shadow.activeTexture != unit) {
  g_shadow.activeTexture = unit;
  if(gl::GLCore::activeTexture != nullptr) {
   reinterpret_cast<void(APIENTRY*)(unsigned)>(gl::GLCore::activeTexture)(static_cast<unsigned>(texture));
  }
 }
}
void RenderSystem::bindTexture(int texture) {
 bindTexture(0x0DE1, texture);
}
void RenderSystem::bindTexture(unsigned int texture) {
 bindTexture(0x0DE1, static_cast<int>(texture));
}
void RenderSystem::bindTexture(int target, int texture) {
 if(texture < 0) {
  texture = 0;
 }
 const unsigned int uTex = static_cast<unsigned int>(texture);
 if(target == 0x0DE1 && g_shadow.activeTexture >= 0 && g_shadow.activeTexture < 32) {
  if(g_shadow.boundTextures[g_shadow.activeTexture] == uTex) {
   return;
  }
  g_shadow.boundTextures[g_shadow.activeTexture] = uTex;
  if(uTex != 0) {
   g_textureUnitOf[uTex] = g_shadow.activeTexture;
  }
 }
 ::glBindTexture(static_cast<unsigned>(target), uTex);
}
void RenderSystem::unbindTexture(int texture) {
 if(texture <= 0) return;
 const unsigned uTex = static_cast<unsigned>(texture);
 const auto it = g_textureUnitOf.find(uTex);
 if(it != g_textureUnitOf.end()) {
  const int unit = it->second;
  g_textureUnitOf.erase(it);
  if(unit >= 0 && unit < 32 && g_shadow.boundTextures[unit] == uTex) {
   activeTexture(0x84C0 + unit);
   ::glBindTexture(0x0DE1, 0);
   g_shadow.boundTextures[unit] = 0;
  }
  return;
 }
 for(int i = 0; i < 32; ++i) {
  if(g_shadow.boundTextures[i] == uTex) {
   activeTexture(0x84C0 + i);
   ::glBindTexture(0x0DE1, 0);
   g_shadow.boundTextures[i] = 0;
  }
 }
}
unsigned int RenderSystem::genTexture() {
 std::lock_guard lock(g_textureMutex);
 unsigned int tex = 0;
 ::glGenTextures(1, &tex);
 g_allocatedTextures.push_back(tex);
 return tex;
}
void RenderSystem::deleteTexture(unsigned int texture) {
 if(texture == 0) return;
 unbindTexture(static_cast<int>(texture));
 {
  std::lock_guard lock(g_textureMutex);
  std::erase(g_allocatedTextures, texture);
 }
 ::glDeleteTextures(1, &texture);
}
void RenderSystem::clearAllocatedTextures() {
 std::lock_guard lock(g_textureMutex);
 if(!g_allocatedTextures.empty()) {
  ::glDeleteTextures(static_cast<int>(g_allocatedTextures.size()), g_allocatedTextures.data());
  g_allocatedTextures.clear();
 }
}
void RenderSystem::enableTexture() {
 g_shadow.texture2D = true;
}
void RenderSystem::disableTexture() {
 g_shadow.texture2D = false;
}
void RenderSystem::enableLighting() {
 gl::engine_pipeline::setLightingEnabled(true);
}
void RenderSystem::disableLighting() {
 gl::engine_pipeline::setLightingEnabled(false);
}
void RenderSystem::setupGui3DLiveLighting() {
 enableLighting();
}
void RenderSystem::setupGuiFlatItemLighting() {
 // GUI item icons render with an ortho projection and no camera, so the per-frame
 // world-sun light (view-space direction from the last 3D pass) is meaningless here —
 // it made icons flicker/shade based on time of day and look direction. Use a fixed,
 // camera-independent key light instead, approximating vanilla's two-light GUI rig
 // (Lighting.turnOn: light0 dir (0.2, 1.0, -0.7), diffuse 0.6, ambient 0.4).
 // Direction chosen against the post-transform view-space normals of the GUI block
 // icon (scale(1,1,-1), rotate 210 X, rotate -45 Y): top face ~(0,-0.87,0.5),
 // sides ~(+-0.71,0.35,0.61). This puts the top at full brightness and gives the
 // two visible sides distinct, vanilla-like shading.
 static const float kDir[3] = {0.288675f, -0.827512f, 0.481125f}; // normalize(0.3, -0.86, 0.5)
 gl::engine_pipeline::WorldLightUniforms guiLight;
 guiLight.sunDirView[0] = kDir[0];
 guiLight.sunDirView[1] = kDir[1];
 guiLight.sunDirView[2] = kDir[2];
 guiLight.sunColor[0] = guiLight.sunColor[1] = guiLight.sunColor[2] = 1.0f;
  guiLight.sunIntensity = 0.6f;
  guiLight.ambient[0] = guiLight.ambient[1] = guiLight.ambient[2] = 0.55f;
 gl::engine_pipeline::setWorldLight(guiLight);
 enableLighting();
}
void RenderSystem::colorMask(bool r, bool g, bool b, bool a) {
 if(g_shadow.colorMaskR != r || g_shadow.colorMaskG != g || g_shadow.colorMaskB != b || g_shadow.colorMaskA != a) {
  g_shadow.colorMaskR = r;
  g_shadow.colorMaskG = g;
  g_shadow.colorMaskB = b;
  g_shadow.colorMaskA = a;
  ::glColorMask(r ? 1 : 0, g ? 1 : 0, b ? 1 : 0, a ? 1 : 0);
 }
}
RenderSystem::ColorMaskScope::ColorMaskScope(bool r, bool g, bool b, bool a)
    : savedR_(g_shadow.colorMaskR), savedG_(g_shadow.colorMaskG),
      savedB_(g_shadow.colorMaskB), savedA_(g_shadow.colorMaskA) {
 RenderSystem::colorMask(r, g, b, a);
}
RenderSystem::ColorMaskScope::~ColorMaskScope() {
 RenderSystem::colorMask(savedR_, savedG_, savedB_, savedA_);
}
void RenderSystem::hintFogEnabled(bool enabled) {
 g_shadow.fogEnabled = enabled;
}
void RenderSystem::viewport(int x, int y, int width, int height) {
 if(!g_shadow.viewportValid || g_shadow.viewport[0] != x || g_shadow.viewport[1] != y || g_shadow.viewport[2] != width || g_shadow.viewport[3] != height) {
  g_shadow.viewport[0] = x;
  g_shadow.viewport[1] = y;
  g_shadow.viewport[2] = width;
  g_shadow.viewport[3] = height;
  g_shadow.viewportValid = true;
  ::glViewport(x, y, width, height);
 }
}
bool RenderSystem::getCachedViewport(int outViewport[4]) {
 if(!g_shadow.viewportValid) {
  return false;
 }
 outViewport[0] = g_shadow.viewport[0];
 outViewport[1] = g_shadow.viewport[1];
 outViewport[2] = g_shadow.viewport[2];
 outViewport[3] = g_shadow.viewport[3];
 return true;
}
void RenderSystem::color4f(float r, float g, float b, float a) {
 g_shadow.constColor[0] = r;
 g_shadow.constColor[1] = g;
 g_shadow.constColor[2] = b;
 g_shadow.constColor[3] = a;
}
void RenderSystem::color3f(float r, float g, float b) {
 color4f(r, g, b, 1.0f);
}
void RenderSystem::alphaFunc(int func, float ref) {
 g_shadow.alphaFunc = func;
 g_shadow.alphaRef = ref;
}
void RenderSystem::alphaTest(float ref) {
 alphaFunc(0x0204, ref); // GL_GREATER
}
void RenderSystem::getFloatv(int pname, float* params) {
 switch(pname) {
 case 0x0B00: // GL_CURRENT_COLOR
  std::memcpy(params, g_shadow.constColor, sizeof(float) * 4);
  return;
 case 0x0BC2: // GL_ALPHA_TEST_REF
  params[0] = g_shadow.alphaRef;
  return;
 case 0xBA6: // GL_MODELVIEW_MATRIX
  std::memcpy(params, math::g_modelView.top().data(), sizeof(float) * 16);
  return;
 case 0xBA7: // GL_PROJECTION_MATRIX
  std::memcpy(params, math::g_projection.top().data(), sizeof(float) * 16);
  return;
 default:
  ::glGetFloatv(static_cast<unsigned>(pname), params);
 }
}
void RenderSystem::getIntegerv(int pname, int* params) {
 switch(pname) {
 case 0x0BA2: // GL_VIEWPORT
  if(g_shadow.viewportValid) {
   std::memcpy(params, g_shadow.viewport, sizeof(int) * 4);
  } else {
   ::glGetIntegerv(static_cast<unsigned>(pname), params);
  }
  return;
 case 0x0BA0: // GL_MATRIX_MODE
  params[0] = (g_activeMatrix == &math::g_projection) ? 0x1701 : 0x1700;
  return;
 case 0x0B54: // GL_SHADE_MODEL
  params[0] = 0x1D01; // GL_SMOOTH
  return;
 case 0x0BC1: // GL_ALPHA_TEST_FUNC
  params[0] = g_shadow.alphaFunc;
  return;
 case 0x0BE1: // GL_BLEND_SRC
  params[0] = g_shadow.blendSrc;
  return;
 case 0x0BE0: // GL_BLEND_DST
  params[0] = g_shadow.blendDst;
  return;
 case 0x0B74: // GL_DEPTH_FUNC
  params[0] = g_shadow.depthFunc;
  return;
 default:
  ::glGetIntegerv(static_cast<unsigned>(pname), params);
 }
}
} // namespace net::minecraft::client::render
