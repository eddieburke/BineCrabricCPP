#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::render {
namespace {
PackUniformValues frameStub() {
 PackUniformValues v{};
 v.frameTime = 0.05f;
 v.sunAngle = 0.25f;
 v.rainStrength = 0.5f;
 v.wetness = 0.5f;
 v.biome = 3; // Forest
 v.biomeCategory = 10;
 v.isSprinting = 1;
 v.cameraPosition[0] = 10.0f;
 v.cameraPosition[1] = 64.0f;
 v.cameraPosition[2] = -3.0f;
 // GLSL indexing: matrix.i.j is m[i][j], column i and row j.
 v.gbufferModelView[3] = 7.5f;  // column 0, row 3
 v.gbufferModelView[12] = 2.5f; // column 3, row 0
 return v;
}
TEST(CustomUniformsTest, EvaluatesIrisDocExamples) {
 std::vector<CustomUniformDecl> decls = {
     {"isOakBiome", CustomUniformType::Bool, true, "in(biome, BIOME_FOREST, BIOME_DARK_FOREST)"},
     {"sunHeight", CustomUniformType::Float, true, "sin(2.0*pi * sunAngle)"},
     {"sprintFactor", CustomUniformType::Float, true, "if(is_sprinting, 1.0, 0.0)"},
     {"camY", CustomUniformType::Float, true, "cameraPosition.y"},
     {"mv03", CustomUniformType::Float, true, "gbufferModelView.0.3"},
     {"mv30", CustomUniformType::Float, true, "gbufferModelView.3.0"},
     {"mv3x", CustomUniformType::Float, true, "gbufferModelView.3.x"},
     {"waterTint", CustomUniformType::Float, false, "if(in(biome, BIOME_SWAMP), 0.38, 0.247)"},
     {"waterColor", CustomUniformType::Vec3, true, "vec3(waterTint, 0.5, 0.9)"},
 };
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 runtime.evaluate(frameStub());
 const auto& values = runtime.values();
 EXPECT_TRUE(values.at("isOakBiome").asBool());
 EXPECT_NEAR(values.at("sunHeight").asFloat(), std::sin(2.0f * 3.14159265f * 0.25f), 1e-4f);
 EXPECT_FLOAT_EQ(values.at("sprintFactor").asFloat(), 1.0f);
 EXPECT_FLOAT_EQ(values.at("camY").asFloat(), 64.0f);
 EXPECT_FLOAT_EQ(values.at("mv03").asFloat(), 7.5f);
 EXPECT_FLOAT_EQ(values.at("mv30").asFloat(), 2.5f);
 EXPECT_FLOAT_EQ(values.at("mv3x").asFloat(), 2.5f);
 EXPECT_NEAR(values.at("waterTint").asFloat(), 0.247f, 1e-5f);
 EXPECT_EQ(values.at("waterColor").type, CustomUniformType::Vec3);
 EXPECT_NEAR(values.at("waterColor").f[0], 0.247f, 1e-5f);
}
TEST(CustomUniformsTest, SmoothAndArithmetic) {
 std::vector<CustomUniformDecl> decls = {
     {"a", CustomUniformType::Float, true, "1.0 + 2.0 * 3.0"},
     {"b", CustomUniformType::Bool, true, "between(5, 1, 9) && !false"},
     {"c", CustomUniformType::Int, true, "floor(3.7)"},
     {"d", CustomUniformType::Float, true, "smooth(1.0, 1.0)"},
 };
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 runtime.evaluate(frameStub());
 EXPECT_FLOAT_EQ(runtime.values().at("a").asFloat(), 7.0f);
 EXPECT_TRUE(runtime.values().at("b").asBool());
 EXPECT_EQ(runtime.values().at("c").asInt(), 3);
 EXPECT_FLOAT_EQ(runtime.values().at("d").asFloat(), 1.0f);
}
TEST(CustomUniformsTest, VariadicMinMaxAcceptUpToSixteenArguments) {
 std::vector<CustomUniformDecl> decls = {
     {"minimum", CustomUniformType::Float, true, "min(9.0, 4.0, 7.0, -2.0, 3.0)"},
     {"maximum", CustomUniformType::Int, true, "max(1, 7, 3, 16, 9, 2)"},
     {"vectorMaximum", CustomUniformType::Vec3, true,
      "max(vec3(1.0, 8.0, 3.0), vec3(4.0, 2.0, 9.0), vec3(0.0, 7.0, 5.0))"},
 };
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 runtime.evaluate(frameStub());
 EXPECT_FLOAT_EQ(runtime.values().at("minimum").asFloat(), -2.0f);
 EXPECT_EQ(runtime.values().at("maximum").asInt(), 16);
 EXPECT_FLOAT_EQ(runtime.values().at("vectorMaximum").f[0], 4.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("vectorMaximum").f[1], 8.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("vectorMaximum").f[2], 9.0f);
}
// RenderPearl transforms directions with expressions like
// `gbufferModelViewInverse.0.0 * d.x + gbufferModelViewInverse.1.0 * d.y`, which
// only parse if `.0.0` lexes as two indices rather than the float ".0.0".
TEST(CustomUniformsTest, ParsesChainedMatrixIndices) {
 PackUniformValues frame{};
 for(int i = 0; i < 16; ++i) frame.gbufferModelViewInverse[i] = static_cast<float>(i);
 std::vector<CustomUniformDecl> decls = {
     {"a", CustomUniformType::Float, true, "gbufferModelViewInverse.2.1"},
     {"b", CustomUniformType::Float, true, "gbufferModelViewInverse.0.y"},
     {"c", CustomUniformType::Vec3, true,
      "vec3(gbufferModelViewInverse.0.0, gbufferModelViewInverse.1.0, gbufferModelViewInverse.2.0)"},
     {"d", CustomUniformType::Float, true, "0.5 * gbufferModelViewInverse.3.3"},
 };
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("a").asFloat(), 9.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("b").asFloat(), 1.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("c").f[0], 0.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("c").f[1], 4.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("c").f[2], 8.0f);
 EXPECT_FLOAT_EQ(runtime.values().at("d").asFloat(), 7.5f);
}
bool load(const std::unordered_map<std::string, std::string>& files, PackDefinition& pack,
          std::unordered_map<std::string, PackSourceOption>& options, std::string& error) {
 std::vector<std::string> resources;
 for(const auto& [path, _] : files) resources.push_back(path);
 return PackLoader::load(
     resources, [&](std::string_view path) { return files.at(std::string(path)); }, pack, options, error);
}
TEST(CustomUniformsTest, ParsesUniformAndVariableDirectives) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 ASSERT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "variable.float.waterColorR = if(in(biome, BIOME_SWAMP), 0.38, 0.247)\n"
                    "uniform.vec3.waterColor = vec3(waterColorR, 0.5, 0.9)\n"
                    "uniform.bool.isHot = in(biome_category, CAT_DESERT, CAT_SAVANNA)\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.customUniforms.size(), 3u);
 EXPECT_FALSE(pack.customUniforms[0].upload);
 EXPECT_EQ(pack.customUniforms[0].name, "waterColorR");
 EXPECT_EQ(pack.customUniforms[0].type, CustomUniformType::Float);
 EXPECT_TRUE(pack.customUniforms[1].upload);
 EXPECT_EQ(pack.customUniforms[1].name, "waterColor");
 EXPECT_EQ(pack.customUniforms[1].type, CustomUniformType::Vec3);
 EXPECT_TRUE(pack.customUniforms[2].upload);
 EXPECT_EQ(pack.customUniforms[2].type, CustomUniformType::Bool);
 CustomUniformRuntime runtime;
 ASSERT_TRUE(runtime.compile(pack.customUniforms, error)) << error;
}
}
} // namespace net::minecraft::client::render
