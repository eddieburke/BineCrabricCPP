#pragma once
#include <cstddef>
#include <functional>
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client {
class Minecraft;
namespace option {
struct RenderSettings;
}
}
namespace net::minecraft::client::render {
struct FrameRenderCamera;
}
namespace net::minecraft::client::render::core {
namespace math = net::minecraft::util::math;
using gl::ShaderProgram;
enum class RenderStage : int {
 None,
 Sky,
 Sunset,
 CustomSky,
 Sun,
 Moon,
 Stars,
 Void,
 TerrainSolid,
 TerrainCutoutMipped,
 TerrainCutout,
 Entities,
 BlockEntities,
 Destroy,
 Outline,
 Debug,
 HandSolid,
 TerrainTranslucent,
 Tripwire,
 Particles,
 Clouds,
 RainSnow,
 WorldBorder,
 HandTranslucent
};
void bindMatrixStacks(math::MatrixStack* modelView, math::MatrixStack* projection);
void unbindMatrixStacks();
[[nodiscard]] math::MatrixStack& modelViewStack();
[[nodiscard]] math::MatrixStack& projectionStack();
[[nodiscard]] const math::Matrix4f& currentModelView();
[[nodiscard]] const math::Matrix4f& currentProjection();
class ScopedMatrixStacks {
 public:
 ScopedMatrixStacks(math::MatrixStack& modelView, math::MatrixStack& projection) {
  bindMatrixStacks(&modelView, &projection);
 }
 ~ScopedMatrixStacks() {
  unbindMatrixStacks();
 }
 ScopedMatrixStacks(const ScopedMatrixStacks&) = delete;
 ScopedMatrixStacks& operator=(const ScopedMatrixStacks&) = delete;
};
class ScopedModelView {
 public:
 ScopedModelView() {
  modelViewStack().push();
 }
 ~ScopedModelView() {
  modelViewStack().pop();
 }
 ScopedModelView(const ScopedModelView&) = delete;
 ScopedModelView& operator=(const ScopedModelView&) = delete;
};
struct WorldLightUniforms {
 bool enabled = false;
 float sunDirView[3] = {0.0f, 0.0f, 0.0f};
 float sunDirWorld[3] = {0.0f, 0.0f, 0.0f};
 float sunColor[3] = {1.0f, 1.0f, 1.0f};
 float sunIntensity = 0.0f;
 float fillDirView[3] = {0.0f, 0.0f, 0.0f};
 float fillIntensity = 0.0f;
 float ambient[3] = {1.0f, 1.0f, 1.0f};
 float worldTime = 0.0f;
 float brightness = 0.0f;
};
struct FogUniforms {
 bool enabled = false;
 int mode = 0; // 0 off, 1 linear, 2 exp, 3 exp2
 int shape = 0;
 float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
 float density = 0.0f;
 float start = 0.0f;
 float end = 0.0f;
 // Lua fog_settings: shape / start / end / density (color via world_color only).
 bool modEnabled = false;
 bool modExponential = false;
 float modStart = 0.2f;
 float modEnd = 0.8f;
 float modDensity = 0.1f;
};
struct SkyUniforms {
 float sunDirection[3] = {0.0f, 0.0f, 1.0f};
 float skyColor[3] = {0.5f, 0.6f, 0.8f};
 float horizonColor[3] = {0.8f, 0.7f, 0.6f};
 float starBrightness = 0.0f;
 float sunIntensity = 0.0f;
 float moonPhase = 0.0f;
 bool renderStars = true;
};
// GL toggles go straight to the driver. A tiny private dirty-cache in the .cpp
// elides redundant enables/binds; it is not a restoreable CPU state machine.
// Temporary overrides use bit RAII (BlendScope / DepthScope / CullScope / …).
void blendAlpha();
void blendAdditive();
void blendDstAlpha();
void blendInverseColor();
void blendMultiply();
void blendCustom(int src, int dst);
void blendFunc(int src, int dst);
void enableBlend();
void disableBlend();
[[nodiscard]] bool blendEnabled();
[[nodiscard]] int blendSrcFactor();
[[nodiscard]] int blendDstFactor();
// Pack bufferBlend directives — sticky until unlockBlend (fullscreen pass end).
struct BlendMode {
 int srcRgb = 0x0302;
 int dstRgb = 0x0303;
 int srcAlpha = 0x0302;
 int dstAlpha = 0x0303;
};
void lockBlend(const BlendMode* mode);
void lockBufferBlend(int drawBufferIndex, const BlendMode* mode);
void unlockBlend();
void enableDepthTest();
void depthTest();
void depthTestWrite(bool write);
void disableDepthTest();
void depthFunc(int func);
void depthMask(bool enabled);
[[nodiscard]] bool depthTestEnabled();
[[nodiscard]] bool depthWriteEnabled();
void enableCull();
void cullFace(int mode);
void cullBackFaces();
void disableCull();
[[nodiscard]] bool cullEnabled();
void enablePolygonOffset();
void disablePolygonOffset();
void polygonOffset(float factor, float units);
void clearColor(float r, float g, float b, float a);
void clear(int mask);
void clearDepth(double depth);
void activeTexture(int texture);
void invalidateTextureBindCache();
void bindTexture(int texture);
void bindTexture(unsigned int texture);
void bindTexture(int target, int texture);
void unbindTexture(int texture);
[[nodiscard]] int boundTexture();
unsigned int genTexture();
void deleteTexture(unsigned int texture);
void clearAllocatedTextures();
void colorMask(bool r, bool g, bool b, bool a);
void hintFogEnabled(bool enabled);
int getActiveTextureUnit();
void viewport(int x, int y, int width, int height);
bool getCachedViewport(int outViewport[4]);
void setGuiScaleFactor(int factor);
int getGuiScaleFactor();
void getIntegerv(int pname, int* params);
// Saves current blend enable+func, then applies enable ? blendFunc(src,dst) : disable.
class BlendScope {
 public:
 explicit BlendScope(bool enable, int src = 0x0302, int dst = 0x0303);
 ~BlendScope();
 BlendScope(const BlendScope&) = delete;
 BlendScope& operator=(const BlendScope&) = delete;

 private:
 bool savedEnable_;
 int savedSrc_;
 int savedDst_;
};
class DepthScope {
 public:
 DepthScope(bool test, bool write);
 ~DepthScope();
 DepthScope(const DepthScope&) = delete;
 DepthScope& operator=(const DepthScope&) = delete;

 private:
 bool savedTest_;
 bool savedWrite_;
};
class CullScope {
 public:
 explicit CullScope(bool enable, int mode = 0x0405);
 ~CullScope();
 CullScope(const CullScope&) = delete;
 CullScope& operator=(const CullScope&) = delete;

 private:
 bool savedEnable_;
 int savedMode_;
};
class ColorMaskScope {
 public:
 ColorMaskScope(bool r, bool g, bool b, bool a);
 ~ColorMaskScope();
 ColorMaskScope(const ColorMaskScope&) = delete;
 ColorMaskScope& operator=(const ColorMaskScope&) = delete;

 private:
 bool savedR_, savedG_, savedB_, savedA_;
};
// Saves the active-unit 2D bind; optional ctor binds `texture` for the scope.
class TextureBindScope {
 public:
 TextureBindScope();
 explicit TextureBindScope(int texture);
 ~TextureBindScope();
 TextureBindScope(const TextureBindScope&) = delete;
 TextureBindScope& operator=(const TextureBindScope&) = delete;

 private:
 int savedUnit_;
 unsigned savedTex_;
};
// Compact GL bits a RenderPassScope restores — not a full GPU mirror.
struct PassGlBits {
 bool blend = false;
 bool depthTest = false;
 bool depthWrite = true;
 bool cull = false;
 int blendSrc = 0x0302; // GL_SRC_ALPHA
 int blendDst = 0x0303; // GL_ONE_MINUS_SRC_ALPHA
 int cullMode = 0x0405;
 bool colorMaskR = true;
 bool colorMaskG = true;
 bool colorMaskB = true;
 bool colorMaskA = true;
};
[[nodiscard]] PassGlBits capturePassGlBits();
void restorePassGlBits(const PassGlBits& bits);
// Draw args only — GL blend/depth/cull are set by RenderType / bit scopes.
struct RenderPass {
 math::Matrix4f modelView = math::Matrix4f::identityMatrix();
 math::Matrix4f projection = math::Matrix4f::identityMatrix();
 float chunkOffset[3] = {0.0f, 0.0f, 0.0f};
 bool sectionLocal = false;
 const void* vertexData = nullptr;
 unsigned buffer = 0;
 std::size_t byteOffset = 0;
 std::size_t vertexCount = 0;
 int stride = 0;
 int glMode = 0x0004;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 FogUniforms fog{};
  ShaderProgram* programOverride = nullptr;
  bool fullscreen = false;
};
void setDrawCameraState(const float* modelView,
                       const float* projection,
                       const float* modelViewInverse,
                       const float* projectionInverse,
                       const float* cameraPosition);
void setDrawCameraStateFromCamera(const FrameRenderCamera& camera, float farPlane);
void clearDrawCameraState();
[[nodiscard]] bool drawCameraStateValid() noexcept;
[[nodiscard]] const float* drawCameraPosition() noexcept;
class ScopedDrawCameraState {
 public:
 ScopedDrawCameraState();
 ~ScopedDrawCameraState();
 ScopedDrawCameraState(const ScopedDrawCameraState&) = delete;
 ScopedDrawCameraState& operator=(const ScopedDrawCameraState&) = delete;

 private:
 math::Matrix4f modelView_{};
 math::Matrix4f projection_{};
 math::Matrix4f modelViewInverse_{};
 math::Matrix4f projectionInverse_{};
 float cameraPosition_[3] = {0.0f, 0.0f, 0.0f};
 bool valid_ = false;
};
// NDC fullscreen triangle. Pack composites and colortex0 present both call this
// after binding their own program — no separate blit/submit aliases.
void drawFullscreen();
void setWorldLight(const WorldLightUniforms& light);
[[nodiscard]] const WorldLightUniforms& worldLight();
void setLightingEnabled(bool enabled);
[[nodiscard]] bool lightingEnabled();
// Constant vaColor fallback for meshes without a colour array (replaces FF glColor).
void setConstColor(float r, float g, float b, float a);
[[nodiscard]] const float* constColor();
// Shader alpha test reference. Disabled / ALWAYS semantics are expressed as ref 0.
void setAlphaTestRef(float ref);
[[nodiscard]] float alphaTestRef();
void setFog(const FogUniforms& fog);
const FogUniforms& fog();
void setFogEnabled(bool enabled);
void fogUpdateFromWorld(::net::minecraft::client::Minecraft* client, float tickDelta,
                        const ::net::minecraft::client::option::RenderSettings& frame);
void fogApplyMode(::net::minecraft::client::Minecraft* client, int mode,
                  const ::net::minecraft::client::option::RenderSettings& frame);
void setSkyUniforms(const SkyUniforms& sky);
const SkyUniforms& skyUniforms();
bool ensureReady();
ShaderProgram* program();
void bindAndUploadUniforms(const RenderPass& pass);
void bindAndUploadUniforms();
void submit(const RenderPass& pass);
// Pending section-local terrain params for Tessellator / bound-buffer draws.
void setPendingTerrainDraw(float chunkOffsetX, float chunkOffsetY, float chunkOffsetZ);
void clearPendingTerrainDraw();
void setActiveProgram(ShaderProgram* program);
void setDrawEnabled(bool enabled);
bool drawEnabled();
using ProgramUniformUploader = std::function<void(ShaderProgram&)>;
void setProgramUniformUploader(ProgramUniformUploader uploader);
void advanceProgramUniforms();
void setEntityId(int id);
void setBlockEntityId(int id);
void setRenderedItemId(int id);
void setRenderStage(RenderStage stage);
[[nodiscard]] RenderStage renderStage();
void setEntityColor(float red, float green, float blue, float alpha);
int entityId();
int blockEntityId();
int renderedItemId();
void setTextureFilteringMode(int mode);
// Diffuse always lives on unit 0. Untextured passes bind a 1x1 white texel there
// so gtexture * colour still works without an enableTexture flag.
void bindDiffuse(int texture);
void bindWhiteDiffuse();
class RenderedItemScope {
 public:
 explicit RenderedItemScope(int id) : previous_(renderedItemId()) { setRenderedItemId(id); }
 ~RenderedItemScope() { setRenderedItemId(previous_); }
 RenderedItemScope(const RenderedItemScope&) = delete;
 RenderedItemScope& operator=(const RenderedItemScope&) = delete;

 private:
 int previous_ = 0;
};
class RenderStageScope {
 public:
 explicit RenderStageScope(RenderStage stage) : previous_(renderStage()) { setRenderStage(stage); }
 ~RenderStageScope() { setRenderStage(previous_); }
 RenderStageScope(const RenderStageScope&) = delete;
 RenderStageScope& operator=(const RenderStageScope&) = delete;

 private:
 RenderStage previous_;
};
class BlockEntityIdScope {
 public:
 explicit BlockEntityIdScope(int id) : previous_(blockEntityId()) { setBlockEntityId(id); }
 ~BlockEntityIdScope() { setBlockEntityId(previous_); }
 BlockEntityIdScope(const BlockEntityIdScope&) = delete;
 BlockEntityIdScope& operator=(const BlockEntityIdScope&) = delete;

 private:
 int previous_ = 0;
};
class EntityIdScope {
 public:
 explicit EntityIdScope(int id) : previous_(entityId()) { setEntityId(id); }
 ~EntityIdScope() { setEntityId(previous_); }
 EntityIdScope(const EntityIdScope&) = delete;
 EntityIdScope& operator=(const EntityIdScope&) = delete;

 private:
 int previous_ = 0;
};
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals);
void drawFromBoundBuffer(unsigned buffer,
                         std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals);
void configureAttribs(unsigned buffer,
                      std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals);
void invalidateAttribCache();
} // namespace net::minecraft::client::render::core
