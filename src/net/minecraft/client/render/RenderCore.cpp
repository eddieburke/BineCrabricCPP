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
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::client::render {
namespace math = net::minecraft::util::math;
namespace gl = net::minecraft::client::gl;
namespace abi = vertex_abi;
namespace core {
namespace {
WorldLightUniforms g_worldLight{};
FogUniforms g_fog{};
// Lua fog_settings inputs. They are what the hook last answered, not uniform values,
// so they stay out of FogUniforms — setFog() must not be able to clobber them.
bool g_modFogEnabled = false;
bool g_modFogExponential = false;
float g_modFogStart = 0.2f;
float g_modFogEnd = 0.8f;
float g_modFogDensity = 0.1f;
SkyUniforms g_skyUniforms{};
unsigned int g_globalsGeneration = 1;
unsigned int g_globalsPushed = 0;
const float kDefaultNormal[3] = {0.0f, 1.0f, 0.0f};
constexpr unsigned int kArrayBuffer = 0x8892; // GL_ARRAY_BUFFER
constexpr unsigned int kStreamDraw = 0x88E0; // GL_STREAM_DRAW
constexpr unsigned int kFloat = 0x1406; // GL_FLOAT
struct AttribCache {
 unsigned buffer = 0;
 std::size_t baseOffset = 0;
 int stride = 0;
 bool hasTexture = false;
 bool hasNormals = false;
 bool overrideColor = false;
 std::uint32_t colorOverride = 0xFFFFFFFFU;
 bool valid = false;
};
AttribCache g_attribCache;
float g_constNormal[3] = {0.0f, 0.0f, 0.0f};
bool g_constNormalSet = false;
// Source of truth for the colour a vertex gets when its producer set none.
// The Tessellator reads the packed form per vertex, so it never needs uploading.
float g_constColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
std::uint32_t g_constColorPacked = 0xFFFFFFFFU;
float g_alphaTestRef = 0.1f;
gl::GlTexture g_whiteTexture;
unsigned int whiteTexture() {
 if(!g_whiteTexture) {
  const int previous = std::max(0, getActiveTextureUnit());
  g_whiteTexture = gl::GlTexture(genTexture());
  if(!g_whiteTexture) return 0;
  const unsigned char pixel[4] = {255, 255, 255, 255};
  activeTexture(gl::tex::Texture0);
  bindTexture(static_cast<int>(g_whiteTexture.handle()));
  // Sized internal format required: unsized GL_RGBA (0x1908) is INVALID_ENUM
  // on forward-compatible core (GLFW_OPENGL_FORWARD_COMPAT).
  ::glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, pixel);
  ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
  ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
  activeTexture(gl::tex::Texture0 + previous);
 }
 return g_whiteTexture.handle();
}
gl::GlTexture g_entityOverlayTexture;
bool g_entityOverlayDirty = true;
float g_entityColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
CelestialState g_celestialState;
FrameRenderCamera g_cameraFrame;
ShaderProgram* g_activeProgram = nullptr;
std::optional<WorldProgramId> g_activeWorldProgram;
ShaderProgram* g_lastProgram = nullptr;
bool g_drawEnabled = true;
unsigned int g_programUniformGeneration = 1;
ShaderProgram* g_programUniformMaterialProgram = nullptr;
std::optional<WorldProgramId> g_programUniformMaterialWorldProgram;
int g_programUniformDiffuseTexture = -1;
math::Matrix4f g_uploadedModelView{};
math::Matrix4f g_uploadedProjection{};
bool g_matricesUploaded = false;
math::Matrix4f g_drawModelView{};
math::Matrix4f g_drawProjection{};
math::Matrix4f g_drawModelViewInverse{};
math::Matrix4f g_drawProjectionInverse{};
math::Matrix4f g_drawPose{};
float g_drawCameraPosition[3] = {0.0f, 0.0f, 0.0f};
bool g_drawCameraValid = false;
float g_uploadedChunkOffset[3] = {0.0f, 0.0f, 0.0f};
bool g_chunkOffsetUploaded = false;
float g_uploadedEntityColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
WorldLightUniforms g_uploadedWorldLight{};
float g_uploadedAlphaTestRef = -1.0f;
int g_uploadedBlendFunc[4] = {-1, -1, -1, -1};
FogUniforms g_uploadedFog{};
bool g_uploadedFogOn = false;
bool g_fogUploaded = false;
bool g_passUniformsUploaded = false;
float g_pendingChunkOffset[3] = {0.0f, 0.0f, 0.0f};
bool g_pendingTerrainDraw = false;
Pipeline* shaderPipeline() {
 net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 return minecraft != nullptr && minecraft->gameRenderer != nullptr
            ? minecraft->gameRenderer->shaderPipeline()
            : nullptr;
}
int g_entityId = -1;
int g_blockEntityId = -1;
int g_renderedItemId = -1;
RenderStage g_renderStage = RenderStage::None;
int g_textureFilteringMode = 0;
int g_uploadedEntityId = -1;
int g_uploadedBlockEntityId = -1;
int g_uploadedRenderedItemId = -1;
int g_uploadedRenderStage = -1;
int g_uploadedTextureFilteringMode = -1;
gl::GlVao g_vao;
gl::GlBuffer g_streamVbo;
bool g_triedFullscreen = false;
gl::GlVao g_fullscreenVao;
gl::GlBuffer g_fullscreenVbo;
const void* bufOffset(std::size_t o) {
 return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(o));
}
void ensureVao() {
 if(!g_vao && gl::GLCore::vaoSupported) {
  unsigned int h = 0;
  gl::GLCore::genVertexArrays(1, &h);
  g_vao = gl::GlVao(h);
 }
 if(!g_streamVbo && gl::GLCore::genBuffers != nullptr) {
  unsigned int h = 0;
  gl::GLCore::genBuffers(1, &h);
  g_streamVbo = gl::GlBuffer(h);
 }
}
void uploadStreaming(const void* data, std::size_t bytes) {
 gl::GLCore::bindBuffer(kArrayBuffer, g_streamVbo.handle());
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
  unsigned int h = 0;
  gl::GLCore::genVertexArrays(1, &h);
  g_fullscreenVao = gl::GlVao(h);
  unsigned int hb = 0;
  gl::GLCore::genBuffers(1, &hb);
  g_fullscreenVbo = gl::GlBuffer(hb);
  const float tri[] = {-1.0f, -1.0f, 0.0f, 0.0f, 3.0f, -1.0f, 2.0f, 0.0f, -1.0f, 3.0f, 0.0f, 2.0f};
  gl::GLCore::bindBuffer(kArrayBuffer, g_fullscreenVbo.handle());
  gl::GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(sizeof(tri)), tri, 0x88E4);
  if(g_fullscreenVao) {
   gl::GLCore::bindVertexArray(g_fullscreenVao.handle());
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
 gl::GLCore::bindVertexArray(g_fullscreenVao.handle());
 ::glDrawArrays(static_cast<GLenum>(0x0004), 0, 3);
 gl::GLCore::bindVertexArray(0);
 invalidateAttribCache();
}
} // namespace
void bindWhiteDiffuse() {
 const unsigned int white = whiteTexture();
 if(white != 0) {
  activeTexture(gl::tex::Texture0);
  bindTexture(static_cast<int>(white));
 }
}
bool ensureReady() {
 gl::GLCore::ensureLoaded();
 if(!gl::GLCore::shaderSupported || !gl::GLCore::vaoSupported) {
  return false;
 }
 ensureVao();
 return g_vao.handle() != 0;
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
 // A pass boundary. The pose is model->pass-base space, so it means nothing once
 // the base changes.
 g_drawPose.identity();
}
void setDrawCameraStateFromCamera(const FrameRenderCamera& camera) {
 float projection[16]{};
 // The projection is built from the camera's own planes (nearPlane/farPlane set by
 // GameRenderer from the render distance, or carried by a custom shadow camera).
 buildCameraProjection(projection, camera);
 float gbufferModelView[16]{};
 buildCameraModelView(gbufferModelView, camera);
 math::Matrix4f modelView;
 modelView.set(gbufferModelView);
 math::Matrix4f modelViewInverse = modelView;
 modelViewInverse.invert();
 math::Matrix4f projectionInverseMatrix;
 projectionInverseMatrix.set(projection);
 projectionInverseMatrix.invert();
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
 const float cameraPosition[3] = {static_cast<float>(camera.eyeX), static_cast<float>(camera.eyeY),
                                  static_cast<float>(camera.eyeZ)};
 setDrawCameraState(modelView.m, projection, modelViewInverse.m, projectionInverseMatrix.m, cameraPosition);
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
const math::Matrix4f& drawModelView() noexcept {
 return g_drawModelView;
}
const math::Matrix4f& drawProjection() noexcept {
 return g_drawProjection;
}
// The pass base. Only a pass owner calls this — the world camera, the GUI ortho,
// the hand's own space. Producers never do; they compose a pose instead.
void setPassModelView(const math::Matrix4f& modelView) noexcept {
 g_drawModelView = modelView;
 // The inverse is the same fact as the matrix. bindAndUploadUniforms trusts it
 // whenever a draw sits on the pass base, so leaving the old one behind uploads a
 // modelViewMatrixInverse belonging to the previous base — the hand pass carried
 // the identity inverse through the damage tilt and the view bob.
 g_drawModelViewInverse = modelView;
 g_drawModelViewInverse.invert();
 g_matricesUploaded = false;
 g_drawPose.identity();
}
void setDrawPose(const math::Matrix4f& pose) noexcept {
 g_drawPose = pose;
}
const math::Matrix4f& drawPose() noexcept {
 return g_drawPose;
}
ScopedDrawCameraState::ScopedDrawCameraState()
    : modelView_(g_drawModelView),
      projection_(g_drawProjection),
      modelViewInverse_(g_drawModelViewInverse),
      projectionInverse_(g_drawProjectionInverse),
      pose_(g_drawPose),
      valid_(g_drawCameraValid) {
 cameraPosition_[0] = g_drawCameraPosition[0];
 cameraPosition_[1] = g_drawCameraPosition[1];
 cameraPosition_[2] = g_drawCameraPosition[2];
}
ScopedDrawCameraState::~ScopedDrawCameraState() {
 if(valid_) {
  setDrawCameraState(modelView_.m, projection_.m, modelViewInverse_.m, projectionInverse_.m, cameraPosition_);
  // setDrawCameraState resets the pose (it is a pass boundary), so put the saved
  // one back — a nested render must hand its caller the pose it had.
  g_drawPose = pose_;
 } else {
  clearDrawCameraState();
 }
}
void uploadFogUniforms(ShaderProgram& program, const FogUniforms& fog, bool on) {
 using Slot = ShaderProgram::IrisUniformSlot;
 program.set3fAt(program.uniformLocation(Slot::FogColor), fog.color);
 program.set1fAt(program.uniformLocation(Slot::FogDensity), fog.density);
 program.set1fAt(program.uniformLocation(Slot::FogStart), fog.start);
 program.set1fAt(program.uniformLocation(Slot::FogEnd), fog.end);
 program.set1iAt(program.uniformLocation(Slot::FogMode), on ? fogModeToGlConstant(fog.mode) : 0);
 program.set1iAt(program.uniformLocation(Slot::FogShape), on ? fog.shape : -1);
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
  active->set1i("tex", 0);
  active->set1i("gtexture", 0);
  active->set1i("flw_diffuseTex", 0);
  active->set1i("texture", 0);
  active->set1i("u_MainSampler", 0);
 }
 if(pass.fullscreen) {
  activeTexture(gl::tex::Texture0);
  g_lastProgram = active;
  g_matricesUploaded = false;
  return;
 }
 const math::Matrix4f& modelView = pass.modelView;
 const math::Matrix4f& projection = pass.projection;
 const bool matricesChanged = !g_matricesUploaded || programChanged ||
                              std::memcmp(g_uploadedModelView.m, modelView.m, sizeof(modelView.m)) != 0 ||
                              std::memcmp(g_uploadedProjection.m, projection.m, sizeof(projection.m)) != 0;
 if(matricesChanged) {
  const bool isPassBase = g_drawCameraValid &&
                          std::memcmp(modelView.m, g_drawModelView.m, sizeof(modelView.m)) == 0 &&
                          std::memcmp(projection.m, g_drawProjection.m, sizeof(projection.m)) == 0;
  math::Matrix4f modelViewInverse = isPassBase ? g_drawModelViewInverse : modelView;
  math::Matrix4f projectionInverse = isPassBase ? g_drawProjectionInverse : projection;
  if(!isPassBase) {
   modelViewInverse.invert();
   projectionInverse.invert();
  }
  uploadGeometryDrawMatrices(*active, modelView.m, projection.m, modelViewInverse.m, projectionInverse.m);
  g_uploadedModelView = modelView;
  g_uploadedProjection = projection;
  g_matricesUploaded = true;
 }
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
 const int blendFunc[4] = {glBits.blendSrc, glBits.blendDst, glBits.blendSrcAlpha, glBits.blendDstAlpha};
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
 const bool snapshotChanged = active->needsUniformSnapshot(g_programUniformGeneration);
 activeTexture(gl::tex::Texture0);
 const int diffuseTexture = boundTexture();
 const std::optional<WorldProgramId> worldProgram =
     pass.programOverride == nullptr ? g_activeWorldProgram : std::nullopt;
 const bool materialChanged = active != g_programUniformMaterialProgram ||
                              worldProgram != g_programUniformMaterialWorldProgram ||
                              diffuseTexture != g_programUniformDiffuseTexture;
 Pipeline* pipeline = shaderPipeline();
 if((snapshotChanged || programChanged) && pipeline != nullptr && worldProgram.has_value()) {
  // Rebinding still has to re-point the samplers -- texture units are global -- but
  // re-uploading a snapshot the program already holds was the bulk of this call, and
  // mod item draws that ping-pong between programs paid it on every single draw.
  pipeline->bindWorldProgramState(*active, *worldProgram, snapshotChanged);
  if(snapshotChanged) active->markUniformSnapshotPushed(g_programUniformGeneration);
 }
 if(materialChanged && pipeline != nullptr && worldProgram.has_value()) {
  pipeline->bindWorldProgramMaterial(*active, *worldProgram);
  g_programUniformMaterialProgram = active;
  g_programUniformMaterialWorldProgram = worldProgram;
  g_programUniformDiffuseTexture = diffuseTexture;
 }
 // see third_party/mcp/iris/pipeline/programs/ShaderKey.java
 const bool fogOn = g_fog.enabled && active->fogClass();
 if(!g_fogUploaded || programChanged || fogOn != g_uploadedFogOn ||
    g_uploadedFog.mode != g_fog.mode || g_uploadedFog.shape != g_fog.shape ||
    g_uploadedFog.density != g_fog.density || g_uploadedFog.start != g_fog.start ||
    g_uploadedFog.end != g_fog.end ||
    std::memcmp(g_uploadedFog.color, g_fog.color, sizeof(float) * 3) != 0) {
  uploadFogUniforms(*active, g_fog, fogOn);
  g_uploadedFog = g_fog;
  g_uploadedFogOn = fogOn;
  g_fogUploaded = true;
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
 activeTexture(gl::tex::Texture0);
 bindTexture(diffuseTexture);
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
  configureAttribs(pass.buffer, pass.byteOffset, pass.stride, pass.hasTexture, pass.hasNormals,
                   pass.overrideColor, pass.colorOverride);
  const int mode = active != nullptr && active->tessellation() ? 0x000E : pass.glMode;
  if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
  ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(pass.vertexCount));
 } else {
  uploadStreaming(pass.vertexData, pass.vertexCount * static_cast<std::size_t>(pass.stride));
  configureAttribs(g_streamVbo.handle(), 0, pass.stride, pass.hasTexture, pass.hasNormals,
                   pass.overrideColor, pass.colorOverride);
  const int mode = active != nullptr && active->tessellation() ? 0x000E : pass.glMode;
  if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
  ::glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(pass.vertexCount));
 }
}
void submitIndexedQuads(const RenderPass& pass, unsigned indexBuffer, int indexCount) {
 if(!g_drawEnabled || !ensureReady() || pass.vertexCount == 0 || indexCount == 0 || pass.buffer == 0) {
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
 configureAttribs(pass.buffer, pass.byteOffset, pass.stride, pass.hasTexture, pass.hasNormals,
                  pass.overrideColor, pass.colorOverride);
 constexpr unsigned kElementArrayBuffer = 0x8893;
 gl::GLCore::bindBuffer(kElementArrayBuffer, indexBuffer);
 ::glDrawElements(0x0004, indexCount, 0x1405, nullptr); // GL_TRIANGLES, GL_UNSIGNED_INT
}
bool configureIndexedVao(unsigned vao,
                         unsigned vertexBuffer,
                         unsigned indexBuffer,
                         std::size_t baseOffset,
                         int stride,
                         bool hasTexture,
                         bool hasNormals) {
 if(vao == 0 || vertexBuffer == 0 || indexBuffer == 0 || !gl::GLCore::vaoSupported ||
    gl::GLCore::bindVertexArray == nullptr) {
  return false;
 }
 gl::GLCore::bindVertexArray(vao);
 gl::GLCore::bindBuffer(kArrayBuffer, vertexBuffer);
 for(const abi::Format& format : abi::Formats) {
  const bool enabled = format.availability == abi::Availability::Always ||
                       (format.availability == abi::Availability::Texture && hasTexture) ||
                       (format.availability == abi::Availability::Normal && hasNormals);
  if(!enabled) {
   gl::GLCore::disableVertexAttribArray(format.location);
   continue;
  }
  gl::GLCore::enableVertexAttribArray(format.location);
  if(format.integer) {
   if(gl::GLCore::vertexAttribIPointer != nullptr) {
    gl::GLCore::vertexAttribIPointer(format.location, format.components, format.type, stride,
                                     bufOffset(baseOffset + format.offset));
   }
  } else {
   gl::GLCore::vertexAttribPointer(format.location, format.components, format.type,
                                   format.normalized ? 1 : 0, stride,
                                   bufOffset(baseOffset + format.offset));
  }
 }
 if(!hasTexture) {
  gl::GLCore::vertexAttrib4f(abi::Texture, 0.0f, 0.0f, 0.0f, 1.0f);
 }
 gl::GLCore::disableVertexAttribArray(abi::ChunkFade);
 if(gl::GLCore::vertexAttrib4f != nullptr) {
  gl::GLCore::vertexAttrib4f(abi::ChunkFade, 1.0f, 0.0f, 0.0f, 0.0f);
 }
 constexpr unsigned kElementArrayBuffer = 0x8893;
 gl::GLCore::bindBuffer(kElementArrayBuffer, indexBuffer);
 gl::GLCore::bindVertexArray(0);
 g_attribCache = AttribCache{};
 return true;
}
int submitIndexedQuadsBatch(const RenderPass& pass,
                            unsigned vao,
                            std::span<const int> indexCounts,
                            std::span<const int> baseVertices) {
 if(!g_drawEnabled || !ensureReady() || pass.vertexCount == 0 || vao == 0 || indexCounts.empty() ||
    indexCounts.size() != baseVertices.size() || gl::GLCore::drawElementsBaseVertex == nullptr) {
  return 0;
 }
 if(!pass.hasTexture) {
  bindWhiteDiffuse();
 }
 bindAndUploadUniforms(pass);
 ShaderProgram* active = pass.programOverride != nullptr ? pass.programOverride : g_activeProgram;
 if(active == nullptr) {
  return 0;
 }
 gl::GLCore::bindVertexArray(vao);
 g_attribCache = AttribCache{};
 const unsigned mode = active->tessellation() ? 0x000E : 0x0004;
 if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) {
  gl::GLCore::patchParameteri(0x8E72, 3);
 }
 int submitted = 0;
 {
  if(gl::GLCore::multiDrawElementsBaseVertex != nullptr && indexCounts.size() > 1) {
   static std::vector<const void*> offsets;
   offsets.resize(indexCounts.size(), nullptr);
   gl::GLCore::multiDrawElementsBaseVertex(mode,
                                           indexCounts.data(),
                                           0x1405,
                                           offsets.data(),
                                           static_cast<int>(indexCounts.size()),
                                           baseVertices.data());
   submitted = 1;
  } else {
   for(std::size_t i = 0; i < indexCounts.size(); ++i) {
    gl::GLCore::drawElementsBaseVertex(mode, indexCounts[i], 0x1405, nullptr, baseVertices[i]);
   }
   submitted = static_cast<int>(indexCounts.size());
  }
 }
 return submitted;
}
void unbindVertexArray() {
 if(gl::GLCore::vaoSupported && gl::GLCore::bindVertexArray != nullptr) {
  gl::GLCore::bindVertexArray(0);
 }
 g_attribCache = AttribCache{};
}
void setActiveProgram(ShaderProgram* program, std::optional<WorldProgramId> worldProgram) {
 if(g_activeProgram == program && g_activeWorldProgram == worldProgram) {
  return;
 }
 g_activeProgram = program;
 g_activeWorldProgram = worldProgram;
 g_lastProgram = nullptr;
 g_chunkOffsetUploaded = false;
}
std::optional<WorldProgramId> activeWorldProgram() {
 return g_activeWorldProgram;
}
void setDrawEnabled(bool enabled) {
 g_drawEnabled = enabled;
}
bool drawEnabled() {
 return g_drawEnabled;
}
void advanceProgramUniforms() {
 ++g_programUniformGeneration;
 g_programUniformMaterialProgram = nullptr;
 g_programUniformMaterialWorldProgram.reset();
 g_programUniformDiffuseTexture = -1;
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
 const auto channel = [](float value) {
  return static_cast<std::uint32_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)) & 0xFFU;
 };
 g_constColorPacked = channel(r) | (channel(g) << 8U) | (channel(b) << 16U) | (channel(a) << 24U);
}
const float* constColor() {
 return g_constColor;
}
std::uint32_t constColorPacked() {
 return g_constColorPacked;
}
void setAlphaTestRef(float ref) {
#ifndef NDEBUG
 // WI-5: g_alphaTestRef is main-GL-thread-write-only; mesh workers never touch
 // it (the value is captured into the ChunkMeshJob snapshot instead).
 net::minecraft::util::concurrent::assertOnMainThread();
#endif
 if(g_alphaTestRef != ref) {
  g_alphaTestRef = ref;
 }
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
 settings.enabled = g_modFogEnabled;
 settings.spherical = g_fog.shape == 0;
 settings.exponential = g_modFogExponential;
 settings.start = g_modFogStart;
 settings.end = g_modFogEnd;
 settings.density = g_modFogDensity;
 net::minecraft::mod::runtime::luaHookFogSettings(settings);
 g_modFogEnabled = settings.enabled;
 g_fog.shape = settings.spherical ? 0 : 1;
 g_modFogExponential = settings.exponential;
 g_modFogStart = settings.start;
 g_modFogEnd = settings.end;
 g_modFogDensity = settings.density;
 const Vec3d sky = world.getSkyColor(client->camera, tickDelta);
 const Vec3d fogColor = world.getFogColor(tickDelta);
 const float colorBlend = frame.renderDistance.fogColorBlend();
 float r = static_cast<float>(fogColor.x + (sky.x - fogColor.x) * colorBlend);
 float g = static_cast<float>(fogColor.y + (sky.y - fogColor.y) * colorBlend);
 float b = static_cast<float>(fogColor.z + (sky.z - fogColor.z) * colorBlend);
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
void fogApplyMode(::net::minecraft::client::Minecraft* client, bool skyPass,
                  const ::net::minecraft::client::option::RenderSettings& frame) {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 // The exponential branches derive `end` from density alone, so the sky pass writes
 // the same value the terrain pass did; it used to be skipped, which left `end` at
 // whatever the previous pass had (0 on the first frame).
 if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  const float density = frame.clearWater ? 0.02f : 0.1f;
  g_fog.mode = 2;
  g_fog.density = density;
  g_fog.end = 3.0f / density;
 } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
  g_fog.mode = 2;
  g_fog.density = 2.0f;
  g_fog.end = 1.5f;
 } else if(g_modFogEnabled && g_modFogExponential) {
  // The mod-provided density is a plain uniform value (Iris FogUniforms.fogDensity =
  // max(0, captured density)) — it must not be scaled by the render distance.
  g_fog.mode = 2;
  g_fog.density = g_modFogDensity;
  g_fog.end = g_modFogDensity > 0.0f ? 3.0f / g_modFogDensity : frame.renderDistance.blocks;
 } else {
  g_fog.mode = 1;
  // see third_party/mcp/iris/uniforms/FogUniforms.java
  const float fogEnd = frame.renderDistance.fogEnd();
  if(skyPass) {
   // Beta setupFog(-1): start 0, end farPlaneDistance * 0.8, so the sky blends toward
   // the fog colour instead of meeting fogged terrain unblended.
   g_fog.start = 0.0f;
   g_fog.end = frame.renderDistance.skyFogEnd();
  } else {
   g_fog.end = fogEnd * (g_modFogEnabled ? g_modFogEnd : 1.0f);
   const float start = g_modFogEnabled ? fogEnd * g_modFogStart : frame.renderDistance.fogStart();
   g_fog.start = std::min(start, g_fog.end * 0.9f);
  }
  if(client->world->dimension != nullptr && client->world->dimension->isNether) {
   g_fog.start = 0.0f;
  }
 }
}
void setSkyUniforms(const SkyUniforms& sky) {
 g_skyUniforms = sky;
 ++g_globalsGeneration;
}
void setCelestialState(const CelestialState& state) {
 g_celestialState = state;
 ++g_globalsGeneration;
}
const CelestialState& celestialState() {
 return g_celestialState;
}
void setCameraFrame(FrameRenderCamera camera) {
 g_cameraFrame = camera;
}
const FrameRenderCamera& cameraFrame() {
 return g_cameraFrame;
}
const SkyUniforms& skyUniforms() {
 return g_skyUniforms;
}
void configureAttribs(unsigned buffer,
                      std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasNormals,
                      bool overrideColor,
                      std::uint32_t colorOverride) {
 const bool cached = g_attribCache.valid && buffer != 0 && g_attribCache.buffer == buffer &&
                     g_attribCache.baseOffset == baseOffset && g_attribCache.stride == stride &&
                     g_attribCache.hasTexture == hasTexture &&
                     g_attribCache.hasNormals == hasNormals &&
                     g_attribCache.overrideColor == overrideColor &&
                     g_attribCache.colorOverride == colorOverride;
 if(!cached) {
  if(g_vao.handle() != 0) {
   gl::GLCore::bindVertexArray(g_vao.handle());
  }
  if(buffer != 0) {
   gl::GLCore::bindBuffer(kArrayBuffer, buffer);
  }
  for(const abi::Format& format : abi::Formats) {
   const bool enabled = format.location != abi::Color || !overrideColor;
   const bool available = format.availability == abi::Availability::Always ||
                          (format.availability == abi::Availability::Texture && hasTexture) ||
                          (format.availability == abi::Availability::Normal && hasNormals);
   if(!enabled || !available) {
    gl::GLCore::disableVertexAttribArray(format.location);
    continue;
   }
   gl::GLCore::enableVertexAttribArray(format.location);
   if(format.integer) {
    if(gl::GLCore::vertexAttribIPointer != nullptr)
     gl::GLCore::vertexAttribIPointer(format.location, format.components, format.type, stride,
                                      bufOffset(baseOffset + format.offset));
   } else {
    gl::GLCore::vertexAttribPointer(format.location, format.components, format.type,
                                    format.normalized ? 1 : 0, stride,
                                    bufOffset(baseOffset + format.offset));
   }
  }
  if(!hasTexture)
   gl::GLCore::vertexAttrib4f(abi::Texture, 0.0f, 0.0f, 0.0f, 1.0f);
  if(overrideColor) {
   constexpr float scale = 1.0f / 255.0f;
   gl::GLCore::vertexAttrib4f(abi::Color,
                              static_cast<float>(colorOverride & 0xFFU) * scale,
                              static_cast<float>((colorOverride >> 8U) & 0xFFU) * scale,
                              static_cast<float>((colorOverride >> 16U) & 0xFFU) * scale,
                              static_cast<float>((colorOverride >> 24U) & 0xFFU) * scale);
  }
  gl::GLCore::disableVertexAttribArray(abi::ChunkFade);
  if(gl::GLCore::vertexAttrib4f != nullptr)
   gl::GLCore::vertexAttrib4f(abi::ChunkFade, 1.0f, 0.0f, 0.0f, 0.0f);
  g_attribCache =
      AttribCache{buffer, baseOffset, stride, hasTexture, hasNormals, overrideColor, colorOverride, buffer != 0};
 }
 if(!hasNormals) {
  const float* n = kDefaultNormal;
  if(!g_constNormalSet || std::memcmp(g_constNormal, n, sizeof(float) * 3) != 0) {
   gl::GLCore::vertexAttrib4f(abi::Normal, n[0], n[1], n[2], 0.0f);
   std::memcpy(g_constNormal, n, sizeof(float) * 3);
   g_constNormalSet = true;
  }
 }
}
void invalidateAttribCache() {
 g_attribCache = AttribCache{};
 g_constNormalSet = false;
 g_lastProgram = nullptr;
 g_matricesUploaded = false;
 g_passUniformsUploaded = false;
 g_fogUploaded = false;
 g_programUniformMaterialProgram = nullptr;
 g_programUniformMaterialWorldProgram.reset();
 g_programUniformDiffuseTexture = -1;
}
void setEntityColor(float red, float green, float blue, float alpha) {
 if(g_entityColor[0] != red || g_entityColor[1] != green || g_entityColor[2] != blue || g_entityColor[3] != alpha) {
  g_entityColor[0] = red;
  g_entityColor[1] = green;
  g_entityColor[2] = blue;
  g_entityColor[3] = alpha;
  g_entityOverlayDirty = true;
  ++g_globalsGeneration;
  ++g_programUniformGeneration;
 }
}
unsigned int entityOverlayTexture() {
 if(!g_entityOverlayTexture) {
  g_entityOverlayTexture = gl::GlTexture(genTexture());
  g_entityOverlayDirty = true;
 }
 if(!g_entityOverlayTexture) return 0;
 if(g_entityOverlayDirty) {
  const int previous = std::max(0, getActiveTextureUnit());
  const auto channel = [](float value) {
   return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
  };
  const unsigned char pixel[4] = {
      channel(g_entityColor[0]), channel(g_entityColor[1]), channel(g_entityColor[2]), channel(1.0f - g_entityColor[3])};
  activeTexture(gl::tex::Texture0 + 2);
  bindTexture(static_cast<int>(g_entityOverlayTexture.handle()));
  ::glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, pixel);
  ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
  ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
  activeTexture(gl::tex::Texture0 + previous);
  g_entityOverlayDirty = false;
 }
 return g_entityOverlayTexture.handle();
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
 int blendSrc = 0x0001; // GL_ONE
 int blendDst = 0x0000; // GL_ZERO
 int blendSrcAlpha = 0x0001; // GL_ONE
 int blendDstAlpha = 0x0000; // GL_ZERO
 bool indexedBlendDirty = false;
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
 bits.blendSrcAlpha = g_gl.blendSrcAlpha;
 bits.blendDstAlpha = g_gl.blendDstAlpha;
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
 blendFuncSeparate(bits.blendSrc, bits.blendDst, bits.blendSrcAlpha, bits.blendDstAlpha);
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
 // glBlendFunc sets BOTH pairs, so the cache must compare all four. Comparing only
 // the RGB pair let a call be elided while the alpha pair still held a pack's
 // separate-alpha factors from a previous lockBlend.
 if(g_gl.indexedBlendDirty || g_gl.blendSrc != src || g_gl.blendDst != dst || g_gl.blendSrcAlpha != src ||
    g_gl.blendDstAlpha != dst) {
  g_gl.blendSrc = src;
  g_gl.blendDst = dst;
  g_gl.blendSrcAlpha = src;
  g_gl.blendDstAlpha = dst;
  g_gl.indexedBlendDirty = false;
  ::glBlendFunc(static_cast<unsigned>(src), static_cast<unsigned>(dst));
 }
}
void blendFuncSeparate(int srcRgb, int dstRgb, int srcAlpha, int dstAlpha) {
 if(gl::GLCore::blendFuncSeparate == nullptr) {
  blendFunc(srcRgb, dstRgb);
  return;
 }
 if(g_gl.indexedBlendDirty || g_gl.blendSrc != srcRgb || g_gl.blendDst != dstRgb || g_gl.blendSrcAlpha != srcAlpha ||
    g_gl.blendDstAlpha != dstAlpha) {
  g_gl.blendSrc = srcRgb;
  g_gl.blendDst = dstRgb;
  g_gl.blendSrcAlpha = srcAlpha;
  g_gl.blendDstAlpha = dstAlpha;
  g_gl.indexedBlendDirty = false;
  gl::GLCore::blendFuncSeparate(static_cast<unsigned>(srcRgb), static_cast<unsigned>(dstRgb),
                                static_cast<unsigned>(srcAlpha), static_cast<unsigned>(dstAlpha));
 }
}
void lockBlend(const BlendMode* mode) {
 if(mode == nullptr) {
  disableBlend();
  return;
 }
 enableBlend();
 blendFuncSeparate(mode->srcRgb, mode->dstRgb, mode->srcAlpha, mode->dstAlpha);
}
void lockBufferBlend(int drawBufferIndex, const BlendMode* mode) {
 if(drawBufferIndex < 0) {
  return;
 }
 if(mode == nullptr) {
  if(gl::GLCore::blendFunci != nullptr) {
   if(gl::GLCore::blendFuncSeparatei != nullptr) {
    gl::GLCore::blendFuncSeparatei(static_cast<unsigned>(drawBufferIndex), 1, 0, 1, 0);
    g_gl.indexedBlendDirty = true;
   } else {
    gl::GLCore::blendFunci(static_cast<unsigned>(drawBufferIndex), 1, 0);
    g_gl.indexedBlendDirty = true;
   }
  }
  return;
 }
 if(gl::GLCore::blendFuncSeparatei != nullptr) {
  gl::GLCore::blendFuncSeparatei(static_cast<unsigned>(drawBufferIndex),
                                 static_cast<unsigned>(mode->srcRgb), static_cast<unsigned>(mode->dstRgb),
                                 static_cast<unsigned>(mode->srcAlpha), static_cast<unsigned>(mode->dstAlpha));
  g_gl.indexedBlendDirty = true;
 } else if(gl::GLCore::blendFunci != nullptr) {
  gl::GLCore::blendFunci(static_cast<unsigned>(drawBufferIndex), static_cast<unsigned>(mode->srcRgb),
                         static_cast<unsigned>(mode->dstRgb));
  g_gl.indexedBlendDirty = true;
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
void blendDstAlpha() {
 enableBlend();
 blendFunc(0x0302, 0x0304);
}
void blendInverseColor() {
 enableBlend();
 blendFunc(0x0000, 0x0301);
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
 const int previousUnit = g_gl.activeTexture;
 g_textureUnitOf.erase(uTex);
 for(int i = 0; i < 32; ++i) {
  if(g_gl.boundTextures[i] == uTex) {
   activeTexture(0x84C0 + i);
   ::glBindTexture(0x0DE1, 0);
   g_gl.boundTextures[i] = 0;
  }
 }
 if(previousUnit >= 0 && previousUnit < 32) activeTexture(0x84C0 + previousUnit);
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
 g_whiteTexture.reset();
 g_entityOverlayTexture.reset();
 std::lock_guard lock(g_textureMutex);
 if(!g_allocatedTextures.empty()) {
  ::glDeleteTextures(static_cast<int>(g_allocatedTextures.size()), g_allocatedTextures.data());
  g_allocatedTextures.clear();
 }
}
void releaseGlResources() {
 setActiveProgram(nullptr);
 gl::ShaderProgram::unbind();
 if(gl::GLCore::vaoSupported) gl::GLCore::bindVertexArray(0);
 if(gl::GLCore::bindBuffer != nullptr) {
  gl::GLCore::bindBuffer(kArrayBuffer, 0);
  gl::GLCore::bindBuffer(0x8893, 0);
 }
 g_fullscreenVao.reset();
 g_fullscreenVbo.reset();
 g_vao.reset();
 g_streamVbo.reset();
 g_fullscreenReady = false;
 g_triedFullscreen = false;
 invalidateAttribCache();
 clearAllocatedTextures();
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
 // Bind on the saved unit so the scope can restore the exact bind later.
 bindTexture(0x0DE1, texture);
}
TextureBindScope::~TextureBindScope() {
 // Restore on the saved unit (the saved bind belongs to that unit).
 activeTexture(0x84C0 + savedUnit_);
 bindTexture(0x0DE1, static_cast<int>(savedTex_));
}
} // namespace core
} // namespace net::minecraft::client::render
