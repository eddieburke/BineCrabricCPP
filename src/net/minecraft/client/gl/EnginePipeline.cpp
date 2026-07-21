#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include <cstdint>
#include <cstring>
#include <cmath>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
namespace net::minecraft::client::gl {
namespace math = net::minecraft::util::math;
namespace render = net::minecraft::client::render;
namespace engine_pipeline {
namespace {
WorldLightUniforms g_worldLight{};
FogUniforms g_fog{};
struct UberUniforms {
 bool texture2D = false;
 float constColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
 bool alphaTest = false;
 float alphaRef = 0.1f;
 bool lighting = false;
 float sunDirView[3] = {0.0f, 0.0f, 0.0f};
 float sunColor[3] = {1.0f, 1.0f, 1.0f};
 float sunIntensity = 0.0f;
 float fillDirView[3] = {0.0f, 0.0f, 0.0f};
 float fillIntensity = 0.0f;
 float ambient[3] = {1.0f, 1.0f, 1.0f};
 float worldTime = 0.0f;
 float brightness = 0.0f;
 int fogMode = 0;
 float fogColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
 float fogDensity = 0.0f;
 float fogStart = 0.0f;
 float fogEnd = 0.0f;
};
const float kDefaultNormal[3] = {0.0f, 0.0f, 0.0f};
constexpr unsigned int kArrayBuffer = 0x8892; // GL_ARRAY_BUFFER
constexpr unsigned int kStreamDraw = 0x88E0; // GL_STREAM_DRAW
constexpr unsigned int kFloat = 0x1406; // GL_FLOAT
constexpr unsigned int kUnsignedByte = 0x1401; // GL_UNSIGNED_BYTE
constexpr unsigned int kByte = 0x1400; // GL_BYTE
const char* kVersion = "#version 330 core\n";
// Byte offsets within TessellatorVertex (pos, uv, color, normal); stride is 28.
constexpr std::size_t kOffPos = 0;
constexpr std::size_t kOffUV = 12;
constexpr std::size_t kOffColor = 20;
constexpr std::size_t kOffNormal = 24;
constexpr std::size_t kOffLight = 28;
struct AttribCache {
 unsigned buffer = 0;
 std::size_t baseOffset = 0;
 int stride = 0;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 bool hasLightmap = false;
 bool valid = false;
};
AttribCache g_attribCache;
float g_constNormal[3] = {0.0f, 0.0f, 0.0f};
bool g_constNormalSet = false;
ShaderProgram* g_activeProgram = nullptr;
ShaderProgram* g_lastProgram = nullptr;
unsigned int g_vao = 0;
unsigned int g_streamVbo = 0;
std::size_t g_streamCapacity = 0;
// Fullscreen-triangle blit resources.
ShaderProgram g_blit;
bool g_triedBlit = false;
unsigned int g_blitVao = 0;
unsigned int g_blitVbo = 0;
const char* kBlitVsh =
    "in vec2 aPos;\n"
    "in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main(){ vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
const char* kBlitFsh =
    "in vec2 vUV;\n"
    "uniform sampler2D uTexture;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = texture(uTexture, vUV); }\n";
const void* bufOffset(std::size_t o) {
 return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(o));
}
void ensureVao() {
 if(g_vao == 0 && GLCore::vaoSupported) {
  GLCore::genVertexArrays(1, &g_vao);
 }
 if(g_streamVbo == 0 && GLCore::genBuffers != nullptr) {
  GLCore::genBuffers(1, &g_streamVbo);
 }
}
void uploadStreaming(const void* data, std::size_t bytes) {
 GLCore::bindBuffer(kArrayBuffer, g_streamVbo);
 if(bytes <= g_streamCapacity) {
  GLCore::bufferSubData(kArrayBuffer, 0, static_cast<intptr_t>(bytes), data);
 } else {
  GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(bytes), data, kStreamDraw);
  g_streamCapacity = bytes;
 }
}
bool g_blitReady = false;
bool ensureBlitResources() {
 GLCore::ensureLoaded();
 if(!GLCore::shaderSupported || !GLCore::vaoSupported) {
  return false;
 }
 if(!g_triedBlit) {
  g_triedBlit = true;
  g_blit.compile(kBlitVsh, kBlitFsh, kVersion);
  GLCore::genVertexArrays(1, &g_blitVao);
  GLCore::genBuffers(1, &g_blitVbo);
  const float tri[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,
      3.0f, -1.0f, 2.0f, 0.0f,
      -1.0f, 3.0f, 0.0f, 2.0f};
  GLCore::bindBuffer(kArrayBuffer, g_blitVbo);
  GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(sizeof(tri)), tri, 0x88E4);
  if(g_blit.valid() && g_blitVao != 0) {
   GLCore::bindVertexArray(g_blitVao);
   const int stride = 4 * static_cast<int>(sizeof(float));
   GLCore::enableVertexAttribArray(0);
   GLCore::vertexAttribPointer(0, 2, kFloat, 0, stride, bufOffset(0));
   GLCore::enableVertexAttribArray(1);
   GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(2 * sizeof(float)));
   GLCore::bindVertexArray(0);
   GLCore::bindBuffer(kArrayBuffer, 0);
   g_blitReady = true;
  }
 }
 return g_blitReady;
}
} // namespace
bool ensureReady() {
 GLCore::ensureLoaded();
 if(!GLCore::shaderSupported || !GLCore::vaoSupported) {
  return false;
 }
 ensureVao();
 return g_vao != 0;
}
ShaderProgram* program() {
 return g_activeProgram;
}
void bindAndUploadUniforms(const RenderPass& pass) {
 ShaderProgram* active = pass.programOverride != nullptr
                             ? pass.programOverride
                             : g_activeProgram;
 if(active == nullptr) {
  return;
 }
 static UberUniforms s_snapshot{};
 static math::Matrix4f s_lastModelView{};
 static math::Matrix4f s_lastProjection{};
 active->bind();
 const render::RenderSystem::StateShadow shadow = render::RenderSystem::getShadow();
 UberUniforms p{};
 p.texture2D = shadow.texture2D;
 p.constColor[0] = shadow.constColor[0];
 p.constColor[1] = shadow.constColor[1];
 p.constColor[2] = shadow.constColor[2];
 p.constColor[3] = shadow.constColor[3];
 p.alphaRef = shadow.alphaRef;
 constexpr int kAlphaAlways = 0x0207; // GL_ALWAYS
 p.alphaTest = shadow.alphaFunc != kAlphaAlways;
 p.lighting = g_worldLight.enabled;
 std::memcpy(p.sunDirView, g_worldLight.sunDirView, sizeof(p.sunDirView));
 std::memcpy(p.sunColor, g_worldLight.sunColor, sizeof(p.sunColor));
 p.sunIntensity = g_worldLight.sunIntensity;
 std::memcpy(p.fillDirView, g_worldLight.fillDirView, sizeof(p.fillDirView));
 p.fillIntensity = g_worldLight.fillIntensity;
 std::memcpy(p.ambient, g_worldLight.ambient, sizeof(p.ambient));
 p.brightness = g_worldLight.brightness;
 p.worldTime = g_worldLight.worldTime;
 const FogUniforms& fog = pass.fog.enabled ? pass.fog : g_fog;
 p.fogMode = fog.enabled ? fog.mode : 0;
 p.fogColor[0] = fog.color[0];
 p.fogColor[1] = fog.color[1];
 p.fogColor[2] = fog.color[2];
 p.fogColor[3] = fog.color[3];
 p.fogDensity = fog.density;
 p.fogStart = fog.start;
 p.fogEnd = fog.end;
 const bool programChanged = (active != g_lastProgram);
 const math::Matrix4f& modelView = pass.modelView;
 const math::Matrix4f& projection = pass.projection;
 if(programChanged || std::memcmp(&s_lastModelView, &modelView, sizeof(math::Matrix4f)) != 0) {
  active->setMatrix4("uModelView", modelView);
  s_lastModelView = modelView;
 }
 if(programChanged || std::memcmp(&s_lastProjection, &projection, sizeof(math::Matrix4f)) != 0) {
  active->setMatrix4("uProjection", projection);
  s_lastProjection = projection;
 }
 if(programChanged) {
  active->set1i("uTexture", 0);
  active->set1i("uLightMap", 1);
 }
 if(programChanged || p.texture2D != s_snapshot.texture2D) {
  active->set1i("uUseTexture", p.texture2D ? 1 : 0);
 }
 if(programChanged || std::memcmp(p.constColor, s_snapshot.constColor, sizeof(p.constColor)) != 0) {
  active->set4f("uConstColor", p.constColor[0], p.constColor[1], p.constColor[2], p.constColor[3]);
 }
 if(programChanged || p.alphaTest != s_snapshot.alphaTest) {
  active->set1i("uAlphaTest", p.alphaTest ? 1 : 0);
 }
 if(programChanged || p.alphaRef != s_snapshot.alphaRef) {
  active->set1f("uAlphaRef", p.alphaRef);
 }
 if(programChanged || p.lighting != s_snapshot.lighting) {
  active->set1i("uLighting", p.lighting ? 1 : 0);
 }
 if(programChanged || p.fogMode != s_snapshot.fogMode) {
  active->set1i("uFogMode", p.fogMode);
 }
 if(programChanged || std::memcmp(p.fogColor, s_snapshot.fogColor, sizeof(p.fogColor)) != 0) {
  active->set4f("uFogColor", p.fogColor[0], p.fogColor[1], p.fogColor[2], p.fogColor[3]);
 }
 if(programChanged || p.fogDensity != s_snapshot.fogDensity) {
  active->set1f("uFogDensity", p.fogDensity);
 }
 if(programChanged || p.fogStart != s_snapshot.fogStart) {
  active->set1f("uFogStart", p.fogStart);
 }
 if(programChanged || p.fogEnd != s_snapshot.fogEnd) {
  active->set1f("uFogEnd", p.fogEnd);
 }
 if(programChanged || std::memcmp(p.sunDirView, s_snapshot.sunDirView, sizeof(p.sunDirView)) != 0) {
  active->set3f("uSunDirectionView", p.sunDirView[0], p.sunDirView[1], p.sunDirView[2]);
 }
 if(programChanged || std::memcmp(p.sunColor, s_snapshot.sunColor, sizeof(p.sunColor)) != 0) {
  active->set3f("uSunColor", p.sunColor[0], p.sunColor[1], p.sunColor[2]);
 }
 if(programChanged || p.sunIntensity != s_snapshot.sunIntensity) {
  active->set1f("uSunIntensity", p.sunIntensity);
 }
 if(programChanged || std::memcmp(p.fillDirView, s_snapshot.fillDirView, sizeof(p.fillDirView)) != 0) {
  active->set3f("uFillDirectionView", p.fillDirView[0], p.fillDirView[1], p.fillDirView[2]);
 }
 if(programChanged || p.fillIntensity != s_snapshot.fillIntensity) {
  active->set1f("uFillIntensity", p.fillIntensity);
 }
 if(programChanged || std::memcmp(p.ambient, s_snapshot.ambient, sizeof(p.ambient)) != 0) {
  active->set3f("uAmbient", p.ambient[0], p.ambient[1], p.ambient[2]);
 }
 if(programChanged || p.worldTime != s_snapshot.worldTime) {
  active->set1f("uWorldTime", p.worldTime);
 }
 if(programChanged || p.brightness != s_snapshot.brightness) {
  active->set1f("uBrightness", p.brightness);
 }
 s_snapshot = p;
 g_lastProgram = active;
}
void submit(const RenderPass& pass) {
 if(!ensureReady() || pass.vertexCount == 0) {
  return;
 }
 bindAndUploadUniforms(pass);
 if(pass.programOverride == nullptr && g_activeProgram == nullptr) {
  return;
 }
 if(pass.buffer != 0) {
  configureAttribs(pass.buffer, pass.byteOffset, pass.stride, pass.hasTexture, pass.hasColor, pass.hasNormals, pass.hasLightmap);
  ::glDrawArrays(static_cast<GLenum>(pass.glMode), 0, static_cast<GLsizei>(pass.vertexCount));
 } else {
  uploadStreaming(pass.vertexData, pass.vertexCount * static_cast<std::size_t>(pass.stride));
  configureAttribs(g_streamVbo, 0, pass.stride, pass.hasTexture, pass.hasColor, pass.hasNormals, pass.hasLightmap);
  ::glDrawArrays(static_cast<GLenum>(pass.glMode), 0, static_cast<GLsizei>(pass.vertexCount));
 }
}
void submitFullscreenPass(const RenderPass& pass) {
 invalidateAttribCache();
 bindAndUploadUniforms(pass);
 if(pass.programOverride == nullptr && g_activeProgram == nullptr) {
  return;
 }
 drawFullscreenTriangle();
}
void setActiveProgram(ShaderProgram* program) {
 g_activeProgram = program;
 g_lastProgram = nullptr;
}
ShaderProgram* activeProgram() {
 return g_activeProgram;
}
ShaderProgram* getActiveProgram() {
 return g_activeProgram;
}
void setWorldLight(const WorldLightUniforms& light) {
 // The enabled flag is owned by RenderSystem's lighting toggles, not the per-frame
 // sun data, so preserve it here.
 const bool enabled = g_worldLight.enabled;
 g_worldLight = light;
 g_worldLight.enabled = enabled;
}
void setLightingEnabled(bool enabled) {
 g_worldLight.enabled = enabled;
}
void setFog(const FogUniforms& fog) {
 g_fog = fog;
}
const FogUniforms& fog() {
 return g_fog;
}
void configureAttribs(unsigned buffer,
                      std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals,
                      bool hasLightmap) {
 // TessellatorVertex carries no lightmap: kOffLight is exactly its size, so a
 // caller passing hasLightmap with that stride would point attribute 4 past the
 // end of every vertex and fetch out-of-bounds data. Require the stride to
 // actually contain the two floats before wiring it up.
 const bool lightmapInStride =
     hasLightmap && static_cast<std::size_t>(stride) >= kOffLight + 2 * sizeof(float);
 const bool cached = g_attribCache.valid && buffer != 0 && g_attribCache.buffer == buffer &&
                     g_attribCache.baseOffset == baseOffset && g_attribCache.stride == stride &&
                     g_attribCache.hasTexture == hasTexture && g_attribCache.hasColor == hasColor &&
                     g_attribCache.hasNormals == hasNormals && g_attribCache.hasLightmap == lightmapInStride;
 if(!cached) {
  if(g_vao != 0) {
   GLCore::bindVertexArray(g_vao);
  }
  // Position (always present).
  GLCore::enableVertexAttribArray(0);
  GLCore::vertexAttribPointer(0, 3, kFloat, 0, stride, bufOffset(baseOffset + kOffPos));
  // UV.
  if(hasTexture) {
   GLCore::enableVertexAttribArray(1);
   GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(baseOffset + kOffUV));
  } else {
   GLCore::disableVertexAttribArray(1);
   GLCore::vertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 0.0f);
  }
  // Color (normalized ubyte).
  if(hasColor) {
   GLCore::enableVertexAttribArray(2);
   GLCore::vertexAttribPointer(2, 4, kUnsignedByte, 1, stride, bufOffset(baseOffset + kOffColor));
  } else {
   GLCore::disableVertexAttribArray(2);
   GLCore::vertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
  }
  // Normal pointer (normalized byte); the disabled-case constant is refreshed below.
  if(hasNormals) {
   GLCore::enableVertexAttribArray(3);
   GLCore::vertexAttribPointer(3, 3, kByte, 1, stride, bufOffset(baseOffset + kOffNormal));
  } else {
   GLCore::disableVertexAttribArray(3);
  }
  // Lightmap coordinate (aLight, loc 4). Only the solid program consumes it.
  if(lightmapInStride) {
   GLCore::enableVertexAttribArray(4);
   GLCore::vertexAttribPointer(4, 2, kFloat, 0, stride, bufOffset(baseOffset + kOffLight));
  } else {
   GLCore::disableVertexAttribArray(4);
   GLCore::vertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
  }
  g_attribCache = AttribCache{buffer, baseOffset, stride, hasTexture, hasColor, hasNormals, lightmapInStride, buffer != 0};
 }
 if(!hasNormals) {
  const float* n = kDefaultNormal;
  if(!g_constNormalSet || std::memcmp(g_constNormal, n, sizeof(float) * 3) != 0) {
   GLCore::vertexAttrib4f(3, n[0], n[1], n[2], 0.0f);
   std::memcpy(g_constNormal, n, sizeof(float) * 3);
   g_constNormalSet = true;
  }
 }
}
void finishAttribs() {}
void invalidateAttribCache() {
 g_attribCache = AttribCache{};
 g_constNormalSet = false;
 g_lastProgram = nullptr;
}
void bindAndUploadUniforms() {
 RenderPass pass;
 pass.modelView = math::g_modelView.top();
 pass.projection = math::g_projection.top();
 pass.fog = g_fog;
 bindAndUploadUniforms(pass);
}
void drawFromBoundBuffer(unsigned buffer,
                         std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals,
                         bool hasLightmap) {
 if(!ensureReady() || vertexCount == 0) {
  return;
 }
 bindAndUploadUniforms();
 if(g_activeProgram == nullptr) {
  return;
 }
 configureAttribs(buffer, byteOffset, stride, hasTexture, hasColor, hasNormals, hasLightmap);
 ::glDrawArrays(static_cast<GLenum>(glMode), 0, static_cast<GLsizei>(vertexCount));
}
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals,
                     bool hasLightmap) {
 if(!ensureReady() || vertexCount == 0) {
  return;
 }
 bindAndUploadUniforms();
 if(g_activeProgram == nullptr) {
  return;
 }
 uploadStreaming(data, vertexCount * static_cast<std::size_t>(stride));
 configureAttribs(g_streamVbo, 0, stride, hasTexture, hasColor, hasNormals, hasLightmap);
 ::glDrawArrays(static_cast<GLenum>(glMode), 0, static_cast<GLsizei>(vertexCount));
}
void blitFullscreen() {
 if(!ensureBlitResources()) {
  return;
 }
 ShaderProgram* savedProgram = g_activeProgram;
 g_blit.bind();
 g_blit.set1i("uTexture", 0);
 g_blit.set1i("colortex0", 0);
 GLCore::bindVertexArray(g_blitVao);
 ::glDrawArrays(static_cast<GLenum>(0x0004), 0, 3);
 if(g_vao != 0) {
  GLCore::bindVertexArray(g_vao);
 } else {
  GLCore::bindVertexArray(0);
 }
 invalidateAttribCache();
 g_activeProgram = savedProgram;
}
void drawFullscreenTriangle() {
  if(!ensureBlitResources()) {
   return;
  }
  ShaderProgram* savedProgram = g_activeProgram;
  int prevVao = 0;
  ::glGetIntegerv(0x85B5, &prevVao);
  GLCore::bindVertexArray(g_blitVao);
  ::glDrawArrays(static_cast<GLenum>(0x0004), 0, 3);
  GLCore::bindVertexArray(static_cast<unsigned>(prevVao));
  g_activeProgram = savedProgram;
  invalidateAttribCache();
}
} // namespace engine_pipeline
} // namespace net::minecraft::client::gl
