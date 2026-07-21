#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
namespace net::minecraft::client::render {
namespace {
const char* kVersion = "#version 330 core\n";
gl::ProgramCache g_shaderCache;
const char* kRendertypeGuiVsh = R"(
in vec3 aPos;
in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec4 vColor;

void main() {
  gl_Position = uProjection * uModelView * vec4(aPos, 1.0);
  vColor = aColor;
}
)";
const char* kRendertypeGuiFsh = R"(
in vec4 vColor;

uniform vec4 uConstColor;

layout(location = 0) out vec4 fragColor;

void main() {
  fragColor = uConstColor * vColor;
}
)";
const char* kRendertypeGuiTexturedVsh = R"(
in vec3 aPos;
in vec2 aUV;
in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;

void main() {
  gl_Position = uProjection * uModelView * vec4(aPos, 1.0);
  vUV = aUV;
  vColor = aColor;
}
)";
const char* kRendertypeGuiTexturedFsh = R"(
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform vec4 uConstColor;

layout(location = 0) out vec4 fragColor;

void main() {
  fragColor = texture(uTexture, vUV) * vColor * uConstColor;
}
)";
const char* kRendertypeTextVsh = R"(
in vec3 aPos;
in vec2 aUV;
in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;

void main() {
  gl_Position = uProjection * uModelView * vec4(aPos, 1.0);
  vUV = aUV;
  vColor = aColor;
}
)";
const char* kRendertypeTextFsh = R"(
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform vec4 uConstColor;
uniform int uAlphaTest;
uniform float uAlphaRef;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 base = uConstColor * vColor * texture(uTexture, vUV);
  if(uAlphaTest == 1 && base.a <= uAlphaRef) {
    discard;
  }
  fragColor = base;
}
)";
WorldProgramResolver g_worldProgramResolver;
} // namespace
void setWorldProgramResolver(WorldProgramResolver resolver) {
 g_worldProgramResolver = std::move(resolver);
}
gl::ShaderProgram* RenderType::loadProgram(const std::string& baseName) {
 if(baseName == "rendertype_gui") {
  return g_shaderCache.getFromSource("rendertype_gui", kRendertypeGuiVsh, kRendertypeGuiFsh, kVersion);
 }
 if(baseName == "rendertype_gui_textured") {
  return g_shaderCache.getFromSource(
      "rendertype_gui_textured", kRendertypeGuiTexturedVsh, kRendertypeGuiTexturedFsh, kVersion);
 }
 if(baseName == "rendertype_text") {
  return g_shaderCache.getFromSource("rendertype_text", kRendertypeTextVsh, kRendertypeTextFsh, kVersion);
 }
 return nullptr;
}
RenderType::RenderType(std::string name, int glMode, bool hasTexture, bool hasColor, bool hasNormals,
                       bool hasLightmap, std::string programName, State state,
                       std::string worldProgramKey)
    : name_(std::move(name)), glMode_(glMode), hasTexture_(hasTexture), hasColor_(hasColor),
      hasNormals_(hasNormals), hasLightmap_(hasLightmap), programName_(std::move(programName)),
      state_(state), worldProgramKey_(std::move(worldProgramKey)) {}
void RenderType::setupRenderState() const {
 gl::ShaderProgram* active = nullptr;
 if(!worldProgramKey_.empty()) {
  // Resolved fresh each pass so a live pack switch / recompile takes effect. Not cached
  // in program_, which is reserved for the engine-internal (gui/text) programs.
  active = g_worldProgramResolver ? g_worldProgramResolver(worldProgramKey_) : nullptr;
 } else {
  if(program_ == nullptr && !programName_.empty()) {
   program_ = loadProgram(programName_);
  }
  active = program_;
 }
 gl::engine_pipeline::setActiveProgram(active);
 if(hasTexture_) {
  RenderSystem::enableTexture();
 } else {
  RenderSystem::disableTexture();
 }
 if(state_.blend) {
  RenderSystem::enableBlend();
  RenderSystem::blendFunc(state_.blendSrc, state_.blendDst);
 } else {
  RenderSystem::disableBlend();
 }
 if(state_.depthTest) {
  RenderSystem::enableDepthTest();
 } else {
  RenderSystem::disableDepthTest();
 }
 RenderSystem::depthMask(state_.depthWrite);
 if(state_.cull) {
  RenderSystem::enableCull();
  RenderSystem::cullFace(state_.cullMode);
 } else {
  RenderSystem::disableCull();
 }
 if(state_.alphaTest) {
  RenderSystem::alphaFunc(0x0204, state_.alphaRef); // GL_GREATER
 } else {
  RenderSystem::alphaFunc(0x0207, state_.alphaRef); // GL_ALWAYS
 }
}
void RenderType::restoreRenderState(const RenderSystem::StateShadow& saved,
                                    gl::ShaderProgram* savedProgram) const {
 if(saved.texture2D) {
  RenderSystem::enableTexture();
 } else {
  RenderSystem::disableTexture();
 }
 if(saved.blend) {
  RenderSystem::enableBlend();
 } else {
  RenderSystem::disableBlend();
 }
 RenderSystem::blendFunc(saved.blendSrc, saved.blendDst);
 if(saved.depthTest) {
  RenderSystem::enableDepthTest();
 } else {
  RenderSystem::disableDepthTest();
 }
 RenderSystem::depthMask(saved.depthWrite);
 if(saved.cullFace) {
  RenderSystem::enableCull();
 } else {
  RenderSystem::disableCull();
 }
 RenderSystem::cullFace(saved.cullFaceMode);
 RenderSystem::alphaFunc(saved.alphaFunc, saved.alphaRef);
 RenderSystem::colorMask(saved.colorMaskR, saved.colorMaskG, saved.colorMaskB, saved.colorMaskA);
 if(saved.viewportValid) {
  RenderSystem::viewport(saved.viewport[0], saved.viewport[1],
                         saved.viewport[2], saved.viewport[3]);
 }
 {
  auto fog = gl::engine_pipeline::fog();
  if(saved.fogEnabled != fog.enabled) {
   fog.enabled = saved.fogEnabled;
   gl::engine_pipeline::setFog(fog);
  }
 }
 gl::engine_pipeline::setActiveProgram(savedProgram);
}
RenderPassScope::RenderPassScope(const RenderType& type)
    : type_(&type), saved_(RenderSystem::getShadow()),
      savedProgram_(gl::engine_pipeline::getActiveProgram()) {
 type_->setupRenderState();
}
RenderPassScope::~RenderPassScope() {
 type_->restoreRenderState(saved_, savedProgram_);
}
namespace {
RenderType::State solidState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 s.alphaTest = true;
 s.alphaRef = 0.1f;
 return s;
}
RenderType::State cutoutState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 return s;
}
RenderType::State translucentState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = true;
 s.cull = false;
 return s;
}
RenderType::State guiState() {
 RenderType::State s{};
 s.blend = true;
 s.depthTest = false;
 s.cull = false;
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
RenderType::State entityCutoutState() {
 RenderType::State s{};
 s.depthTest = true;
 s.depthWrite = true;
 s.cull = true;
 s.lighting = true;
 return s;
}
} // namespace
RenderType& RenderType::solid() {
 static RenderType instance =
     RenderType("solid", 0x0004, true, true, true, true, "", solidState(), "gbuffers_terrain");
 return instance;
}
RenderType& RenderType::cutout() {
 static RenderType instance =
     RenderType("cutout", 0x0004, true, true, true, false, "", cutoutState(), "gbuffers_entities");
 return instance;
}
RenderType& RenderType::translucent() {
 static RenderType instance = RenderType("translucent", 0x0004, true, true, true, true, "", translucentState(), "gbuffers_terrain");
 return instance;
}
RenderType& RenderType::gui() {
 static RenderType instance = RenderType("gui", 0x0004, false, true, false, false, "rendertype_gui", guiState());
 return instance;
}
RenderType& RenderType::guiTextured() {
 static RenderType instance =
     RenderType("gui_textured", 0x0004, true, true, false, false, "rendertype_gui_textured", guiState());
 return instance;
}
RenderType& RenderType::guiItem3D() {
 static RenderType instance =
     RenderType("gui_item_3d", 0x0004, true, true, true, false, "", guiItem3DState(), "gbuffers_entities");
 return instance;
}
RenderType& RenderType::text() {
 static RenderType instance = RenderType("text", 0x0004, true, true, false, false, "rendertype_text", textState());
 return instance;
}
RenderType& RenderType::sky() {
 static RenderType instance = RenderType("sky", 0x0004, false, false, false, false, "", skyState(), "gbuffers_sky");
 return instance;
}
RenderType& RenderType::entityCutout() {
 static RenderType instance = RenderType("entity_cutout", 0x0004, true, true, true, false, "", entityCutoutState(), "gbuffers_entities");
 return instance;
}
} // namespace net::minecraft::client::render
