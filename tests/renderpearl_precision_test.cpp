#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"

namespace net::minecraft::test {
namespace {
using client::gl::GLCore;
constexpr int kSamples = 512;

std::string readPearlShader(const std::string& path) {
 const auto file = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" /
                   "RenderPearl v2.8.0-beta.4" / "shaders" / path;
 std::ifstream input(file, std::ios::binary);
 if(!input) throw std::runtime_error("Cannot read " + file.string());
 return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string precisionPreamble(bool half) {
 std::string text = "#version 430 core\n#define immut\n#define uint16_t uint\n";
 if(half) {
  text += glfwExtensionSupported("GL_NV_gpu_shader5")
              ? "#extension GL_NV_gpu_shader5 : require\n"
              : "#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require\n";
 } else {
  text += "#define float16_t float\n#define f16vec2 vec2\n#define f16vec3 vec3\n#define f16vec4 vec4\n";
 }
 return text;
}

std::vector<float> computeSamples(const std::string& source, bool half, int side,
                                 const std::function<void(client::gl::ShaderProgram&)>& setup = {},
                                 bool translate = false, double* gpuMillis = nullptr) {
 client::gl::ShaderProgram program;
 const std::string preamble = precisionPreamble(half);
 const std::string compiledSource = translate
     ? client::render::prepareSource("deferred", client::render::ShaderStage::Compute,
                                      client::render::PackDefinition{}, preamble + source)
     : source;
 EXPECT_TRUE(program.compileCompute(compiledSource, preamble)) << program.lastError();
 if(!program.valid()) return {};
 unsigned int handle = 0;
 GLCore::genBuffers(1, &handle);
 client::gl::GlBuffer buffer(handle);
 std::vector<float> values(static_cast<std::size_t>(side * side), 0.0f);
 GLCore::bindBuffer(0x90D2, buffer.handle());
 GLCore::bufferData(0x90D2, static_cast<intptr_t>(values.size() * sizeof(float)), values.data(), 0x88E9);
 GLCore::bindBufferBase(0x90D2, 0, buffer.handle());
 program.bind();
 if(setup) setup(program);
 GLCore::dispatchCompute(static_cast<unsigned int>(side / 16), static_cast<unsigned int>(side / 16), 1);
 GLCore::memoryBarrier(0x2200);
 if(gpuMillis != nullptr) {
  std::array<unsigned int, 24> queries{};
  std::array<std::uint64_t, 24> durations{};
  GLCore::genQueries(static_cast<int>(queries.size()), queries.data());
  for(unsigned int query : queries) {
   GLCore::beginQuery(0x88BF, query);
   GLCore::dispatchCompute(static_cast<unsigned int>(side / 16), static_cast<unsigned int>(side / 16), 1);
   GLCore::memoryBarrier(0x2200);
   GLCore::endQuery(0x88BF);
  }
  for(std::size_t i = 0; i < queries.size(); ++i) {
   GLCore::getQueryObjectui64v(queries[i], 0x8866, &durations[i]);
  }
  GLCore::deleteQueries(static_cast<int>(queries.size()), queries.data());
  std::sort(durations.begin(), durations.end());
  *gpuMillis = static_cast<double>(durations[durations.size() / 2]) / 1000000.0;
 }
 const void* mapped = GLCore::mapBufferRange(0x90D2, 0,
                                            static_cast<intptr_t>(values.size() * sizeof(float)), 0x0001);
 EXPECT_NE(mapped, nullptr);
 if(mapped != nullptr) {
  std::memcpy(values.data(), mapped, values.size() * sizeof(float));
  GLCore::unmapBuffer(0x90D2);
 }
 GLCore::bindBufferBase(0x90D2, 0, 0);
 GLCore::bindBuffer(0x90D2, 0);
 client::gl::ShaderProgram::unbind();
 EXPECT_EQ(::glGetError(), 0u);
 return values;
}

class RenderPearlPrecision : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window_ = glfwCreateWindow(64, 64, "renderpearl-precision", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  GLCore::ensureLoaded();
 }
 static void TearDownTestSuite() {
  client::render::core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) glfwDestroyWindow(window_);
  window_ = nullptr;
 }
 inline static GLFWwindow* window_ = nullptr;
};
}

TEST_F(RenderPearlPrecision, RuntimeTranslationKeepsHorizonStarsCompact) {
 const std::string deferred = readPearlShader("world_default/deferred.csh");
 const auto sky = deferred.find("} else { // Render sky.");
 ASSERT_NE(sky, std::string::npos);
 const auto start = deferred.find("immut uvec", sky);
 const auto end = deferred.find("color = stars + fog_color;", start);
 ASSERT_NE(start, std::string::npos);
 ASSERT_NE(end, std::string::npos);
 const std::string source = readPearlShader("lib/prng/pcg.glsl") + R"(
layout(local_size_x=16,local_size_y=16) in;
layout(std430,binding=0) buffer Results { float values[]; };
void main() {
 ivec2 pixel=ivec2(gl_GlobalInvocationID.xy);
 vec2 uv=(vec2(pixel)+0.5)/512.0;
 float azimuth=(uv.x-0.5)*0.5;
 float altitude=uv.y*0.12;
 vec3 pe=vec3(cos(altitude)*sin(azimuth),sin(altitude),-cos(altitude)*cos(azimuth));
 f16vec3 n_pe=f16vec3(normalize(pe));
 float16_t sky_fog_val=float16_t(0.0);
 vec3 skyState=vec3(0.0);
 float frameTimeCounter=0.0;
)" + deferred.substr(start, end - start) + R"(
 values[pixel.y*512+pixel.x]=float(stars);
}
)";
 for(bool translated : {false, true}) {
  SCOPED_TRACE(translated);
  double gpuMillis = 0.0;
  const auto values = computeSamples(source, false, kSamples, {}, translated, &gpuMillis);
  ASSERT_EQ(values.size(), static_cast<std::size_t>(kSamples * kSamples));
  int longest = 0;
  int lit = 0;
  std::array<int, kSamples> run{};
  for(int y = 0; y < kSamples; ++y) {
   for(int x = 0; x < kSamples; ++x) {
    const bool star = values[static_cast<std::size_t>(y * kSamples + x)] > 0.01f;
    run[x] = star ? run[x] + 1 : 0;
    longest = std::max(longest, run[x]);
    lit += star;
   }
  }
  RecordProperty(translated ? "translated_gpu_ms" : "stock_gpu_ms", std::to_string(gpuMillis));
  RecordProperty(translated ? "translated_longest_star" : "stock_longest_star", longest);
  EXPECT_GT(lit, 20);
  if(translated) EXPECT_LE(longest, 8);
  else EXPECT_GT(longest, 8);
 }
 EXPECT_EQ(readPearlShader("world_default/deferred.csh"), deferred);
}

TEST_F(RenderPearlPrecision, RuntimeTranslationRemovesFalseHandShadowsAndPreservesBlockers) {
 if(!glfwExtensionSupported("GL_NV_gpu_shader5") &&
    !glfwExtensionSupported("GL_EXT_shader_explicit_arithmetic_types_float16")) {
  GTEST_SKIP() << "Native half precision is unavailable";
 }
 client::render::PackInstance pack;
 pack.path = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "RenderPearl v2.8.0-beta.4";
 pack.directory = true;
 const std::string hand = client::render::PackCompiler::resolveIncludes(pack, "shaders/lib/light/hand.glsl");
 ASSERT_FALSE(hand.empty());
 const std::string source = R"(
#define HAND_LIGHT 1
#define HAND_LIGHT_TRACE_STEPS 16
#define HAND_LIGHT_TRACE_HARDNESS 1
#define MAX_HAND_LIGHT_TRACE_DIST 48
#define MV_INV mat3(1.0)
const float lumi_dir_bl=1.0;
const float16_t min_n_dot_l=float16_t(0.0);
f16vec3 brdf(float16_t ndotl,f16vec3 normal,f16vec3 eye,f16vec3 light,
             float16_t roughness,float16_t f0,bool metal,f16vec3 color,f16vec3 inverse_color) {
 return f16vec3(1.0);
}
)" + readPearlShader("lib/mmul.glsl") + hand + R"(
layout(local_size_x=16,local_size_y=16) in;
layout(std430,binding=0) buffer Results { float values[]; };
void main() {
 ivec2 pixel=ivec2(gl_GlobalInvocationID.xy);
 vec2 xy=((vec2(pixel)+0.5)/128.0*2.0-1.0)*0.8;
 float distance=8.0/(1.0+0.4*xy.x+0.3*xy.y);
 vec3 view=vec3(xy,-1.0)*distance;
 vec3 origin=vec3(0.0,0.0,-0.1);
 f16vec3 normal=f16vec3(normalize(vec3(-0.4,-0.3,1.0)));
 f16vec3 light=get_hand_light(uint16_t(1u),0xffffffffu,origin,view,view,
     f16vec3(normalize(view)),float16_t(1.0),float16_t(0.04),false,
     normal,normal,f16vec3(1.0),f16vec3(1.0),float16_t(0.0),false);
 values[pixel.y*128+pixel.x]=float(light.x)*dot(origin-view,origin-view);
}
)";
 constexpr int depthSize = 1024;
 constexpr float nearPlane = 0.05f;
 constexpr float farPlane = 1650.0f;
 const float a = -(farPlane + nearPlane) / (farPlane - nearPlane);
 const float b = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);
 const std::array<float, 16> projection = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, a, -1, 0, 0, b, 0};
 std::vector<float> depth(static_cast<std::size_t>(depthSize * depthSize));
 client::gl::GlTexture texture(client::render::core::genTexture());
 client::render::core::activeTexture(0x84C0);
 client::render::core::bindTexture(static_cast<int>(texture.handle()));
 ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
 ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
 ::glTexParameteri(0x0DE1, 0x2802, 0x812F);
 ::glTexParameteri(0x0DE1, 0x2803, 0x812F);
 for(int hardness : {0, 1}) {
 std::string modeSource = source;
 if(hardness == 0) {
  const std::string setting = "#define HAND_LIGHT_TRACE_HARDNESS 1";
  modeSource.replace(modeSource.find(setting), setting.size(), "#define HAND_LIGHT_TRACE_HARDNESS 0");
 }
 SCOPED_TRACE(hardness);
 for(bool blocked : {false, true}) {
  for(int y = 0; y < depthSize; ++y) {
   for(int x = 0; x < depthSize; ++x) {
    const float dx = (static_cast<float>(x) + 0.5f) / depthSize * 2.0f - 1.0f;
    const float dy = (static_cast<float>(y) + 0.5f) / depthSize * 2.0f - 1.0f;
    const float distance = blocked ? 1.0f : 8.0f / (1.0f + 0.4f * dx + 0.3f * dy);
    depth[static_cast<std::size_t>(y * depthSize + x)] = 0.5f + 0.5f * (-a + b / distance);
   }
  }
  ::glTexImage2D(0x0DE1, 0, 0x81A6, depthSize, depthSize, 0, 0x1902, 0x1406, depth.data());
  for(bool half : {false, true}) {
   for(bool translated : {false, true}) {
    SCOPED_TRACE(::testing::Message() << "half=" << half << " translated=" << translated << " blocked=" << blocked);
    double gpuMillis = 0.0;
    const auto values = computeSamples(modeSource, half, 128, [&](client::gl::ShaderProgram& program) {
     program.set1i("depthtex2", 0);
     program.setMatrix4("gbufferProjection", projection.data());
    }, translated, half && !blocked && hardness == 1 ? &gpuMillis : nullptr);
    ASSERT_EQ(values.size(), 128u * 128u);
    int falseShadows = 0;
    for(float value : values) {
     ASSERT_TRUE(std::isfinite(value));
     if(blocked) {
      ASSERT_LT(value, 0.15f);
     } else {
      falseShadows += std::abs(value - 1.0f) > 0.02f;
     }
    }
    if(!blocked) {
     if(half && hardness == 1) {
      RecordProperty(translated ? "translated_gpu_ms" : "stock_gpu_ms", std::to_string(gpuMillis));
      RecordProperty(translated ? "translated_false_shadows" : "stock_false_shadows", falseShadows);
     }
     if(half && !translated) EXPECT_GT(falseShadows, 50);
     else EXPECT_EQ(falseShadows, 0);
    }
   }
  }
 }
 }
}

TEST(RenderPearlSourceRepair, UnrelatedAndCommentedShaderMathIsPreserved) {
 const std::string source = R"(#version 430 core
/*
uvec2 seed = uvec2(ivec2(n_pe.xz * 1000.0 + sin(frameTimeCounter * 1000.0) * 0.2));
float hash = float(pcg(seed.x + pcg(seed.y))) / float(0xFFFFFFFFu);
*/
void main() {
 vec3 pe = vec3(1.0);
 float sky_fog_val = 0.0;
 vec2 sample_pos = pe.xz * 1000.0;
}
)";
 const client::render::PackDefinition pack;
 EXPECT_EQ(client::render::prepareSource("deferred", client::render::ShaderStage::Compute, pack, source),
           client::render::normalizePackSource(pack, source));
}

TEST(RenderPearlSourceRepair, IncompleteTraceFingerprintIsNotPartiallyRewritten) {
 std::string source = precisionPreamble(false) +
     "#define HAND_LIGHT_TRACE_STEPS 16\n#define HAND_LIGHT_TRACE_HARDNESS 1\n" +
     readPearlShader("lib/light/hand.glsl");
 const auto divisor = source.find("steps + 1");
 ASSERT_NE(divisor, std::string::npos);
 source.replace(divisor, std::string("steps + 1").size(), "steps + 2");
 const client::render::PackDefinition pack;
 EXPECT_EQ(client::render::prepareSource("deferred", client::render::ShaderStage::Compute, pack, source),
           client::render::normalizePackSource(pack, source));
}

TEST(RenderPearlSourceRepair, DisabledTracingIsPreserved) {
 const std::string source = precisionPreamble(false) +
     "#define HAND_LIGHT_TRACE_STEPS 0\n#define HAND_LIGHT_TRACE_HARDNESS 1\n" +
     readPearlShader("lib/light/hand.glsl");
 const client::render::PackDefinition pack;
 EXPECT_EQ(client::render::prepareSource("deferred", client::render::ShaderStage::Compute, pack, source),
           client::render::normalizePackSource(pack, source));
}
}
