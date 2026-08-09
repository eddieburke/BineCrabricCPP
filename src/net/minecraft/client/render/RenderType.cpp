#include "net/minecraft/client/render/RenderType.hpp"
#include <atomic>
#include <cstdio>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::render {
namespace {
std::array<std::atomic_int, 256> g_shaderBlockIds{};
DrawPhase g_drawPhase = DrawPhase::All;
Pipeline* shaderPipeline() {
 net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 return minecraft != nullptr && minecraft->gameRenderer != nullptr
            ? minecraft->gameRenderer->shaderPipeline()
            : nullptr;
}
} // namespace
int resolveShaderObjectId(const std::string& kind, const std::string& name, int fallback) {
 Pipeline* pipeline = shaderPipeline();
 return pipeline != nullptr ? pipeline->resolveObjectId(kind, name, fallback) : fallback;
}
std::uint64_t shaderObjectIdRevision() {
 Pipeline* pipeline = shaderPipeline();
 return pipeline != nullptr ? pipeline->objectIdRevision() : 0;
}
void setShaderBlockIds(const std::array<int, 256>& ids) {
 for(std::size_t i = 0; i < ids.size(); ++i) g_shaderBlockIds[i].store(ids[i], std::memory_order_relaxed);
}
int resolveShaderBlockId(int id) {
 return id >= 0 && id < static_cast<int>(g_shaderBlockIds.size())
            ? g_shaderBlockIds[static_cast<std::size_t>(id)].load(std::memory_order_relaxed)
            : id;
}
void setDrawPhase(DrawPhase phase) {
 g_drawPhase = phase;
}
RenderType::RenderType(std::string_view name,
                       int glMode,
                       bool hasTexture,
                       std::string_view programName,
                       State state,
                       std::optional<WorldProgramId> worldProgram)
    : name_(name),
      glMode_(glMode),
      hasTexture_(hasTexture),
      programName_(programName),
      state_(state),
      worldProgram_(worldProgram) {
}
void RenderType::setupRenderState() const {
 // Resolved fresh each pass so a live pack switch / recompile takes effect.
 Pipeline* pipeline = shaderPipeline();
 gl::ShaderProgram* active = pipeline != nullptr && worldProgram_.has_value()
                                 ? pipeline->worldProgram(*worldProgram_)
                                 : nullptr;
 core::setActiveProgram(active, active != nullptr ? worldProgram_ : std::nullopt);
 // Diffuse sampler is always unit 0. Untextured passes own a white texel there;
 // textured passes leave unit 0 for the caller to bind.
 if(!hasTexture_) {
  core::bindWhiteDiffuse();
 }
 if(state_.blend) {
  core::enableBlend();
  core::blendFunc(state_.blendSrc, state_.blendDst);
 } else {
  core::disableBlend();
 }
 if(state_.depthTest) {
  core::enableDepthTest();
 } else {
  core::disableDepthTest();
 }
 core::depthMask(state_.depthWrite);
 if(state_.cull) {
  core::enableCull();
  core::cullFace(state_.cullMode);
 } else {
  core::disableCull();
 }
   core::setAlphaTestRef(state_.alphaTest ? state_.alphaRef : -1.0f);
   core::setLightingEnabled(state_.lighting);
    if(pipeline != nullptr && active != nullptr && worldProgram_.has_value())
     pipeline->applyWorldPassDirectives(*worldProgram_, *active);
  }
void RenderType::restoreRenderState(const core::PassGlBits& saved,
                                    gl::ShaderProgram* savedProgram,
                                    std::optional<WorldProgramId> savedWorldProgram) const {
 core::restorePassGlBits(saved);
 // Fog is owned by the frame stage (GameRenderer), not by pass enter/exit.
 // Lighting / alpha / const color are restored from core snapshots on RenderPassScope.
 core::setActiveProgram(savedProgram, savedWorldProgram);
 Pipeline* pipeline = shaderPipeline();
 if(pipeline != nullptr && savedProgram != nullptr && savedWorldProgram.has_value())
  pipeline->applyWorldPassDirectives(*savedWorldProgram, *savedProgram);
}
RenderPassScope::RenderPassScope(const RenderType& type)
    : type_(&type),
      saved_(core::capturePassGlBits()),
      savedProgram_(core::program()),
      savedWorldProgram_(core::activeWorldProgram()),
      savedDrawEnabled_(core::drawEnabled()),
      savedRenderStage_(core::renderStage()),
      savedWorldLight_(core::worldLight()),
      savedAlphaTestRef_(core::alphaTestRef()),
      savedPose_(core::drawPose()) {
 const float* color = core::constColor();
 savedConstColor_[0] = color[0];
 savedConstColor_[1] = color[1];
 savedConstColor_[2] = color[2];
 savedConstColor_[3] = color[3];
 const bool translucent = type.state_.blend;
 const bool phaseEnabled =
     g_drawPhase == DrawPhase::All || (g_drawPhase == DrawPhase::Translucent) == translucent;
 core::setDrawEnabled(savedDrawEnabled_ && phaseEnabled);
 type_->setupRenderState();
}
RenderPassScope::~RenderPassScope() {
 type_->restoreRenderState(saved_, savedProgram_, savedWorldProgram_);
 core::setRenderStage(savedRenderStage_);
 core::setWorldLight(savedWorldLight_);
 core::setLightingEnabled(savedWorldLight_.enabled);
 core::setAlphaTestRef(savedAlphaTestRef_);
 core::setConstColor(savedConstColor_[0], savedConstColor_[1], savedConstColor_[2], savedConstColor_[3]);
 core::setDrawEnabled(savedDrawEnabled_);
 // The pose is state, so it needs a scope. Producers publish theirs inside a
 // pass (particles, block damage, entity shadows); without this the last one
 // published would follow the camera into whatever drew next.
 core::setDrawPose(savedPose_);
}
namespace {
// Chunk-mesh passes cull back faces, which is the state Iris always renders terrain in:
// its backFace.solid / backFace.cutout / backFace.cutoutMipped / backFace.translucent
// directives are parsed (ShaderProperties.java:98-101) but never consumed anywhere in
// 26.1, so no pack can turn culling off. Geometry that needs to be seen from both sides
// emits both faces itself — cross/crop plants, fire, rails and redstone all do.
// Culling off is not a free "make everything visible": a back face reaches the pack with
// its normal pointing away from the viewer and with at_tangent.w handedness computed for
// the opposite winding, which corrupts w_face_normal, the normal-mapping TBN basis and
// DIR_SHADING (prog/lit_forward.fsh), on top of rasterising every hidden face.
RenderType::State solidState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 return s;
}
RenderType::State cutoutState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 s.alphaTest = true;
 s.alphaRef = 0.5f;
 return s;
}
// Leaf-blob interiors: cutout with culling off. This is the one terrain layer
// that wants it, because each leaf<->leaf boundary contributes a single quad
// (LeafInteriorFaces.hpp) that has to be visible from both sides. Nothing in
// this layer is coincident with anything, so rasterising both windings cannot
// Z-fight. Everything above about back faces reaching the pack with an inverted
// normal and tangent handedness does apply here - it is the price of one quad
// per boundary instead of a coplanar pair.
RenderType::State cutoutInteriorState() {
 RenderType::State s = cutoutState();
 s.cull = false;
 return s;
}
RenderType::State translucentState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = false;
 return s;
}
// Terrain translucent is a chunk-mesh pass, so everything the comment above
// solidState() says applies to it: it culls back faces. Sharing translucentState()
// left culling off, which rasterises every water quad twice — the back face
// arriving with an inverted normal and at_tangent.w handedness computed for the
// opposite winding, so the pack's TBN basis and face normal are wrong on it — and
// then blends the two coincident faces on top of each other. Clouds/weather/lines
// keep translucentState(); they are not chunk meshes and do want both windings.
RenderType::State terrainTranslucentState() {
 RenderType::State s = translucentState();
 s.cull = true;
 s.alphaTest = true;
 s.alphaRef = 0.0001f;
 return s;
}
RenderType::State alphaTranslucentState() {
 RenderType::State s = translucentState();
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State damagedBlockState() {
 // Iris ShaderKey.CRUMBLING (ShaderKey.java:61) declares AlphaTests.ONE_TENTH_ALPHA
 // for the destroyed-block overlay. The destroy tiles carry a ~transparent white
 // background (a=1/255) that must be discarded, or packs with an ALPHA_CHECK +
 // TEX_ALPHA damagedblock program blend it
 // into the scene at ~2x via their DST_COLOR/SRC_COLOR buffer directive.
 RenderType::State s{};
 s.blend = true;
 s.depthTest = true;
 s.depthWrite = false;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State particlesTranslucentState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = true;
 // Vanilla particles leave the depth mask off so blended quads do not punch
 // see-through holes into the scene.
 s.depthWrite = false;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State guiState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = false;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State textState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = false;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State guiItem3DState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 // GUI 3D item lighting (two-key diffuse dirs set by ItemRenderer).
 s.lighting = true;
 return s;
}
RenderType::State skyState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = false;
 s.cull = false;
 return s;
}
RenderType::State skyTexturedState() {
 // Classic sun/moon: SRC_ALPHA, ONE so transparent black texels add nothing
 // instead of overwriting the sky dome with opaque black.
 RenderType::State s = skyState();
 s.blend = true;
 s.blendSrc = 0x0302; // GL_SRC_ALPHA
 s.blendDst = 0x0001; // GL_ONE
 return s;
}
RenderType::State entityCutoutState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 s.lighting = true;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
} // namespace
RenderType& RenderType::solid() {
 static RenderType instance =
      RenderType("solid", 0x0004, true, "", solidState(), WorldProgramId::TerrainSolid);
 return instance;
}
RenderType& RenderType::cutout() {
 static RenderType instance =
      RenderType("cutout", 0x0004, true, "", cutoutState(), WorldProgramId::TerrainCutout);
 return instance;
}
RenderType& RenderType::cutoutInterior() {
 // Same program key as cutout(): to a pack this is still gbuffers_terrain
 // cutout geometry, and it must not need a program of its own.
 static RenderType instance =
      RenderType("cutout_interior", 0x0004, true, "", cutoutInteriorState(), WorldProgramId::TerrainCutout);
 return instance;
}
RenderType& RenderType::translucent() {
 static RenderType instance =
      RenderType("translucent", 0x0004, true, "", terrainTranslucentState(), WorldProgramId::TerrainTranslucent);
 return instance;
}
RenderType& RenderType::gui() {
 static RenderType instance =
      RenderType("gui", 0x0004, false, "", guiState(), WorldProgramId::Gui);
 return instance;
}
RenderType& RenderType::guiTextured() {
 static RenderType instance =
      RenderType("gui_textured", 0x0004, true, "", guiState(), WorldProgramId::GuiTextured);
 return instance;
}
RenderType& RenderType::guiItem3D() {
 static RenderType instance =
      RenderType("gui_item_3d", 0x0004, true, "", guiItem3DState(), WorldProgramId::Item);
 return instance;
}
RenderType& RenderType::text() {
 static RenderType instance =
      RenderType("text", 0x0004, true, "", textState(), WorldProgramId::Text);
 return instance;
}
RenderType& RenderType::sky() {
 static RenderType instance =
      RenderType("sky", 0x0004, false, "", skyState(), WorldProgramId::SkyBasic);
 return instance;
}
RenderType& RenderType::skyTextured() {
 static RenderType instance =
      RenderType("sky_textured", 0x0004, true, "", skyTexturedState(), WorldProgramId::SkyTextured);
 return instance;
}
RenderType& RenderType::basic() {
 static RenderType instance =
      RenderType("basic", 0x0004, false, "", translucentState(), WorldProgramId::Basic);
 return instance;
}
RenderType& RenderType::lines() {
 static RenderType instance =
      RenderType("lines", 0x0004, false, "", translucentState(), WorldProgramId::Line);
 return instance;
}
RenderType& RenderType::entityCutout() {
 static RenderType instance =
      RenderType("entity_cutout", 0x0004, true, "", entityCutoutState(), WorldProgramId::Entities);
 return instance;
}
RenderType& RenderType::entityTranslucent() {
 static RenderType instance =
      RenderType("entity_translucent", 0x0004, true, "", alphaTranslucentState(), WorldProgramId::EntitiesTranslucent);
 return instance;
}
RenderType& RenderType::lightning() {
 static RenderType instance =
      RenderType("lightning", 0x0004, true, "", translucentState(), WorldProgramId::Lightning);
 return instance;
}
RenderType& RenderType::block() {
 static RenderType instance =
      RenderType("block", 0x0004, true, "", entityCutoutState(), WorldProgramId::Block);
 return instance;
}
RenderType& RenderType::blockTranslucent() {
 static RenderType instance =
      RenderType("block_translucent", 0x0004, true, "", alphaTranslucentState(), WorldProgramId::BlockTranslucent);
 return instance;
}
RenderType& RenderType::particles() {
 static RenderType instance =
      RenderType("particles", 0x0004, true, "", entityCutoutState(), WorldProgramId::Particles);
 return instance;
}
RenderType& RenderType::particlesTranslucent() {
 static RenderType instance =
      RenderType("particles_translucent", 0x0004, true, "", particlesTranslucentState(), WorldProgramId::ParticlesTranslucent);
 return instance;
}
 RenderType& RenderType::clouds() {
  static RenderType instance =
      RenderType("clouds", 0x0004, true, "", alphaTranslucentState(), WorldProgramId::Clouds);
  return instance;
 }
RenderType& RenderType::weather() {
 static RenderType instance =
      RenderType("weather", 0x0004, true, "", alphaTranslucentState(), WorldProgramId::Weather);
 return instance;
}
RenderType& RenderType::hand() {
 static RenderType instance =
      RenderType("hand", 0x0004, true, "", entityCutoutState(), WorldProgramId::Hand);
 return instance;
}
RenderType& RenderType::damagedBlock() {
 static RenderType instance =
      RenderType("damaged_block", 0x0004, true, "", damagedBlockState(), WorldProgramId::DamagedBlock);
 return instance;
}
} // namespace net::minecraft::client::render
