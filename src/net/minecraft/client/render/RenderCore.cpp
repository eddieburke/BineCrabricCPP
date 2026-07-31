#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <utility>
#include <vector>
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::client::render {
namespace math = net::minecraft::util::math;
namespace gl = net::minecraft::client::gl;
namespace core {
namespace {
thread_local math::MatrixStack t_defaultModelView;
thread_local math::MatrixStack t_defaultProjection;
thread_local math::MatrixStack* t_modelView = &t_defaultModelView;
thread_local math::MatrixStack* t_projection = &t_defaultProjection;
thread_local std::vector<std::pair<math::MatrixStack*, math::MatrixStack*>> t_bindStack;
WorldLightUniforms g_worldLight{};
FogUniforms g_fog{};
SkyUniforms g_skyUniforms{};
unsigned int g_globalsGeneration = 1;
unsigned int g_globalsPushed = 0;
const float kDefaultNormal[3] = {0.0f, 0.0f, 0.0f};
constexpr unsigned int kArrayBuffer = 0x8892; // GL_ARRAY_BUFFER
constexpr unsigned int kStreamDraw = 0x88E0; // GL_STREAM_DRAW
constexpr unsigned int kFloat = 0x1406; // GL_FLOAT
constexpr unsigned int kUnsignedByte = 0x1401; // GL_UNSIGNED_BYTE
constexpr unsigned int kByte = 0x1400; // GL_BYTE
constexpr std::size_t kOffPos = 0;
constexpr std::size_t kOffUV = 12;
constexpr std::size_t kOffColor = 20;
constexpr std::size_t kOffNormal = 24;
constexpr std::size_t kOffMidBlock = 28;
constexpr std::size_t kOffLight = 32;
constexpr std::size_t kOffEntity = 36;
constexpr std::size_t kOffMidTexCoord = 44;
constexpr std::size_t kOffTangent = 52;
struct AttribCache {
 unsigned buffer = 0;
 std::size_t baseOffset = 0;
 int stride = 0;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 bool legacy = false;
 bool valid = false;
};
AttribCache g_attribCache;
float g_constNormal[3] = {0.0f, 0.0f, 0.0f};
bool g_constNormalSet = false;
// Source of truth for constant vertex colour.
float g_constColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
// Last colour pushed to vaColor — compared against g_constColor to skip redundant uploads.
float g_uploadedConstColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
bool g_constColorUploaded = false;
float g_alphaTestRef = 0.1f;
unsigned int g_whiteTexture = 0;
unsigned int whiteTexture() {
 if(g_whiteTexture == 0) {
  const int previous = std::max(0, getActiveTextureUnit());
  g_whiteTexture = genTexture();
  if(g_whiteTexture == 0) return 0;
  const unsigned char pixel[4] = {255, 255, 255, 255};
  activeTexture(gl::tex::Texture0);
  bindTexture(static_cast<int>(g_whiteTexture));
  // Sized internal format required: unsized GL_RGBA (0x1908) is INVALID_ENUM
  // on forward-compatible core (GLFW_OPENGL_FORWARD_COMPAT).
  ::glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, pixel);
  ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
  ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
  activeTexture(gl::tex::Texture0 + previous);
 }
 return g_whiteTexture;
}
float g_entityColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
ShaderProgram* g_activeProgram = nullptr;
ShaderProgram* g_lastProgram = nullptr;
bool g_drawEnabled = true;
ProgramUniformUploader g_programUniformUploader;
unsigned int g_programUniformGeneration = 1;
unsigned int g_programUniformPushed = 0;
int g_programUniformDiffuseTexture = -1;
math::Matrix4f g_uploadedModelView{};
math::Matrix4f g_uploadedProjection{};
bool g_matricesUploaded = false;
math::Matrix4f g_drawModelView{};
math::Matrix4f g_drawProjection{};
math::Matrix4f g_drawModelViewInverse{};
math::Matrix4f g_drawProjectionInverse{};
float g_drawCameraPosition[3] = {0.0f, 0.0f, 0.0f};
bool g_drawCameraValid = false;
float g_uploadedChunkOffset[3] = {0.0f, 0.0f, 0.0f};
bool g_chunkOffsetUploaded = false;
float g_uploadedEntityColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
FogUniforms g_uploadedFog{};
WorldLightUniforms g_uploadedWorldLight{};
float g_uploadedAlphaTestRef = -1.0f;
int g_uploadedBlendFunc[4] = {-1, -1, -1, -1};
bool g_passUniformsUploaded = false;
float g_pendingChunkOffset[3] = {0.0f, 0.0f, 0.0f};
bool g_pendingTerrainDraw = false;
int g_entityId = 0;
int g_blockEntityId = 0;
int g_renderedItemId = 0;
RenderStage g_renderStage = RenderStage::None;
int g_textureFilteringMode = 0;
int g_uploadedEntityId = -1;
int g_uploadedBlockEntityId = -1;
int g_uploadedRenderedItemId = -1;
int g_uploadedRenderStage = -1;
int g_uploadedTextureFilteringMode = -1;
int g_uploadedFogShape = -1;
unsigned int g_vao = 0;
unsigned int g_streamVbo = 0;
bool g_triedFullscreen = false;
unsigned int g_fullscreenVao = 0;
unsigned int g_fullscreenVbo = 0;
const void* bufOffset(std::size_t o) {
 return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(o));
}
void ensureVao() {
 if(g_vao == 0 && gl::GLCore::vaoSupported) {
  gl::GLCore::genVertexArrays(1, &g_vao);
 }
 if(g_streamVbo == 0 && gl::GLCore::genBuffers != nullptr) {
  gl::GLCore::genBuffers(1, &g_streamVbo);
 }
}
void uploadStreaming(const void* data, std::size_t bytes) {
 gl::GLCore::bindBuffer(kArrayBuffer, g_streamVbo);
 gl::GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(bytes), data, kStreamDraw);
}
bool g_fullscreenReady = false;
bool ensureFullscreenResources() {
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::shaderSupported || !gl::GLCore::vaoSupported) {
  return false;
 }
 if(!g_triedFullscreen) {
  g_triedFullscreen = true;
  gl::GLCore::genVertexArrays(1, &g_fullscreenVao);
  gl::GLCore::genBuffers(1, &g_fullscreenVbo);
  const float tri[] = {-1.0f, -1.0f, 0.0f, 0.0f, 3.0f, -1.0f, 2.0f, 0.0f, -1.0f, 3.0f, 0.0f, 2.0f};
  gl::GLCore::bindBuffer(kArrayBuffer, g_fullscreenVbo);
  gl::GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(sizeof(tri)), tri, 0x88E4);
  if(g_fullscreenVao != 0) {
   gl::GLCore::bindVertexArray(g_fullscreenVao);
   const int stride = 4 * static_cast<int>(sizeof(float));
   gl::GLCore::enableVertexAttribArray(0);
   gl::GLCore::vertexAttribPointer(0, 2, kFloat, 0, stride, bufOffset(0));
   gl::GLCore::enableVertexAttribArray(1);
   gl::GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(2 * sizeof(float)));
   gl::GLCore::bindVertexArray(0);
   gl::GLCore::bindBuffer(kArrayBuffer, 0);
   g_fullscreenReady = true;
  }
 }
 return g_fullscreenReady;
}
void drawFullscreenTriangle() {
 gl::GLCore::bindVertexArray(g_fullscreenVao);
 ::glDrawArrays(static_cast<GLenum>(0x0004), 0, 3);
 gl::GLCore::bindVertexArray(0);
 invalidateAttribCache();
}
} // namespace
void bindDiffuse(int texture) {
 activeTexture(gl::tex::Texture0);
 bindTexture(texture);
}
void bindWhiteDiffuse() {
 const unsigned int white = whiteTexture();
 if(white != 0) {
  bindDiffuse(static_cast<int>(white));
 }
}
void bindMatrixStacks(math::MatrixStack* modelView, math::MatrixStack* projection) {
 t_bindStack.emplace_back(t_modelView, t_projection);
 if(modelView != nullptr) {
  t_modelView = modelView;
 }
 if(projection != nullptr) {
  t_projection = projection;
 }
}
void unbindMatrixStacks() {
 if(t_bindStack.empty()) {
  t_modelView = &t_defaultModelView;
  t_projection = &t_defaultProjection;
  return;
 }
 t_modelView = t_bindStack.back().first;
 t_projection = t_bindStack.back().second;
 t_bindStack.pop_back();
}
math::MatrixStack& modelViewStack() {
 return *t_modelView;
}
math::MatrixStack& projectionStack() {
 return *t_projection;
}
const math::Matrix4f& currentModelView() {
 return t_modelView->top();
}
const math::Matrix4f& currentProjection() {
 return t_projection->top();
}
bool ensureReady() {
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::shaderSupported || !gl::GLCore::vaoSupported) {
  return false;
 }
 ensureVao();
 return g_vao != 0;
}
ShaderProgram* program() {
 return g_activeProgram;
}
void setDrawCameraState(const float* modelView,
                       const float* projection,
                       const float* modelViewInverse,
                       const float* projectionInverse,
                       const float* cameraPosition) {
 if(modelView == nullptr || projection == nullptr || modelViewInverse == nullptr ||
    projectionInverse == nullptr || cameraPosition == nullptr) {
  clearDrawCameraState();
  return;
 }
 std::memcpy(g_drawModelView.m, modelView, sizeof(float) * 16);
 std::memcpy(g_drawProjection.m, projection, sizeof(float) * 16);
 std::memcpy(g_drawModelViewInverse.m, modelViewInverse, sizeof(float) * 16);
 std::memcpy(g_drawProjectionInverse.m, projectionInverse, sizeof(float) * 16);
 g_drawCameraPosition[0] = cameraPosition[0];
 g_drawCameraPosition[1] = cameraPosition[1];
 g_drawCameraPosition[2] = cameraPosition[2];
 g_drawCameraValid = true;
 g_matricesUploaded = false;
}
void setDrawCameraStateFromCamera(const FrameRenderCamera& camera, float farPlane) {
 float modelView[16]{};
 float modelViewInverse[16]{};
 float projection[16]{};
 buildCameraModelView(modelView, camera);
 buildCameraModelViewInverse(modelViewInverse, camera);
 buildCameraProjection(projection, camera, farPlane);
 math::Matrix4f projectionInverseMatrix;
 projectionInverseMatrix.set(projection);
 projectionInverseMatrix.invert();
 const float cameraPosition[3] = {static_cast<float>(camera.eyeX), static_cast<float>(camera.eyeY),
                                  static_cast<float>(camera.eyeZ)};
 setDrawCameraState(modelView, projection, modelViewInverse, projectionInverseMatrix.m, cameraPosition);
}
void clearDrawCameraState() {
 g_drawCameraValid = false;
 g_matricesUploaded = false;
}
bool drawCameraStateValid() noexcept {
 return g_drawCameraValid;
}
const float* drawCameraPosition() noexcept {
 return g_drawCameraPosition;
}
ScopedDrawCameraState::ScopedDrawCameraState()
    : modelView_(g_drawModelView),
      projection_(g_drawProjection),
      modelViewInverse_(g_drawModelViewInverse),
      projectionInverse_(g_drawProjectionInverse),
      valid_(g_drawCameraValid) {
 cameraPosition_[0] = g_drawCameraPosition[0];
 cameraPosition_[1] = g_drawCameraPosition[1];
 cameraPosition_[2] = g_drawCameraPosition[2];
}
ScopedDrawCameraState::~ScopedDrawCameraState() {
 if(valid_) {
  setDrawCameraState(modelView_.m, projection_.m, modelViewInverse_.m, projectionInverse_.m, cameraPosition_);
 } else {
  clearDrawCameraState();
 }
}
void bindAndUploadUniforms(const RenderPass& pass) {
 ShaderProgram* active = pass.programOverride != nullptr ? pass.programOverride : g_activeProgram;
 if(active == nullptr) {
  return;
 }
 active->bind();
 const bool programChanged = (active != g_lastProgram);
 const PassGlBits glBits = capturePassGlBits();
 if(programChanged && !pass.fullscreen) {
  active->set1i("gtexture", 0);
  active->set1i("texture", 0);
 }
 if(pass.fullscreen) {
  activeTexture(gl::tex::Texture0);
  g_lastProgram = active;
  g_matricesUploaded = false;
  return;
 }
 const FogUniforms& fog = pass.fog.enabled ? pass.fog : g_fog;
 const bool sectionLocal = pass.sectionLocal && g_drawCameraValid;
 const math::Matrix4f& modelView = sectionLocal ? g_drawModelView : pass.modelView;
 const math::Matrix4f& projection = sectionLocal ? g_drawProjection : pass.projection;
 const bool matricesChanged = !g_matricesUploaded || programChanged ||
                              std::memcmp(g_uploadedModelView.m, modelView.m, sizeof(modelView.m)) != 0 ||
                              std::memcmp(g_uploadedProjection.m, projection.m, sizeof(projection.m)) != 0;
 if(matricesChanged) {
  math::Matrix4f modelViewInverse = sectionLocal ? g_drawModelViewInverse : modelView;
  math::Matrix4f projectionInverse = sectionLocal ? g_drawProjectionInverse : projection;
  if(!sectionLocal) {
   modelViewInverse.invert();
   projectionInverse.invert();
  }
  shaderpack::uploadGeometryDrawMatrices(*active, modelView.m, projection.m, modelViewInverse.m, projectionInverse.m);
  g_uploadedModelView = modelView;
  g_uploadedProjection = projection;
  g_matricesUploaded = true;
 }
 const bool fogChanged = !g_passUniformsUploaded || programChanged ||
                         g_uploadedFog.enabled != fog.enabled || g_uploadedFog.mode != fog.mode ||
                         g_uploadedFog.shape != fog.shape ||
                         g_uploadedFog.density != fog.density || g_uploadedFog.start != fog.start ||
                         g_uploadedFog.end != fog.end ||
                         std::memcmp(g_uploadedFog.color, fog.color, sizeof(fog.color)) != 0;
 const bool lightChanged = !g_passUniformsUploaded || programChanged ||
                           g_uploadedWorldLight.enabled != g_worldLight.enabled ||
                           g_uploadedWorldLight.sunIntensity != g_worldLight.sunIntensity ||
                           g_uploadedWorldLight.fillIntensity != g_worldLight.fillIntensity ||
                           std::memcmp(g_uploadedWorldLight.sunDirView, g_worldLight.sunDirView, sizeof(float) * 3) != 0 ||
                           std::memcmp(g_uploadedWorldLight.sunColor, g_worldLight.sunColor, sizeof(float) * 3) != 0 ||
                           std::memcmp(g_uploadedWorldLight.ambient, g_worldLight.ambient, sizeof(float) * 3) != 0 ||
                           std::memcmp(g_uploadedWorldLight.fillDirView, g_worldLight.fillDirView, sizeof(float) * 3) != 0;
 const bool entityColorChanged = !g_passUniformsUploaded || programChanged ||
                                 std::memcmp(g_uploadedEntityColor, g_entityColor, sizeof(g_entityColor)) != 0;
 if(entityColorChanged) {
  active->set4f("entityColor", g_entityColor[0], g_entityColor[1], g_entityColor[2], g_entityColor[3]);
  std::memcpy(g_uploadedEntityColor, g_entityColor, sizeof(g_entityColor));
 }
 if(fogChanged) {
  active->set3f("fogColor", fog.color[0], fog.color[1], fog.color[2]);
  active->set1f("fogDensity", fog.density);
  active->set1f("fogStart", fog.start);
  active->set1f("fogEnd", fog.end);
  active->set1i("fogMode", fog.enabled ? fog.mode : 0);
  active->set1i("fogShape", fog.shape);
  g_uploadedFog = fog;
  g_uploadedFogShape = fog.shape;
 }
 if(lightChanged) {
  active->set3f("sunDirectionView", g_worldLight.sunDirView[0], g_worldLight.sunDirView[1], g_worldLight.sunDirView[2]);
  active->set3f("sunColor", g_worldLight.sunColor[0], g_worldLight.sunColor[1], g_worldLight.sunColor[2]);
  active->set3f("ambientColor", g_worldLight.ambient[0], g_worldLight.ambient[1], g_worldLight.ambient[2]);
  active->set3f("fillDirectionView", g_worldLight.fillDirView[0], g_worldLight.fillDirView[1], g_worldLight.fillDirView[2]);
  active->set1f("sunIntensity", g_worldLight.sunIntensity);
  active->set1f("fillIntensity", g_worldLight.fillIntensity);
  active->set1i("lightingEnabled", g_worldLight.enabled ? 1 : 0);
  g_uploadedWorldLight = g_worldLight;
 }
 if(!g_passUniformsUploaded || programChanged || g_uploadedAlphaTestRef != g_alphaTestRef) {
  active->set1f("alphaTestRef", g_alphaTestRef);
  g_uploadedAlphaTestRef = g_alphaTestRef;
 }
 const bool chunkOffsetChanged = !g_chunkOffsetUploaded || programChanged ||
                                 g_uploadedChunkOffset[0] != pass.chunkOffset[0] || g_uploadedChunkOffset[1] != pass.chunkOffset[1] ||
                                 g_uploadedChunkOffset[2] != pass.chunkOffset[2];
 if(chunkOffsetChanged) {
  active->set3f("chunkOffset", pass.chunkOffset[0], pass.chunkOffset[1], pass.chunkOffset[2]);
  g_uploadedChunkOffset[0] = pass.chunkOffset[0];
  g_uploadedChunkOffset[1] = pass.chunkOffset[1];
  g_uploadedChunkOffset[2] = pass.chunkOffset[2];
  g_chunkOffsetUploaded = true;
 }
 const int blendFunc[4] = {glBits.blendSrc, glBits.blendDst, glBits.blendSrc, glBits.blendDst};
 if(programChanged || g_uploadedTextureFilteringMode != g_textureFilteringMode) {
  active->set1i("textureFilteringMode", g_textureFilteringMode);
  g_uploadedTextureFilteringMode = g_textureFilteringMode;
 }
 if(!g_passUniformsUploaded || programChanged ||
    g_uploadedBlendFunc[0] != blendFunc[0] || g_uploadedBlendFunc[1] != blendFunc[1]) {
  active->set4iAt(active->location("blendFunc"), blendFunc);
  std::memcpy(g_uploadedBlendFunc, blendFunc, sizeof(blendFunc));
 }
 g_passUniformsUploaded = true;
 if(programChanged || g_globalsPushed != g_globalsGeneration) {
  active->set3f(
      "sunDirectionWorld", g_skyUniforms.sunDirection[0], g_skyUniforms.sunDirection[1], g_skyUniforms.sunDirection[2]);
  // Pack sunAngle comes only from FrameUniforms (ShaderDoc). Do not upload vanilla celestial here.
  active->set3f("skyColor", g_skyUniforms.skyColor[0], g_skyUniforms.skyColor[1], g_skyUniforms.skyColor[2]);
  active->set3f(
      "horizonColor", g_skyUniforms.horizonColor[0], g_skyUniforms.horizonColor[1], g_skyUniforms.horizonColor[2]);
  active->set1f("starBrightness", g_skyUniforms.starBrightness);
  active->set1i("renderStars", g_skyUniforms.renderStars ? 1 : 0);
  g_globalsPushed = g_globalsGeneration;
 }
 const bool packUniformsChanged = programChanged || g_programUniformPushed != g_programUniformGeneration;
 activeTexture(gl::tex::Texture0);
 const int diffuseTexture = boundTexture();
 const bool materialUniformsChanged = packUniformsChanged || diffuseTexture != g_programUniformDiffuseTexture;
 if(g_programUniformUploader && materialUniformsChanged) {
  g_programUniformUploader(*active);
 }
 if(packUniformsChanged) {
  g_programUniformPushed = g_programUniformGeneration;
 }
 if(materialUniformsChanged) {
  g_programUniformDiffuseTexture = diffuseTexture;
 }
 if(programChanged || g_uploadedEntityId != g_entityId) {
  active->set1i("entityId", g_entityId);
  g_uploadedEntityId = g_entityId;
 }
 if(programChanged || g_uploadedBlockEntityId != g_blockEntityId) {
  active->set1i("blockEntityId", g_blockEntityId);
  g_uploadedBlockEntityId = g_blockEntityId;
 }
 if(programChanged || g_uploadedRenderedItemId != g_renderedItemId) {
  active->set1i("currentRenderedItemId", g_renderedItemId);
  g_uploadedRenderedItemId = g_renderedItemId;
 }
 if(programChanged || g_uploadedRenderStage != static_cast<int>(g_renderStage)) {
  active->set1i("renderStage", static_cast<int>(g_renderStage));
  g_uploadedRenderStage = static_cast<int>(g_renderStage);
 }
 // Uploader may have switched units for lightmap — diffuse stays on 0.
 activeTexture(gl::tex::Texture0);
 g_lastProgram = active;
}
void submit(const RenderPass& pass) {
 if(!g_drawEnabled || !ensureReady() || pass.vertexCount == 0) {
  return;
 }
 if(!pass.hasTexture) {
  bindWhiteDiffuse();
 }
 bindAndUploadUniforms(pass);
 ShaderProgram* active = pass.programOverride != nullptr ? pass.programOverride : g_activeProgram;
 if(active == nullptr) {
  return;
 }
 if(pass.buffer != 0) {
  configureAttribs(pass.buffer,
                   pass.byteOffset,
                   pass.stride,
                   pass.hasTexture,
                   pass.hasColor,
                   pass.hasNormals);
  const int mode = active != nullptr && active->tessellation() ? 0x000E : pass.glMode;
  if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
  ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(pass.vertexCount));
 } else {
  uploadStreaming(pass.vertexData, pass.vertexCount * static_cast<std::size_t>(pass.stride));
  configureAttribs(
      g_streamVbo, 0, pass.stride, pass.hasTexture, pass.hasColor, pass.hasNormals);
  const int mode = active != nullptr && active->tessellation() ? 0x000E : pass.glMode;
  if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
  ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(pass.vertexCount));
 }
}
void setActiveProgram(ShaderProgram* program) {
 if(g_activeProgram == program) {
  return;
 }
 g_activeProgram = program;
 g_lastProgram = nullptr;
 g_chunkOffsetUploaded = false;
}
void setDrawEnabled(bool enabled) {
 g_drawEnabled = enabled;
}
bool drawEnabled() {
 return g_drawEnabled;
}
void setProgramUniformUploader(ProgramUniformUploader uploader) {
 g_programUniformUploader = std::move(uploader);
 ++g_programUniformGeneration;
}
void advanceProgramUniforms() {
 ++g_programUniformGeneration;
}
void setEntityId(int id) {
 if(g_entityId != id) {
  g_entityId = id;
 }
}
void setBlockEntityId(int id) {
 if(g_blockEntityId != id) {
  g_blockEntityId = id;
 }
}
int blockEntityId() {
 return g_blockEntityId;
}
void setTextureFilteringMode(int mode) {
 g_textureFilteringMode = std::clamp(mode, 0, 2);
}
void setRenderedItemId(int id) {
 if(g_renderedItemId != id) {
  g_renderedItemId = id;
 }
}
RenderStage renderStage() {
 return g_renderStage;
}
void setRenderStage(RenderStage stage) {
 if(g_renderStage != stage) {
  g_renderStage = stage;
 }
}
int entityId() {
 return g_entityId;
}
int renderedItemId() {
 return g_renderedItemId;
}
void setWorldLight(const WorldLightUniforms& light) {
 const bool enabled = g_worldLight.enabled;
 g_worldLight = light;
 g_worldLight.enabled = enabled;
}
const WorldLightUniforms& worldLight() {
 return g_worldLight;
}
void setLightingEnabled(bool enabled) {
 g_worldLight.enabled = enabled;
}
bool lightingEnabled() {
 return g_worldLight.enabled;
}
void setConstColor(float r, float g, float b, float a) {
 g_constColor[0] = r;
 g_constColor[1] = g;
 g_constColor[2] = b;
 g_constColor[3] = a;
}
const float* constColor() {
 return g_constColor;
}
void setAlphaTestRef(float ref) {
 g_alphaTestRef = ref;
}
float alphaTestRef() {
 return g_alphaTestRef;
}
void setFog(const FogUniforms& fog) {
 g_fog = fog;
}
const FogUniforms& fog() {
 return g_fog;
}
void setFogEnabled(bool enabled) {
 g_fog.enabled = enabled;
}
void fogUpdateFromWorld(::net::minecraft::client::Minecraft* client, float tickDelta,
                        const ::net::minecraft::client::option::RenderSettings& frame) {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 namespace option = ::net::minecraft::client::option;
 World& world = *client->world;
 mod::FogSettingsEvent settings{&world, client->camera};
 settings.enabled = g_fog.modEnabled;
 settings.spherical = g_fog.shape == 0;
 settings.exponential = g_fog.modExponential;
 settings.start = g_fog.modStart;
 settings.end = g_fog.modEnd;
 settings.density = g_fog.modDensity;
 net::minecraft::mod::runtime::luaHookFogSettings(settings);
 g_fog.modEnabled = settings.enabled;
 g_fog.shape = settings.spherical ? 0 : 1;
 g_fog.modExponential = settings.exponential;
 g_fog.modStart = settings.start;
 g_fog.modEnd = settings.end;
 g_fog.modDensity = settings.density;
 const Vec3d sky = world.getSkyColor(client->camera, tickDelta);
 const Vec3d fogColor = world.getFogColor(tickDelta);
 float r = static_cast<float>(fogColor.x + (sky.x - fogColor.x) * frame.fogColorBlend);
 float g = static_cast<float>(fogColor.y + (sky.y - fogColor.y) * frame.fogColorBlend);
 float b = static_cast<float>(fogColor.z + (sky.z - fogColor.z) * frame.fogColorBlend);
 const float rain = option::rainGradient(frame, &world, tickDelta);
 if(rain > 0.0f) {
  r *= 1.0f - rain * 0.5f;
  g *= 1.0f - rain * 0.5f;
  b *= 1.0f - rain * 0.4f;
 }
 const float thunder = option::thunderGradient(frame, &world, tickDelta);
 if(thunder > 0.0f) {
  const float dark = 1.0f - thunder * 0.5f;
  r *= dark;
  g *= dark;
  b *= dark;
 }
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  r = frame.clearWater ? 0.05f : 0.02f;
  g = frame.clearWater ? 0.05f : 0.02f;
  b = frame.clearWater ? 0.35f : 0.2f;
 } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
  r = 0.6f;
  g = 0.1f;
  b = 0.0f;
 }
 g_fog.color[0] = r;
 g_fog.color[1] = g;
 g_fog.color[2] = b;
 g_fog.color[3] = 1.0f;
 clearColor(r, g, b, 0.0f);
}
void fogApplyMode(::net::minecraft::client::Minecraft* client, int mode,
                  const ::net::minecraft::client::option::RenderSettings& frame) {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 const bool keepEnabled = g_fog.enabled;
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  const float density = frame.clearWater ? 0.02f : 0.1f;
  g_fog.mode = 2;
  g_fog.density = density;
  if(mode >= 0) {
   g_fog.end = 3.0f / density;
  }
 } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
  g_fog.mode = 2;
  g_fog.density = 2.0f;
  if(mode >= 0) {
   g_fog.end = 1.5f;
  }
 } else if(g_fog.modEnabled && g_fog.modExponential) {
  const float density = g_fog.modDensity / frame.renderScale;
  g_fog.mode = 2;
  g_fog.density = density;
  if(mode >= 0) {
   g_fog.end = density > 0.0f ? 3.0f / density : frame.renderDistanceBlocks;
  }
 } else {
  g_fog.mode = 1;
  const float terrainEdge = static_cast<float>(frame.chunkRadius) * 16.0f - 8.0f;
  const float fogRange = std::min(frame.renderDistanceBlocks, terrainEdge);
  if(mode < 0) {
   g_fog.start = 0.0f;
  } else {
   g_fog.end = fogRange * (g_fog.modEnabled ? g_fog.modEnd : 1.0f);
   g_fog.start = std::min(fogRange * (g_fog.modEnabled ? g_fog.modStart : 0.65f), g_fog.end * 0.9f);
  }
  if(client->world->dimension != nullptr && client->world->dimension->isNether) {
   g_fog.start = 0.0f;
  }
 }
 g_fog.enabled = keepEnabled;
}
void setSkyUniforms(const SkyUniforms& sky) {
 g_skyUniforms = sky;
 ++g_globalsGeneration;
}
const SkyUniforms& skyUniforms() {
 return g_skyUniforms;
}
void configureAttribs(unsigned buffer,
                      std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals) {
 const bool cached = g_attribCache.valid && buffer != 0 && g_attribCache.buffer == buffer &&
                     g_attribCache.baseOffset == baseOffset && g_attribCache.stride == stride &&
                     g_attribCache.hasTexture == hasTexture && g_attribCache.hasColor == hasColor &&
                     g_attribCache.hasNormals == hasNormals &&
                     g_attribCache.legacy == (g_lastProgram != nullptr && g_lastProgram->legacyAttributes());
 if(!cached) {
  const bool legacy = g_lastProgram != nullptr && g_lastProgram->legacyAttributes();
  if(g_vao != 0) {
   gl::GLCore::bindVertexArray(g_vao);
  }
  gl::GLCore::enableVertexAttribArray(0);
  gl::GLCore::vertexAttribPointer(0, 3, kFloat, 0, stride, bufOffset(baseOffset + kOffPos));
  const unsigned int uvIndex = legacy ? 8u : 1u;
  const unsigned int colorIndex = legacy ? 3u : 2u;
  const unsigned int normalIndex = legacy ? 2u : 3u;
  gl::GLCore::disableVertexAttribArray(legacy ? 1u : 8u);
  if(hasTexture) {
   gl::GLCore::enableVertexAttribArray(uvIndex);
   gl::GLCore::vertexAttribPointer(uvIndex, 2, kFloat, 0, stride, bufOffset(baseOffset + kOffUV));
  } else {
   gl::GLCore::disableVertexAttribArray(uvIndex);
   gl::GLCore::vertexAttrib4f(uvIndex, 0.0f, 0.0f, 0.0f, 1.0f);
  }
  if(hasColor) {
   gl::GLCore::enableVertexAttribArray(colorIndex);
   gl::GLCore::vertexAttribPointer(colorIndex, 4, kUnsignedByte, 1, stride, bufOffset(baseOffset + kOffColor));
  } else {
   // The constant colour is supplied below, outside the cache, because it
   // changes far more often than the vertex layout does.
   gl::GLCore::disableVertexAttribArray(colorIndex);
  }
  if(hasNormals) {
   gl::GLCore::enableVertexAttribArray(normalIndex);
   gl::GLCore::vertexAttribPointer(normalIndex, 3, kByte, 1, stride, bufOffset(baseOffset + kOffNormal));
  } else {
   gl::GLCore::disableVertexAttribArray(normalIndex);
  }
  gl::GLCore::enableVertexAttribArray(4);
  gl::GLCore::vertexAttribPointer(4, 4, kByte, 0, stride, bufOffset(baseOffset + kOffMidBlock));
  const unsigned int lightIndex = legacy ? 10u : 5u;
  gl::GLCore::disableVertexAttribArray(legacy ? 5u : 10u);
  gl::GLCore::enableVertexAttribArray(lightIndex);
  gl::GLCore::vertexAttribPointer(lightIndex, 2, 0x1403, 0, stride, bufOffset(baseOffset + kOffLight));
  if(legacy) {
   gl::GLCore::enableVertexAttribArray(9);
   gl::GLCore::vertexAttribPointer(9, 2, 0x1403, 0, stride, bufOffset(baseOffset + kOffLight));
  } else {
   gl::GLCore::disableVertexAttribArray(9);
  }
  gl::GLCore::enableVertexAttribArray(6);
  gl::GLCore::vertexAttribPointer(6, 4, 0x1402, 0, stride, bufOffset(baseOffset + kOffEntity));
  gl::GLCore::enableVertexAttribArray(7);
  gl::GLCore::vertexAttribPointer(7, 2, kFloat, 0, stride, bufOffset(baseOffset + kOffMidTexCoord));
  gl::GLCore::enableVertexAttribArray(11);
  gl::GLCore::vertexAttribPointer(11, 4, 0x1402, 1, stride, bufOffset(baseOffset + kOffTangent));
  gl::GLCore::disableVertexAttribArray(12);
  if(gl::GLCore::vertexAttrib4f != nullptr) gl::GLCore::vertexAttrib4f(12, 1.0f, 0.0f, 0.0f, 0.0f);
  g_attribCache = AttribCache{buffer, baseOffset, stride, hasTexture, hasColor, hasNormals, legacy, buffer != 0};
 }
 if(!hasNormals) {
  const float* n = kDefaultNormal;
  if(!g_constNormalSet || std::memcmp(g_constNormal, n, sizeof(float) * 3) != 0) {
   const unsigned int normalIndex = g_lastProgram != nullptr && g_lastProgram->legacyAttributes() ? 2u : 3u;
   gl::GLCore::vertexAttrib4f(normalIndex, n[0], n[1], n[2], 0.0f);
   std::memcpy(g_constNormal, n, sizeof(float) * 3);
   g_constNormalSet = true;
  }
 }
 if(!hasColor) {
  if(!g_constColorUploaded || std::memcmp(g_uploadedConstColor, g_constColor, sizeof(float) * 4) != 0) {
   const unsigned int colorIndex = g_lastProgram != nullptr && g_lastProgram->legacyAttributes() ? 3u : 2u;
   gl::GLCore::vertexAttrib4f(colorIndex, g_constColor[0], g_constColor[1], g_constColor[2], g_constColor[3]);
   std::memcpy(g_uploadedConstColor, g_constColor, sizeof(float) * 4);
   g_constColorUploaded = true;
  }
 }
}
void invalidateAttribCache() {
 g_attribCache = AttribCache{};
 g_constNormalSet = false;
 g_constColorUploaded = false;
 g_lastProgram = nullptr;
 g_matricesUploaded = false;
 g_passUniformsUploaded = false;
}
void setEntityColor(float red, float green, float blue, float alpha) {
 g_entityColor[0] = red;
 g_entityColor[1] = green;
 g_entityColor[2] = blue;
 g_entityColor[3] = alpha;
 ++g_globalsGeneration;
}
void setPendingTerrainDraw(float chunkOffsetX, float chunkOffsetY, float chunkOffsetZ) {
 g_pendingChunkOffset[0] = chunkOffsetX;
 g_pendingChunkOffset[1] = chunkOffsetY;
 g_pendingChunkOffset[2] = chunkOffsetZ;
 g_pendingTerrainDraw = true;
}
void clearPendingTerrainDraw() {
 g_pendingTerrainDraw = false;
 g_pendingChunkOffset[0] = g_pendingChunkOffset[1] = g_pendingChunkOffset[2] = 0.0f;
}
void applyPendingTerrain(RenderPass& pass) {
 if(!g_pendingTerrainDraw) {
  return;
 }
 pass.chunkOffset[0] = g_pendingChunkOffset[0];
 pass.chunkOffset[1] = g_pendingChunkOffset[1];
 pass.chunkOffset[2] = g_pendingChunkOffset[2];
 pass.sectionLocal = true;
}
void bindAndUploadUniforms() {
 RenderPass pass;
 pass.modelView = currentModelView();
 pass.projection = currentProjection();
 pass.fog = g_fog;
 applyPendingTerrain(pass);
 bindAndUploadUniforms(pass);
}
void drawFromBoundBuffer(unsigned buffer,
                         std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals) {
 if(!ensureReady() || vertexCount == 0) {
  return;
 }
 RenderPass pass;
 pass.modelView = currentModelView();
 pass.projection = currentProjection();
 pass.fog = g_fog;
 pass.hasTexture = hasTexture;
 pass.hasColor = hasColor;
 pass.hasNormals = hasNormals;
 applyPendingTerrain(pass);
 if(!hasTexture) {
  bindWhiteDiffuse();
 }
 bindAndUploadUniforms(pass);
 if(g_activeProgram == nullptr) {
  return;
 }
 configureAttribs(buffer, byteOffset, stride, hasTexture, hasColor, hasNormals);
 const int mode = g_activeProgram->tessellation() ? 0x000E : glMode;
 if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
 ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(vertexCount));
}
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals) {
 if(!ensureReady() || vertexCount == 0) {
  return;
 }
 RenderPass pass;
 pass.modelView = currentModelView();
 pass.projection = currentProjection();
 pass.fog = g_fog;
 pass.hasTexture = hasTexture;
 pass.hasColor = hasColor;
 pass.hasNormals = hasNormals;
 applyPendingTerrain(pass);
 if(!hasTexture) {
  bindWhiteDiffuse();
 }
 bindAndUploadUniforms(pass);
 if(g_activeProgram == nullptr) {
  return;
 }
 uploadStreaming(data, vertexCount * static_cast<std::size_t>(stride));
 configureAttribs(g_streamVbo, 0, stride, hasTexture, hasColor, hasNormals);
 const int mode = g_activeProgram->tessellation() ? 0x000E : glMode;
 if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
 ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(vertexCount));
}
void drawFullscreen() {
 if(!g_drawEnabled || !ensureFullscreenResources()) {
  return;
 }
 drawFullscreenTriangle();
}
// --- GL state (dirty-cache elision only; no public snapshot restore) ---
namespace {
struct GlCache {
 bool blend = false;
 bool depthTest = false;
 bool cullFace = false;
 bool polygonOffset = false;
 bool depthWrite = true;
 bool colorMaskR = true;
 bool colorMaskG = true;
 bool colorMaskB = true;
 bool colorMaskA = true;
 int blendSrc = 0x0302; // GL_SRC_ALPHA
 int blendDst = 0x0303; // GL_ONE_MINUS_SRC_ALPHA
 int depthFunc = 0x0201;
 int cullFaceMode = 0x0405;
 float polygonFactor = 0.0f;
 float polygonUnits = 0.0f;
 float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
 int viewport[4] = {0, 0, 0, 0};
 bool viewportValid = false;
 int guiScaleFactor = 1;
 int activeTexture = 0;
 unsigned boundTextures[32] = {0};
};
GlCache g_gl;
std::mutex g_textureMutex;
std::vector<unsigned int> g_allocatedTextures;
std::unordered_map<unsigned, int> g_textureUnitOf;
} // namespace
PassGlBits capturePassGlBits() {
 PassGlBits bits;
 bits.blend = g_gl.blend;
 bits.depthTest = g_gl.depthTest;
 bits.depthWrite = g_gl.depthWrite;
 bits.cull = g_gl.cullFace;
 bits.blendSrc = g_gl.blendSrc;
 bits.blendDst = g_gl.blendDst;
 bits.cullMode = g_gl.cullFaceMode;
 bits.colorMaskR = g_gl.colorMaskR;
 bits.colorMaskG = g_gl.colorMaskG;
 bits.colorMaskB = g_gl.colorMaskB;
 bits.colorMaskA = g_gl.colorMaskA;
 return bits;
}
void restorePassGlBits(const PassGlBits& bits) {
 if(bits.blend)
  enableBlend();
 else
  disableBlend();
 blendFunc(bits.blendSrc, bits.blendDst);
 if(bits.depthTest)
  enableDepthTest();
 else
  disableDepthTest();
 depthMask(bits.depthWrite);
 if(bits.cull)
  enableCull();
 else
  disableCull();
 cullFace(bits.cullMode);
 colorMask(bits.colorMaskR, bits.colorMaskG, bits.colorMaskB, bits.colorMaskA);
}
bool blendEnabled() {
 return g_gl.blend;
}
int blendSrcFactor() {
 return g_gl.blendSrc;
}
int blendDstFactor() {
 return g_gl.blendDst;
}
bool depthTestEnabled() {
 return g_gl.depthTest;
}
bool depthWriteEnabled() {
 return g_gl.depthWrite;
}
bool cullEnabled() {
 return g_gl.cullFace;
}
int boundTexture() {
 if(g_gl.activeTexture < 0 || g_gl.activeTexture >= 32) {
  return 0;
 }
 return static_cast<int>(g_gl.boundTextures[g_gl.activeTexture]);
}
void enableBlend() {
 if(!g_gl.blend) {
  g_gl.blend = true;
  ::glEnable(0x0BE2);
 }
}
void disableBlend() {
 if(g_gl.blend) {
  g_gl.blend = false;
  ::glDisable(0x0BE2);
 }
}
void blendFunc(int src, int dst) {
 if(g_gl.blendSrc != src || g_gl.blendDst != dst) {
  g_gl.blendSrc = src;
  g_gl.blendDst = dst;
  ::glBlendFunc(static_cast<unsigned>(src), static_cast<unsigned>(dst));
 }
}
void lockBlend(const BlendMode* mode) {
 if(mode == nullptr) {
  disableBlend();
  return;
 }
 enableBlend();
 if(gl::GLCore::blendFuncSeparate != nullptr) {
  gl::GLCore::blendFuncSeparate(static_cast<unsigned>(mode->srcRgb), static_cast<unsigned>(mode->dstRgb),
                                static_cast<unsigned>(mode->srcAlpha), static_cast<unsigned>(mode->dstAlpha));
  g_gl.blendSrc = mode->srcRgb;
  g_gl.blendDst = mode->dstRgb;
 } else {
  blendFunc(mode->srcRgb, mode->dstRgb);
 }
}
void lockBufferBlend(int drawBufferIndex, const BlendMode* mode) {
 if(drawBufferIndex < 0) {
  return;
 }
 if(mode == nullptr) {
  if(gl::GLCore::blendFunci != nullptr) {
   // GL_FUNC_ADD with ZERO/ZERO effectively disables contribution when supported via separatei.
   if(gl::GLCore::blendFuncSeparatei != nullptr) {
    gl::GLCore::blendFuncSeparatei(static_cast<unsigned>(drawBufferIndex), 0, 1, 0, 1);
   }
  }
  return;
 }
 if(gl::GLCore::blendFuncSeparatei != nullptr) {
  gl::GLCore::blendFuncSeparatei(static_cast<unsigned>(drawBufferIndex),
                                 static_cast<unsigned>(mode->srcRgb), static_cast<unsigned>(mode->dstRgb),
                                 static_cast<unsigned>(mode->srcAlpha), static_cast<unsigned>(mode->dstAlpha));
 } else if(gl::GLCore::blendFunci != nullptr) {
  gl::GLCore::blendFunci(static_cast<unsigned>(drawBufferIndex), static_cast<unsigned>(mode->srcRgb),
                         static_cast<unsigned>(mode->dstRgb));
 }
}
void unlockBlend() {
 blendAlpha();
}
void blendAlpha() {
 enableBlend();
 blendFunc(0x0302, 0x0303);
}
void blendAdditive() {
 enableBlend();
 blendFunc(0x0001, 0x0001);
}
void blendCustom(int src, int dst) {
 enableBlend();
 blendFunc(src, dst);
}
void blendDstAlpha() {
 enableBlend();
 blendFunc(0x0302, 0x0304);
}
void blendInverseColor() {
 enableBlend();
 blendFunc(0x0000, 0x0301);
}
void blendMultiply() {
 enableBlend();
 blendFunc(0x0307, 0x0301);
}
void enableDepthTest() {
 if(!g_gl.depthTest) {
  g_gl.depthTest = true;
  ::glEnable(0x0B71);
 }
}
void depthTest() {
 enableDepthTest();
 depthFunc(0x0203);
}
void depthTestWrite(bool write) {
 enableDepthTest();
 depthFunc(0x0203);
 depthMask(write);
}
void disableDepthTest() {
 if(g_gl.depthTest) {
  g_gl.depthTest = false;
  ::glDisable(0x0B71);
 }
}
void depthFunc(int func) {
 if(g_gl.depthFunc != func) {
  g_gl.depthFunc = func;
  ::glDepthFunc(static_cast<unsigned>(func));
 }
}
void depthMask(bool enabled) {
 if(g_gl.depthWrite != enabled) {
  g_gl.depthWrite = enabled;
  ::glDepthMask(enabled ? 1 : 0);
 }
}
void enableCull() {
 if(!g_gl.cullFace) {
  g_gl.cullFace = true;
  ::glEnable(0x0B44);
 }
}
void disableCull() {
 if(g_gl.cullFace) {
  g_gl.cullFace = false;
  ::glDisable(0x0B44);
 }
}
void cullFace(int mode) {
 if(g_gl.cullFaceMode != mode) {
  g_gl.cullFaceMode = mode;
  ::glCullFace(static_cast<unsigned>(mode));
 }
}
void cullBackFaces() {
 enableCull();
 cullFace(0x0405);
}
void enablePolygonOffset() {
 if(!g_gl.polygonOffset) {
  g_gl.polygonOffset = true;
  ::glEnable(0x8037);
 }
}
void disablePolygonOffset() {
 if(g_gl.polygonOffset) {
  g_gl.polygonOffset = false;
  ::glDisable(0x8037);
 }
}
void polygonOffset(float factor, float units) {
 if(g_gl.polygonFactor != factor || g_gl.polygonUnits != units) {
  g_gl.polygonFactor = factor;
  g_gl.polygonUnits = units;
  ::glPolygonOffset(factor, units);
 }
}
void clearColor(float r, float g, float b, float a) {
 if(g_gl.clearColor[0] != r || g_gl.clearColor[1] != g || g_gl.clearColor[2] != b || g_gl.clearColor[3] != a) {
  g_gl.clearColor[0] = r;
  g_gl.clearColor[1] = g;
  g_gl.clearColor[2] = b;
  g_gl.clearColor[3] = a;
  ::glClearColor(r, g, b, a);
 }
}
void clear(int mask) {
 ::glClear(static_cast<unsigned>(mask));
}
void clearDepth(double depth) {
 ::glClearDepth(depth);
}
int getActiveTextureUnit() {
 return g_gl.activeTexture;
}
void activeTexture(int texture) {
 const int unit = (texture >= 0x84C0) ? (texture - 0x84C0) : texture;
 const unsigned int glEnum = static_cast<unsigned int>((texture >= 0x84C0) ? texture : (0x84C0 + texture));
 if(unit < 0 || unit >= 32) {
  return;
 }
 if(g_gl.activeTexture != unit) {
  g_gl.activeTexture = unit;
  if(gl::GLCore::activeTexture != nullptr) {
   reinterpret_cast<void(APIENTRY*)(unsigned)>(gl::GLCore::activeTexture)(glEnum);
  }
 }
}
void bindTexture(int texture) {
 bindTexture(0x0DE1, texture);
}
void bindTexture(unsigned int texture) {
 bindTexture(0x0DE1, static_cast<int>(texture));
}
void bindTexture(int target, int texture) {
 if(target <= 0) {
  target = 0x0DE1;
 }
 if(texture < 0) {
  texture = 0;
 }
 const unsigned int uTex = static_cast<unsigned int>(texture);
 if(target == 0x0DE1 && g_gl.activeTexture >= 0 && g_gl.activeTexture < 32) {
  if(g_gl.boundTextures[g_gl.activeTexture] == uTex) {
   return;
  }
  g_gl.boundTextures[g_gl.activeTexture] = uTex;
  if(uTex != 0) {
   g_textureUnitOf[uTex] = g_gl.activeTexture;
  }
 }
 ::glBindTexture(static_cast<unsigned>(target), uTex);
}
void invalidateTextureBindCache() {
 for(unsigned int& bound : g_gl.boundTextures) {
  bound = 0xFFFFFFFFu;
 }
 g_textureUnitOf.clear();
}
void unbindTexture(int texture) {
 if(texture <= 0)
  return;
 const unsigned uTex = static_cast<unsigned>(texture);
 const auto it = g_textureUnitOf.find(uTex);
 if(it != g_textureUnitOf.end()) {
  const int unit = it->second;
  g_textureUnitOf.erase(it);
  if(unit >= 0 && unit < 32 && g_gl.boundTextures[unit] == uTex) {
   activeTexture(0x84C0 + unit);
   ::glBindTexture(0x0DE1, 0);
   g_gl.boundTextures[unit] = 0;
  }
  return;
 }
 for(int i = 0; i < 32; ++i) {
  if(g_gl.boundTextures[i] == uTex) {
   activeTexture(0x84C0 + i);
   ::glBindTexture(0x0DE1, 0);
   g_gl.boundTextures[i] = 0;
  }
 }
}
unsigned int genTexture() {
 std::lock_guard lock(g_textureMutex);
 unsigned int tex = 0;
 ::glGenTextures(1, &tex);
 g_allocatedTextures.push_back(tex);
 return tex;
}
void deleteTexture(unsigned int texture) {
 if(texture == 0)
  return;
 unbindTexture(static_cast<int>(texture));
 {
  std::lock_guard lock(g_textureMutex);
  std::erase(g_allocatedTextures, texture);
 }
 ::glDeleteTextures(1, &texture);
}
void clearAllocatedTextures() {
 std::lock_guard lock(g_textureMutex);
 if(!g_allocatedTextures.empty()) {
  ::glDeleteTextures(static_cast<int>(g_allocatedTextures.size()), g_allocatedTextures.data());
  g_allocatedTextures.clear();
 }
}
void colorMask(bool r, bool g, bool b, bool a) {
 if(g_gl.colorMaskR != r || g_gl.colorMaskG != g || g_gl.colorMaskB != b || g_gl.colorMaskA != a) {
  g_gl.colorMaskR = r;
  g_gl.colorMaskG = g;
  g_gl.colorMaskB = b;
  g_gl.colorMaskA = a;
  ::glColorMask(r ? 1 : 0, g ? 1 : 0, b ? 1 : 0, a ? 1 : 0);
 }
}
void hintFogEnabled(bool enabled) {
 setFogEnabled(enabled);
}
void viewport(int x, int y, int width, int height) {
 if(!g_gl.viewportValid || g_gl.viewport[0] != x || g_gl.viewport[1] != y || g_gl.viewport[2] != width ||
    g_gl.viewport[3] != height) {
  g_gl.viewport[0] = x;
  g_gl.viewport[1] = y;
  g_gl.viewport[2] = width;
  g_gl.viewport[3] = height;
  g_gl.viewportValid = true;
  ::glViewport(x, y, width, height);
 }
}
void setGuiScaleFactor(int factor) {
 g_gl.guiScaleFactor = factor > 0 ? factor : 1;
}
int getGuiScaleFactor() {
 return g_gl.guiScaleFactor;
}
bool getCachedViewport(int outViewport[4]) {
 if(!g_gl.viewportValid) {
  return false;
 }
 outViewport[0] = g_gl.viewport[0];
 outViewport[1] = g_gl.viewport[1];
 outViewport[2] = g_gl.viewport[2];
 outViewport[3] = g_gl.viewport[3];
 return true;
}
void getIntegerv(int pname, int* params) {
 switch(pname) {
 case 0x0BA2:
  if(g_gl.viewportValid) {
   std::memcpy(params, g_gl.viewport, sizeof(int) * 4);
  } else {
   ::glGetIntegerv(static_cast<unsigned>(pname), params);
  }
  return;
 default:
  ::glGetIntegerv(static_cast<unsigned>(pname), params);
 }
}
BlendScope::BlendScope(bool enable, int src, int dst)
    : savedEnable_(g_gl.blend), savedSrc_(g_gl.blendSrc), savedDst_(g_gl.blendDst) {
 if(enable) {
  enableBlend();
  blendFunc(src, dst);
 } else {
  disableBlend();
 }
}
BlendScope::~BlendScope() {
 if(savedEnable_) {
  enableBlend();
  blendFunc(savedSrc_, savedDst_);
 } else {
  disableBlend();
 }
}
DepthScope::DepthScope(bool test, bool write) : savedTest_(g_gl.depthTest), savedWrite_(g_gl.depthWrite) {
 if(test) {
  enableDepthTest();
 } else {
  disableDepthTest();
 }
 depthMask(write);
}
DepthScope::~DepthScope() {
 if(savedTest_) {
  enableDepthTest();
 } else {
  disableDepthTest();
 }
 depthMask(savedWrite_);
}
CullScope::CullScope(bool enable, int mode) : savedEnable_(g_gl.cullFace), savedMode_(g_gl.cullFaceMode) {
 if(enable) {
  enableCull();
  cullFace(mode);
 } else {
  disableCull();
 }
}
CullScope::~CullScope() {
 if(savedEnable_) {
  enableCull();
  cullFace(savedMode_);
 } else {
  disableCull();
 }
}
ColorMaskScope::ColorMaskScope(bool r, bool g, bool b, bool a)
    : savedR_(g_gl.colorMaskR), savedG_(g_gl.colorMaskG), savedB_(g_gl.colorMaskB), savedA_(g_gl.colorMaskA) {
 colorMask(r, g, b, a);
}
ColorMaskScope::~ColorMaskScope() {
 colorMask(savedR_, savedG_, savedB_, savedA_);
}
TextureBindScope::TextureBindScope() : savedUnit_(g_gl.activeTexture), savedTex_(0) {
 if(savedUnit_ >= 0 && savedUnit_ < 32) {
  savedTex_ = g_gl.boundTextures[savedUnit_];
 }
}
TextureBindScope::TextureBindScope(int texture) : TextureBindScope() {
 bindTexture(texture);
}
TextureBindScope::~TextureBindScope() {
 activeTexture(0x84C0 + savedUnit_);
 bindTexture(static_cast<int>(savedTex_));
}
} // namespace core
} // namespace net::minecraft::client::render
