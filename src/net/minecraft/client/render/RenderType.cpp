#include "net/minecraft/client/render/RenderType.hpp"
#include <atomic>
#include <cstdio>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::render {
namespace {
WorldProgramResolver g_worldProgramResolver;
WorldPassDirectiveApplier g_worldPassDirectiveApplier;
ShaderObjectIdResolver g_shaderObjectIdResolver;
std::array<std::atomic_int, 256> g_shaderBlockIds{};
DrawPhase g_drawPhase = DrawPhase::All;
} // namespace
void setWorldProgramResolver(WorldProgramResolver resolver) {
 g_worldProgramResolver = std::move(resolver);
}
void setWorldPassDirectiveApplier(WorldPassDirectiveApplier applier) {
 g_worldPassDirectiveApplier = std::move(applier);
}
void setShaderObjectIdResolver(ShaderObjectIdResolver resolver) {
 g_shaderObjectIdResolver = std::move(resolver);
}
int resolveShaderObjectId(const std::string& kind, const std::string& name, int fallback) {
 return g_shaderObjectIdResolver ? g_shaderObjectIdResolver(kind, name, fallback) : fallback;
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
gl::ShaderProgram* resolveWorldProgram(const std::string& key) {
 return g_worldProgramResolver ? g_worldProgramResolver(key) : nullptr;
}
RenderType::RenderType(std::string name,
                       int glMode,
                       bool hasTexture,
                       bool hasColor,
                       bool hasNormals,
                       std::string programName,
                       State state,
                       std::string worldProgramKey)
    : name_(std::move(name)),
      glMode_(glMode),
      hasTexture_(hasTexture),
      hasColor_(hasColor),
      hasNormals_(hasNormals),
      programName_(std::move(programName)),
      state_(state),
      worldProgramKey_(std::move(worldProgramKey)) {
}
void RenderType::setupRenderState() const {
 // Resolved fresh each pass so a live pack switch / recompile takes effect.
 gl::ShaderProgram* active =
     !worldProgramKey_.empty() && g_worldProgramResolver ? g_worldProgramResolver(worldProgramKey_) : nullptr;
 core::setActiveProgram(active);
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
   core::setAlphaTestRef(state_.alphaTest ? state_.alphaRef : 0.0f);
   core::setLightingEnabled(state_.lighting);
    if(!worldProgramKey_.empty() && g_worldPassDirectiveApplier) {
     g_worldPassDirectiveApplier();
    }
  }
void RenderType::restoreRenderState(const core::PassGlBits& saved, gl::ShaderProgram* savedProgram) const {
 core::restorePassGlBits(saved);
 // Fog is owned by the frame stage (GameRenderer), not by pass enter/exit.
 // Lighting / alpha / const color are restored from core snapshots on RenderPassScope.
 core::setActiveProgram(savedProgram);
}
RenderPassScope::RenderPassScope(const RenderType& type)
    : type_(&type),
      saved_(core::capturePassGlBits()),
      savedProgram_(core::program()),
      savedDrawEnabled_(core::drawEnabled()),
      savedWorldLight_(core::worldLight()),
      savedAlphaTestRef_(core::alphaTestRef()) {
 const float* color = core::constColor();
 savedConstColor_[0] = color[0];
 savedConstColor_[1] = color[1];
 savedConstColor_[2] = color[2];
 savedConstColor_[3] = color[3];
 const bool translucent = type.state_.blend;
 core::setDrawEnabled(
     g_drawPhase == DrawPhase::All || (g_drawPhase == DrawPhase::Translucent) == translucent);
 type_->setupRenderState();
}
RenderPassScope::~RenderPassScope() {
 type_->restoreRenderState(saved_, savedProgram_);
 core::setWorldLight(savedWorldLight_);
 core::setLightingEnabled(savedWorldLight_.enabled);
 core::setAlphaTestRef(savedAlphaTestRef_);
 core::setConstColor(savedConstColor_[0], savedConstColor_[1], savedConstColor_[2], savedConstColor_[3]);
 core::setDrawEnabled(savedDrawEnabled_);
}
namespace {
RenderType::State solidState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 // Crossed plants (and leaf-style alpha) share this pass; cull must stay off so
 // single-sided cross quads remain visible from both sides.
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State cutoutState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = false;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
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
     RenderType("solid", 0x0004, true, true, true, "", solidState(), "gbuffers_terrain_solid");
 return instance;
}
RenderType& RenderType::cutout() {
 static RenderType instance =
     RenderType("cutout", 0x0004, true, true, true, "", cutoutState(), "gbuffers_terrain_cutout");
 return instance;
}
RenderType& RenderType::translucent() {
 static RenderType instance =
     RenderType("translucent", 0x0004, true, true, true, "", translucentState(), "gbuffers_water");
 return instance;
}
RenderType& RenderType::gui() {
 static RenderType instance =
     RenderType("gui", 0x0004, false, true, false, "", guiState(), "gbuffers_gui");
 return instance;
}
RenderType& RenderType::guiTextured() {
 static RenderType instance =
     RenderType("gui_textured", 0x0004, true, true, false, "", guiState(), "gbuffers_gui_textured");
 return instance;
}
RenderType& RenderType::guiItem3D() {
 static RenderType instance =
     RenderType("gui_item_3d", 0x0004, true, true, true, "", guiItem3DState(), "gbuffers_item");
 return instance;
}
RenderType& RenderType::text() {
 static RenderType instance =
     RenderType("text", 0x0004, true, true, false, "", textState(), "gbuffers_text");
 return instance;
}
RenderType& RenderType::sky() {
 static RenderType instance =
     RenderType("sky", 0x0004, false, false, false, "", skyState(), "gbuffers_skybasic");
 return instance;
}
RenderType& RenderType::skyTextured() {
 static RenderType instance =
     RenderType("sky_textured", 0x0004, true, true, false, "", skyTexturedState(), "gbuffers_skytextured");
 return instance;
}
RenderType& RenderType::basic() {
 static RenderType instance =
     RenderType("basic", 0x0004, false, true, false, "", translucentState(), "gbuffers_basic");
 return instance;
}
RenderType& RenderType::lines() {
 static RenderType instance =
     RenderType("lines", 0x0004, false, true, false, "", translucentState(), "gbuffers_line");
 return instance;
}
RenderType& RenderType::entityCutout() {
 static RenderType instance =
     RenderType("entity_cutout", 0x0004, true, true, true, "", entityCutoutState(), "gbuffers_entities");
 return instance;
}
RenderType& RenderType::entityTranslucent() {
 static RenderType instance =
     RenderType("entity_translucent", 0x0004, true, true, true, "", translucentState(), "gbuffers_entities_translucent");
 return instance;
}
RenderType& RenderType::lightning() {
 static RenderType instance =
     RenderType("lightning", 0x0004, true, true, true, "", translucentState(), "gbuffers_lightning");
 return instance;
}
RenderType& RenderType::block() {
 static RenderType instance =
     RenderType("block", 0x0004, true, true, true, "", entityCutoutState(), "gbuffers_block");
 return instance;
}
RenderType& RenderType::blockTranslucent() {
 static RenderType instance =
     RenderType("block_translucent", 0x0004, true, true, true, "", translucentState(), "gbuffers_block_translucent");
 return instance;
}
RenderType& RenderType::particles() {
 static RenderType instance =
     RenderType("particles", 0x0004, true, true, false, "", entityCutoutState(), "gbuffers_particles");
 return instance;
}
RenderType& RenderType::particlesTranslucent() {
 static RenderType instance =
     RenderType("particles_translucent", 0x0004, true, true, false, "", particlesTranslucentState(), "gbuffers_particles_translucent");
 return instance;
}
RenderType& RenderType::clouds() {
 static RenderType instance =
     RenderType("clouds", 0x0004, true, true, false, "", translucentState(), "gbuffers_clouds");
 return instance;
}
RenderType& RenderType::weather() {
 static RenderType instance =
     RenderType("weather", 0x0004, true, true, false, "", translucentState(), "gbuffers_weather");
 return instance;
}
RenderType& RenderType::hand() {
 static RenderType instance =
     RenderType("hand", 0x0004, true, true, true, "", entityCutoutState(), "gbuffers_hand");
 return instance;
}
RenderType& RenderType::damagedBlock() {
 static RenderType instance =
     RenderType("damaged_block", 0x0004, true, true, true, "", translucentState(), "gbuffers_damagedblock");
 return instance;
}
} // namespace net::minecraft::client::render
