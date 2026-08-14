#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/AsyncDepthSampler.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/texture/DynamicTexture.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "support/glsl_snippets_test_fixture.hpp"
namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
using client::render::PackCompiler;
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::PackSetting;
using client::render::PackCatalog::directoryResources;
class ShaderGlIntegrationTest : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "shader-test", nullptr, nullptr);
  if(window_ == nullptr) {
   glfwDefaultWindowHints();
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   window_ = glfwCreateWindow(64, 64, "shader-test", nullptr, nullptr);
  }
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  GLCore::ensureLoaded();
  ASSERT_TRUE(GLCore::shaderSupported);
  ASSERT_TRUE(GLCore::computeSupported);
 }
 static void TearDownTestSuite() {
  client::render::core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
 }
 void SetUp() override {
  if(window_ != nullptr) {
   glfwMakeContextCurrent(window_);
  }
  client::render::core::setActiveProgram(nullptr);
  client::render::core::setDrawEnabled(true);
  client::render::core::invalidateAttribCache();
  client::gl::ShaderProgram::unbind();
  client::gl::GLCore::bindFramebuffer(client::gl::framebuffer::Framebuffer, 0);
  client::render::core::activeTexture(client::gl::tex::Texture0);
  client::render::core::disableBlend();
  client::render::core::disableDepthTest();
  client::render::core::disableCull();
  client::render::setDrawPhase(client::render::DrawPhase::All);
  while(::glGetError() != 0u) {}
 }
 static GLFWwindow* window_;
};
GLFWwindow* ShaderGlIntegrationTest::window_ = nullptr;
void mergeDimension(PackDefinition& target, const PackDefinition& selected) {
 for(const auto& [name, program] : selected.programs) {
  target.programs[name] = program;
 }
 for(const auto& [name, shaderTarget] : selected.targets) {
  target.targets[name] = shaderTarget;
 }
 target.gbufferColorBuffers = std::max(target.gbufferColorBuffers, selected.gbufferColorBuffers);
 target.shadowColorBuffers = std::max(target.shadowColorBuffers, selected.shadowColorBuffers);
 if(selected.shadowMapResolution > 0) {
  target.shadowMapResolution = selected.shadowMapResolution;
 }
 if(!selected.customUniforms.empty()) {
  target.customUniforms = selected.customUniforms;
 }
 for(const auto& pass : selected.passes) {
  const auto match = std::find_if(target.passes.begin(), target.passes.end(), [&pass](const auto& root) {
   return root.type == pass.type && root.name == pass.name;
  });
  if(match == target.passes.end()) {
   target.passes.push_back(pass);
  } else {
   *match = pass;
  }
 }
}
std::string join(const std::vector<std::string>& lines) {
 std::ostringstream output;
 for(const std::string& line : lines) {
  output << line << '\n';
 }
 return output.str();
}
class TextureBindingEntityRenderer final : public client::render::entity::EntityRenderer {
 public:
 void render(const net::minecraft::Entity&,
             double,
             double,
             double,
             float,
             float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override {}
};
class ReplicatedDynamicTexture final : public client::render::texture::DynamicTexture {
 public:
 explicit ReplicatedDynamicTexture(int texture) : DynamicTexture(34), texture_(texture) {
  replicate = 2;
 }
 void tick() override {
  for(std::size_t pixel = 0; pixel < pixels.size(); pixel += 4) {
   pixels[pixel] = 17;
   pixels[pixel + 1] = 83;
   pixels[pixel + 2] = 149;
   pixels[pixel + 3] = 255;
  }
 }
 void bind(client::texture::TextureManager&) override {
  client::render::core::bindTexture(texture_);
 }

 private:
 int texture_;
};
} // namespace
TEST_F(ShaderGlIntegrationTest, DrawBufferBindingQueryIsValid) {
 ASSERT_NE(GLCore::genFramebuffers, nullptr);
 ASSERT_NE(GLCore::bindFramebuffer, nullptr);
}
TEST_F(ShaderGlIntegrationTest, ProgramCacheReloadsStoredBinary) {
 const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
 const std::filesystem::path root =
     std::filesystem::temp_directory_path() / ("minecraft_omega_program_cache_" + std::to_string(nonce));
 const std::string vertex = "void main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}";
 const std::string fragment = "layout(location=0) out vec4 color;void main(){color=vec4(1.0);}";
 const std::string preamble = "#version 430 core\n";
 bool stored = false;
 {
  auto binaries = std::make_shared<client::gl::ShaderBinaryCache>(root);
  client::gl::ProgramCache first(binaries);
  ASSERT_NE(first.getFromSource("test", vertex, fragment, preamble), nullptr);
  EXPECT_EQ(first.stats().sourceCompiles, 1u);
  stored = first.stats().binaryStores == 1u;
  if(stored) {
   client::gl::ProgramCache immediate(binaries);
   ASSERT_NE(immediate.getFromSource("test", vertex, fragment, preamble), nullptr);
   EXPECT_EQ(immediate.stats().binaryHits, 1u);
   EXPECT_EQ(immediate.stats().sourceCompiles, 0u);
  }
 }
 if(!stored) {
  std::error_code error;
  std::filesystem::remove_all(root, error);
  GTEST_SKIP() << "GL implementation exposes no retrievable program binary";
 }
 {
  client::gl::ProgramCache cache(root);
  ASSERT_NE(cache.getFromSource("test", vertex, fragment, preamble), nullptr);
  EXPECT_EQ(cache.stats().binaryHits, 1u);
  EXPECT_EQ(cache.stats().sourceCompiles, 0u);
 }
 std::error_code error;
 std::filesystem::remove_all(root, error);
 EXPECT_FALSE(error);
}
TEST_F(ShaderGlIntegrationTest, CenterDepthReadbackDoesNotWaitForCurrentIssue) {
 client::render::AsyncDepthSampler sampler;
 ::glClearDepth(0.25);
 ::glClear(client::gl::attrib::DepthBufferBit);
 std::optional<float> depth;
 for(int attempt = 0; attempt < 8 && !depth.has_value(); ++attempt) {
  depth = sampler.pollAndIssue(1, 1);
  ::glFinish();
 }
 ASSERT_TRUE(depth.has_value());
 EXPECT_NEAR(*depth, 0.25f, 0.01f);
}
TEST_F(ShaderGlIntegrationTest, NestedPassCannotReenableAFilteredParent) {
 using client::render::DrawPhase;
 using client::render::RenderPassScope;
 using client::render::RenderType;
 client::render::setDrawPhase(DrawPhase::Opaque);
 client::render::core::setDrawEnabled(true);
 {
  const RenderPassScope parent(RenderType::entityTranslucent());
  EXPECT_FALSE(client::render::core::drawEnabled());
  {
   const RenderPassScope child(RenderType::entityCutout());
   EXPECT_FALSE(client::render::core::drawEnabled());
  }
  EXPECT_FALSE(client::render::core::drawEnabled());
 }
 EXPECT_TRUE(client::render::core::drawEnabled());
 client::render::setDrawPhase(DrawPhase::All);
}
TEST_F(ShaderGlIntegrationTest, IrisAlphaThresholdsReachWaterAndEntityPasses) {
 using client::render::RenderPassScope;
 using client::render::RenderType;
 client::render::core::setAlphaTestRef(-1.0f);
 {
  const RenderPassScope pass(RenderType::solid());
  EXPECT_FLOAT_EQ(client::render::core::alphaTestRef(), -1.0f);
 }
 {
  const RenderPassScope pass(RenderType::cutout());
  EXPECT_FLOAT_EQ(client::render::core::alphaTestRef(), 0.5f);
 }
 {
  const RenderPassScope pass(RenderType::translucent());
  EXPECT_FLOAT_EQ(client::render::core::alphaTestRef(), 0.0001f);
 }
 {
  const RenderPassScope pass(RenderType::entityTranslucent());
  EXPECT_FLOAT_EQ(client::render::core::alphaTestRef(), 0.1f);
 }
 EXPECT_FLOAT_EQ(client::render::core::alphaTestRef(), -1.0f);
}
TEST_F(ShaderGlIntegrationTest, EntityBatchDoesNotInheritPreviousPrimitiveMode) {
 client::gl::ShaderProgram program;
 const std::string vertex =
     "layout(location=0) in vec3 vaPosition;void main(){gl_Position=vec4(vaPosition,1.0);}";
 const std::string fragment =
     "layout(location=0) out vec4 color;void main(){color=vec4(1.0,0.25,0.125,1.0);}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 GLCore::bindFramebuffer(client::gl::framebuffer::Framebuffer, 0);
 ::glDrawBuffer(0x0405);
 ::glReadBuffer(0x0405);
 client::render::core::viewport(0, 0, 64, 64);
 client::render::core::disableDepthTest();
 client::render::core::disableCull();
 client::render::core::disableBlend();
 client::render::core::setActiveProgram(&program);
 const auto identity = net::minecraft::util::math::Matrix4f::identityMatrix();
 const float camera[3] = {0.0f, 0.0f, 0.0f};
 client::render::core::setDrawCameraState(identity.m, identity.m, identity.m, identity.m, camera);
 client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
 tessellator.start(0x0000);
 tessellator.vertex(-0.75, -0.75, 0.0);
 tessellator.draw();
 ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
 ::glClear(client::gl::attrib::ColorBufferBit);
 {
  const client::render::Tessellator::ScopedBatch batch;
  tessellator.startQuads();
  tessellator.normal(0.0f, 0.0f, 1.0f);
  tessellator.vertex(-1.0, -1.0, 0.0);
  tessellator.vertex(1.0, -1.0, 0.0);
  tessellator.vertex(1.0, 1.0, 0.0);
  tessellator.vertex(-1.0, 1.0, 0.0);
  tessellator.draw();
 }
 std::array<unsigned char, 4> pixel{};
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba,
                client::gl::pixel::UnsignedByte, pixel.data());
 EXPECT_GT(pixel[0], 250);
 EXPECT_GT(pixel[1], 55);
 EXPECT_GT(pixel[2], 25);
 EXPECT_GT(pixel[3], 250);
 client::render::core::setActiveProgram(nullptr);
 client::render::core::clearDrawCameraState();
 EXPECT_EQ(::glGetError(), 0u);
}
TEST_F(ShaderGlIntegrationTest, CachedMeshColorOverrideReplacesStoredVertexColor) {
 client::gl::ShaderProgram program;
 const std::string vertex =
     "layout(location=0)in vec3 vaPosition;layout(location=2)in vec4 vaColor;"
     "out vec4 color;void main(){gl_Position=vec4(vaPosition,1.0);color=vaColor;}";
 const std::string fragment =
     "in vec4 color;layout(location=0)out vec4 outColor;void main(){outColor=color;}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 client::render::Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 tessellator.start(0x0004);
 tessellator.color(1.0f, 0.0f, 0.0f, 1.0f);
 tessellator.vertex(-1.0, -1.0, 0.0);
 tessellator.vertex(1.0, -1.0, 0.0);
 tessellator.vertex(0.0, 1.0, 0.0);
 client::render::TessellatorMesh mesh = tessellator.takeMesh();
 ASSERT_TRUE(mesh.uploadToGpu());
 client::render::core::setActiveProgram(&program);
 client::render::core::viewport(0, 0, 64, 64);
 ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
 ::glClear(client::gl::attrib::ColorBufferBit);
 client::render::Tessellator::drawMesh(mesh, 0xFF00FF00U);
 std::array<unsigned char, 4> pixel{};
 ::glReadPixels(32, 32, 1, 1, client::gl::pixel::Rgba,
                client::gl::pixel::UnsignedByte, pixel.data());
 EXPECT_LT(pixel[0], 5);
 EXPECT_GT(pixel[1], 250);
 EXPECT_LT(pixel[2], 5);
 EXPECT_GT(pixel[3], 250);
 client::render::core::setActiveProgram(nullptr);
}
TEST_F(ShaderGlIntegrationTest, DisabledBufferBlendReplacesDestination) {
 if(!GLCore::perBufferBlendingSupported || GLCore::blendFunci == nullptr) {
  GTEST_SKIP();
 }
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 targets.clearColors({true}, {{{0.0f, 0.0f, 1.0f, 1.0f}}});
 targets.bindGbuffers();
 client::gl::ShaderProgram program;
 const std::string vertex =
     "void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));"
     "gl_Position=vec4(p*2.0-1.0,0.0,1.0);}";
 const std::string fragment =
     "layout(location=0) out vec4 color;void main(){color=vec4(1.0,0.0,0.0,1.0);}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 program.bind();
 client::render::core::disableDepthTest();
 client::render::core::disableCull();
 client::render::core::enableBlend();
 client::render::core::blendAlpha();
 client::render::core::lockBufferBlend(0, nullptr);
 client::render::core::drawFullscreen();
 std::array<unsigned char, 4> pixel{};
 ::glReadBuffer(0x8CE0);
 ::glReadPixels(2, 2, 1, 1, client::gl::pixel::Rgba, client::gl::pixel::UnsignedByte, pixel.data());
 EXPECT_GT(pixel[0], 250);
 EXPECT_LT(pixel[1], 5);
 EXPECT_LT(pixel[2], 5);
 EXPECT_GT(pixel[3], 250);
 client::gl::ShaderProgram::unbind();
 targets.endGbuffers();
 targets.destroy();
 client::render::core::disableBlend();
 EXPECT_EQ(::glGetError(), 0u);
}
TEST_F(ShaderGlIntegrationTest, ComputeImageBindingsUseCurrentReadSide) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 std::unordered_map<std::string, int> images;
 targets.fillImageBindings(images);
 ASSERT_TRUE(images.contains("colortex0"));
 EXPECT_EQ(images.at("colortex0"), static_cast<int>(targets.readTexture(0)));
 EXPECT_NE(images.at("colortex0"), static_cast<int>(targets.writeTexture(0)));
 const unsigned int first = targets.readTexture(0);
 targets.flip("colortex0");
 images.clear();
 targets.fillImageBindings(images);
 EXPECT_EQ(images.at("colortex0"), static_cast<int>(targets.readTexture(0)));
 EXPECT_NE(images.at("colortex0"), static_cast<int>(targets.writeTexture(0)));
 EXPECT_NE(targets.readTexture(0), first);
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, GbufferRebindUsesPostPrepareReadSides) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4,
                            {client::render::ColorFormat::Rgba8,
                             client::render::ColorFormat::Rgba8},
                            2));
 targets.bindGbuffers();
 const float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
 GLCore::clearBufferfv(client::gl::framebuffer::Color, 1, red);
 const unsigned int before = targets.readTexture(1);
 targets.flip("colortex1");
 EXPECT_NE(targets.readTexture(1), before);
 targets.bindGbuffers();
 const float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};
 GLCore::clearBufferfv(client::gl::framebuffer::Color, 1, green);
 const auto readPixel = [](unsigned int texture) {
  std::array<unsigned char, 4 * 4 * 4> pixels{};
  client::render::core::bindTexture(static_cast<int>(texture));
  ::glGetTexImage(client::gl::cap::Texture2D, 0, client::gl::pixel::Rgba,
                  client::gl::pixel::UnsignedByte, pixels.data());
  return std::array<unsigned char, 4>{pixels[0], pixels[1], pixels[2], pixels[3]};
 };
 const auto current = readPixel(targets.readTexture(1));
 const auto previous = readPixel(before);
 EXPECT_LT(current[0], 5);
 EXPECT_GT(current[1], 250);
 EXPECT_GT(previous[0], 250);
 EXPECT_LT(previous[1], 5);
 targets.endGbuffers();
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, ResetFlipsCopiesFinalReadSideToMain) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba16I}, 1));
 const unsigned int mainTexture = targets.readTexture(0);
 ASSERT_TRUE(targets.bindWrite({"colortex0"}));
 const int value[4] = {1234, -2345, 3456, -4567};
 GLCore::clearBufferiv(client::gl::framebuffer::Color, 0, value);
 targets.flip("colortex0");
 ASSERT_NE(targets.readTexture(0), mainTexture);
 targets.resetFlips();
 ASSERT_EQ(targets.readTexture(0), mainTexture);
 std::array<short, 4 * 4 * 4> pixels{};
 client::render::core::bindTexture(static_cast<int>(mainTexture));
 ::glGetTexImage(client::gl::cap::Texture2D, 0, client::gl::pixel::RgbaInteger,
                 0x1402, pixels.data());
 EXPECT_EQ(pixels[0], value[0]);
 EXPECT_EQ(pixels[1], value[1]);
 EXPECT_EQ(pixels[2], value[2]);
 EXPECT_EQ(pixels[3], value[3]);
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, ComputeWritesRenderTargetInPlaceWithoutPingPongCopy) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(4, 4, {client::render::ColorFormat::Rgba8}, 1));
 targets.clearColors({true}, {{{0.0f, 0.0f, 0.0f, 1.0f}}});
 client::gl::ShaderProgram program;
 const std::string compute =
     "layout(local_size_x=1,local_size_y=1)in;"
     "layout(rgba8)writeonly uniform image2D colorimg0;"
     "void main(){imageStore(colorimg0,ivec2(gl_GlobalInvocationID.xy),vec4(0.25,0.5,0.75,1.0));}";
 ASSERT_TRUE(program.compileCompute(compute, "#version 430 core\n")) << program.lastError();
 program.bind();
 std::unordered_map<std::string, int> images;
 targets.fillImageBindings(images);
 client::render::PackDefinition definition;
 ASSERT_EQ(client::render::bindColorImages(program, images, definition, &targets), 1u);
 GLCore::dispatchCompute(4, 4, 1);
 GLCore::memoryBarrier(0x2028u);
 const auto readPixel = [](unsigned int texture) {
  std::array<unsigned char, 4 * 4 * 4> pixels{};
  client::render::core::bindTexture(static_cast<int>(texture));
  ::glGetTexImage(client::gl::cap::Texture2D, 0, client::gl::pixel::Rgba,
                  client::gl::pixel::UnsignedByte, pixels.data());
  return std::array<unsigned char, 4>{pixels[0], pixels[1], pixels[2], pixels[3]};
 };
 const std::array<unsigned char, 4> current = readPixel(targets.readTexture(0));
 const std::array<unsigned char, 4> alternate = readPixel(targets.writeTexture(0));
 EXPECT_NEAR(current[0], 64, 1);
 EXPECT_NEAR(current[1], 128, 1);
 EXPECT_NEAR(current[2], 191, 1);
 EXPECT_EQ(current[3], 255);
 EXPECT_EQ(alternate[0], 0);
 EXPECT_EQ(alternate[1], 0);
 EXPECT_EQ(alternate[2], 0);
 EXPECT_EQ(alternate[3], 255);
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, NestedPassRestoresWorldProgramIdentityAndStage) {
 client::gl::ShaderProgram parent;
 client::render::core::setActiveProgram(&parent, client::render::WorldProgramId::Entities);
 client::render::core::setRenderStage(client::render::core::RenderStage::Entities);
 {
  const client::render::RenderPassScope child(client::render::RenderType::basic());
  client::render::core::setActiveProgram(nullptr, client::render::WorldProgramId::TerrainTranslucent);
  client::render::core::setRenderStage(client::render::core::RenderStage::TerrainTranslucent);
 }
 EXPECT_EQ(client::render::core::program(), &parent);
 ASSERT_TRUE(client::render::core::activeWorldProgram().has_value());
 EXPECT_EQ(*client::render::core::activeWorldProgram(), client::render::WorldProgramId::Entities);
 EXPECT_EQ(client::render::core::renderStage(), client::render::core::RenderStage::Entities);
 client::render::core::setActiveProgram(nullptr);
 client::render::core::setRenderStage(client::render::core::RenderStage::None);
}
TEST_F(ShaderGlIntegrationTest, EntityAndRenderedItemScopesRestoreNoObjectSentinel) {
 client::render::core::setEntityId(-1);
 client::render::core::setRenderedItemId(-1);
 {
  const client::render::core::EntityIdScope entity(43);
  EXPECT_EQ(client::render::core::entityId(), 43);
  {
   const client::render::core::RenderedItemScope item(91);
   EXPECT_EQ(client::render::core::renderedItemId(), 91);
  }
  EXPECT_EQ(client::render::core::renderedItemId(), -1);
 }
 EXPECT_EQ(client::render::core::entityId(), -1);
 EXPECT_EQ(client::render::core::renderedItemId(), -1);
}
TEST_F(ShaderGlIntegrationTest, GeometryBindsEveryAlbedoSamplerAliasToUnitZero) {
 client::gl::ShaderProgram program;
 const std::string vertex =
     "layout(location=0) in vec3 Position;void main(){gl_Position=vec4(Position,1.0);}";
 const std::string fragment =
     "uniform sampler2D tex;uniform sampler2D texture;uniform sampler2D gtexture;"
     "uniform sampler2D flw_diffuseTex;uniform sampler2D u_MainSampler;"
     "layout(location=0) out vec4 color;void main(){ivec2 p=ivec2(0);color="
     "texelFetch(tex,p,0)+texelFetch(texture,p,0)+texelFetch(gtexture,p,0)+"
     "texelFetch(flw_diffuseTex,p,0)+texelFetch(u_MainSampler,p,0);}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 program.bind();
 for(const char* name : {"tex", "texture", "gtexture", "flw_diffuseTex", "u_MainSampler"}) {
  program.set1i(name, 7);
 }
 client::render::core::setActiveProgram(&program);
 std::array<client::render::TessellatorVertex, 3> vertices{};
 vertices[0].x = -1.0f;
 vertices[0].y = -1.0f;
 vertices[1].x = 1.0f;
 vertices[1].y = -1.0f;
 vertices[2].y = 1.0f;
 client::render::core::RenderPass pass;
 pass.vertexData = vertices.data();
 pass.vertexCount = 3;
 pass.stride = static_cast<int>(sizeof(client::render::TessellatorVertex));
 pass.glMode = 0x0004;
 client::render::core::submit(pass);
 const auto getUniform = reinterpret_cast<PFNGLGETUNIFORMIVPROC>(glfwGetProcAddress("glGetUniformiv"));
 ASSERT_NE(getUniform, nullptr);
 for(const char* name : {"tex", "texture", "gtexture", "flw_diffuseTex", "u_MainSampler"}) {
  int value = -1;
  getUniform(program.handle(), program.location(name), &value);
  EXPECT_EQ(value, 0) << name;
 }
 client::render::core::setActiveProgram(nullptr);
}
TEST_F(ShaderGlIntegrationTest, EntityTextureBindingOwnsUnitZero) {
 client::texture::TextureManager textureManager;
 client::texture::RasterImage image;
 image.width = 1;
 image.height = 1;
 image.argb = {0xFFFFFFFFu};
 client::render::core::activeTexture(client::gl::tex::Texture0);
 const int entityTexture = textureManager.getTextureId("entity_binding_test", image);
 const unsigned int sentinel = client::render::core::genTexture();
 client::render::core::activeTexture(client::gl::tex::Texture0 + 7);
 client::render::core::bindTexture(static_cast<int>(sentinel));
 client::render::entity::EntityRenderDispatcher dispatcher;
 dispatcher.init(nullptr, &textureManager, nullptr, nullptr, nullptr, 0.0f);
 TextureBindingEntityRenderer renderer;
 renderer.setDispatcher(&dispatcher);
 renderer.bindTexture("entity_binding_test");
 EXPECT_EQ(client::render::core::boundTexture(), entityTexture);
 client::render::core::activeTexture(client::gl::tex::Texture0 + 7);
 EXPECT_EQ(client::render::core::boundTexture(), static_cast<int>(sentinel));
 client::render::core::activeTexture(client::gl::tex::Texture0);
 textureManager.deleteTexture(entityTexture);
 client::render::core::deleteTexture(sentinel);
}
TEST_F(ShaderGlIntegrationTest, ReplicatedDynamicAtlasTilesUpdateEveryMipmapLevel) {
 struct MipmapModeScope {
  bool mipmap = client::texture::TextureManager::MIPMAP;
  bool linear = client::texture::TextureManager::MIPMAP_LINEAR;
  ~MipmapModeScope() {
   client::texture::TextureManager::MIPMAP = mipmap;
   client::texture::TextureManager::MIPMAP_LINEAR = linear;
  }
 } mode;
 client::texture::TextureManager::MIPMAP = true;
 client::texture::TextureManager::MIPMAP_LINEAR = false;
 client::texture::TextureManager textureManager;
 client::texture::RasterImage atlas;
 atlas.width = 256;
 atlas.height = 256;
 atlas.argb.assign(256 * 256, 0xFF000000u);
 client::render::core::activeTexture(client::gl::tex::Texture0);
 const int texture = textureManager.getTextureId("replicated_dynamic_mip_atlas", atlas);
 ASSERT_GT(texture, 0);
 ReplicatedDynamicTexture dynamic(texture);
 textureManager.addDynamicTexture(&dynamic);
 textureManager.tick();
 client::render::core::bindTexture(texture);
 for(int level = 0; level <= 4; ++level) {
  const int dimension = 256 >> level;
  const int tileSize = 16 >> level;
  std::vector<unsigned char> pixels(static_cast<std::size_t>(dimension * dimension * 4));
  ::glGetTexImage(client::gl::cap::Texture2D,
                  level,
                  client::gl::pixel::Rgba,
                  client::gl::pixel::UnsignedByte,
                  pixels.data());
  for(int replicateY = 0; replicateY < 2; ++replicateY) {
   for(int replicateX = 0; replicateX < 2; ++replicateX) {
    const int x = (2 + replicateX) * tileSize + tileSize / 2;
    const int y = (2 + replicateY) * tileSize + tileSize / 2;
    const std::size_t offset = static_cast<std::size_t>((x + y * dimension) * 4);
    EXPECT_EQ(pixels[offset], 17) << level << ':' << replicateX << ':' << replicateY;
    EXPECT_EQ(pixels[offset + 1], 83) << level << ':' << replicateX << ':' << replicateY;
    EXPECT_EQ(pixels[offset + 2], 149) << level << ':' << replicateX << ':' << replicateY;
    EXPECT_EQ(pixels[offset + 3], 255) << level << ':' << replicateX << ':' << replicateY;
   }
  }
 }
 textureManager.deleteTexture(texture);
}
TEST_F(ShaderGlIntegrationTest, WorldProgramsBindSceneAndOpaqueDepthSamplers) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(8, 8, std::vector<client::render::ColorFormat>(5, client::render::ColorFormat::Rgba8), 5));
 client::gl::ShaderProgram program;
 const std::string vertex = "void main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}";
 const std::string fragment =
     "uniform sampler2D colortex4;uniform sampler2D gaux1;uniform sampler2D depthtex0;"
     "uniform sampler2D gdepthtex;uniform sampler2D depthtex1;uniform sampler2D depthtex2;"
     "layout(location=0) out vec4 color;void main(){vec2 p=vec2(0.5);color=texture(colortex4,p)+"
     "texture(gaux1,p)+texture(depthtex0,p)+texture(gdepthtex,p)+texture(depthtex1,p)+texture(depthtex2,p);}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n"));
 program.bind();
 client::render::WorldProgramBindContext context;
 context.sceneTargets = &targets;
 context.sceneDepthTexture = static_cast<int>(targets.depthTexture());
 context.opaqueDepthTexture = static_cast<int>(targets.depthTexture());
 context.handDepthTexture = static_cast<int>(targets.depthTexture());
 client::render::bindWorldProgram(program, context);
 const auto getUniform = reinterpret_cast<PFNGLGETUNIFORMIVPROC>(glfwGetProcAddress("glGetUniformiv"));
 ASSERT_NE(getUniform, nullptr);
 const auto readSampler = [&](const char* name) {
  int value = -1;
  getUniform(program.handle(), program.location(name), &value);
  return value;
 };
 EXPECT_EQ(readSampler("colortex4"), readSampler("gaux1"));
 EXPECT_EQ(readSampler("depthtex0"), readSampler("gdepthtex"));
 EXPECT_NE(readSampler("colortex4"), readSampler("depthtex0"));
 EXPECT_NE(readSampler("depthtex0"), readSampler("depthtex1"));
 EXPECT_NE(readSampler("depthtex1"), readSampler("depthtex2"));
 targets.destroy();
}
TEST_F(ShaderGlIntegrationTest, PackDepthSnapshotsKeepOpaqueAndHandSemantics) {
 client::render::PackInstance pack;
 const int fallback = static_cast<int>(client::render::core::genTexture());
 pack.depthTextures[0] = client::gl::GlTexture(client::render::core::genTexture());
 pack.depthTextures[1] = client::gl::GlTexture(client::render::core::genTexture());
 EXPECT_EQ(pack.opaqueDepthTexture(fallback), static_cast<int>(pack.depthTextures[1].handle()));
 EXPECT_EQ(pack.handDepthTexture(fallback), static_cast<int>(pack.depthTextures[0].handle()));
 pack.depthTextures[0].reset();
 EXPECT_EQ(pack.handDepthTexture(fallback), static_cast<int>(pack.depthTextures[1].handle()));
 pack.depthTextures[1].reset();
 EXPECT_EQ(pack.opaqueDepthTexture(fallback), fallback);
 EXPECT_EQ(pack.handDepthTexture(fallback), fallback);
 client::render::core::deleteTexture(static_cast<unsigned int>(fallback));
}
TEST_F(ShaderGlIntegrationTest, PackSamplerOverridesWinWithoutDuplicateBuiltinBindings) {
 client::render::ColorTargets targets;
 ASSERT_TRUE(targets.ensure(8, 8, std::vector<client::render::ColorFormat>(10, client::render::ColorFormat::Rgba8), 10));
 client::gl::ShaderProgram program;
 const std::string vertex = "void main(){gl_Position=vec4(0.0,0.0,0.0,1.0);}";
 const std::string fragment =
     "uniform sampler2D colortex9;uniform sampler2D shadowcolor3;"
     "layout(location=0) out vec4 color;void main(){ivec2 p=ivec2(0);"
     "color=texelFetch(colortex9,p,0)+texelFetch(shadowcolor3,p,0);}";
 ASSERT_TRUE(program.compile(vertex, fragment, "#version 430 core\n")) << program.lastError();
 const unsigned int customColortex = client::render::core::genTexture();
 const unsigned int customShadow = client::render::core::genTexture();
 client::render::PackInstance pack;
 pack.worldTextures["colortex9"] = static_cast<int>(customColortex);
 pack.worldTextures["shadowcolor3"] = static_cast<int>(customShadow);
 const int shadowColors[4] = {static_cast<int>(targets.readTexture(0)),
                              static_cast<int>(targets.readTexture(1)),
                              static_cast<int>(targets.readTexture(2)),
                              static_cast<int>(targets.readTexture(3))};
 program.bind();
 client::render::WorldProgramBindContext context;
 context.pack = &pack;
 context.sceneTargets = &targets;
 context.shadowColorTextures = shadowColors;
 context.shadowColorTextureCount = 4;
 client::render::bindWorldProgram(program, context);
 const auto getUniform = reinterpret_cast<PFNGLGETUNIFORMIVPROC>(glfwGetProcAddress("glGetUniformiv"));
 ASSERT_NE(getUniform, nullptr);
 const auto samplerTexture = [&](const char* name) {
  int unit = -1;
  getUniform(program.handle(), program.location(name), &unit);
  client::render::core::activeTexture(client::gl::tex::Texture0 + unit);
  return client::render::core::boundTexture();
 };
 EXPECT_EQ(samplerTexture("colortex9"), static_cast<int>(customColortex));
 EXPECT_EQ(samplerTexture("shadowcolor3"), static_cast<int>(customShadow));
 client::render::core::activeTexture(client::gl::tex::Texture0);
 client::render::core::deleteTexture(customColortex);
 client::render::core::deleteTexture(customShadow);
 targets.destroy();
}
} // namespace net::minecraft::test
