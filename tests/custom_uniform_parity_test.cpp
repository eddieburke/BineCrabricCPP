#include <gtest/gtest.h>
#include <array>
#include <limits>
#include <string>
#include <utility>
#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"

namespace net::minecraft::client::render {
namespace {
TEST(CustomUniformParity, PackedViewKeepsOddViewportDimensions) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({{"packed", CustomUniformType::Int, true,
                               "65536 * floor(viewHeight) + floor(viewWidth)"}}, error)) << error;
 PackUniformValues frame{};
 for(const auto& size : std::array<std::array<int, 2>, 5>{{{1366, 768}, {1919, 1080}, {1920, 1080},
                                                        {2561, 1441}, {854, 480}}}) {
  frame.viewWidth = static_cast<float>(size[0]);
  frame.viewHeight = static_cast<float>(size[1]);
  runtime.evaluate(frame);
  EXPECT_EQ(runtime.values().at("packed").asInt(), size[1] * 65536 + size[0]);
 }
}

TEST(CustomUniformParity, IntegerLiteralsArithmeticAndComparisonsRemainExact) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"literal", CustomUniformType::Int, true, "16777217"},
     {"sum", CustomUniformType::Float, true, "16777217 + 1"},
     {"wrap", CustomUniformType::Int, true, "2147483647 + 1"},
     {"absolute", CustomUniformType::Int, true, "abs(-2147483647)"},
     {"equal", CustomUniformType::Bool, true, "frameCounter == 16777216"},
     {"greater", CustomUniformType::Bool, true, "frameCounter > 16777216"},
     {"between", CustomUniformType::Bool, true, "between(frameCounter, 16777216, 16777216)"},
     {"fraction", CustomUniformType::Float, true, "1.5 + 2"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameCounter = 16777217;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("literal").asInt(), 16777217);
 EXPECT_FLOAT_EQ(runtime.values().at("sum").asFloat(), 16777218.0f);
 EXPECT_EQ(runtime.values().at("wrap").asInt(), std::numeric_limits<int>::min());
 EXPECT_EQ(runtime.values().at("absolute").asInt(), std::numeric_limits<int>::max());
 EXPECT_FALSE(runtime.values().at("equal").asBool());
 EXPECT_TRUE(runtime.values().at("greater").asBool());
 EXPECT_FALSE(runtime.values().at("between").asBool());
 EXPECT_FLOAT_EQ(runtime.values().at("fraction").asFloat(), 3.5f);
}

TEST(CustomUniformParity, FloatEqualityHasNoImplicitTolerance) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"equal", CustomUniformType::Bool, true, "1.0 == 1.0000005"},
     {"different", CustomUniformType::Bool, true, "1.0 != 1.0000005"},
     {"member", CustomUniformType::Bool, true, "in(1.0, 1.0000005)"},
     {"tolerant", CustomUniformType::Bool, true, "equals(1.0, 1.0000005, 0.000001)"}}, error)) << error;
 runtime.evaluate({});
 EXPECT_FALSE(runtime.values().at("equal").asBool());
 EXPECT_TRUE(runtime.values().at("different").asBool());
 EXPECT_FALSE(runtime.values().at("member").asBool());
 EXPECT_TRUE(runtime.values().at("tolerant").asBool());
}

TEST(CustomUniformParity, ForwardDependenciesUpdateInTheSameFrame) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"answer", CustomUniformType::Float, true, "middle * 2.0"},
     {"middle", CustomUniformType::Float, false, "base + 1.0"},
     {"base", CustomUniformType::Float, false, "rainStrength"}}, error)) << error;
 PackUniformValues frame{};
 frame.rainStrength = 0.5f;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("answer").asFloat(), 3.0f);
 frame.rainStrength = 1.0f;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("answer").asFloat(), 4.0f);
 CustomUniformRuntime moved = std::move(runtime);
 frame.rainStrength = 0.0f;
 moved.evaluate(frame);
 EXPECT_FLOAT_EQ(moved.values().at("answer").asFloat(), 2.0f);
 CustomUniformRuntime assigned;
 ASSERT_TRUE(assigned.compile({{"old", CustomUniformType::Float, true, "1.0"}}, error)) << error;
 assigned = std::move(moved);
 frame.rainStrength = 0.5f;
 assigned.evaluate(frame);
 EXPECT_FLOAT_EQ(assigned.values().at("answer").asFloat(), 3.0f);
}

TEST(CustomUniformParity, RejectsCyclesAndUnknownReferences) {
 CustomUniformRuntime runtime;
 std::string error;
 EXPECT_FALSE(runtime.compile({{"a", CustomUniformType::Float, false, "b"},
                               {"b", CustomUniformType::Float, true, "a"}}, error));
 EXPECT_NE(error.find("circular"), std::string::npos);
 EXPECT_TRUE(runtime.values().empty());
 EXPECT_FALSE(runtime.compile({{"a", CustomUniformType::Float, true, "missing"}}, error));
 EXPECT_NE(error.find("missing"), std::string::npos);
 EXPECT_FALSE(runtime.compile({{"a", CustomUniformType::Int, true, "viewWidth"}}, error));
 EXPECT_FALSE(runtime.compile({{"a", CustomUniformType::Float, true, "unknownFunction(1.0)"}}, error));
}

TEST(CustomUniformParity, DuplicateSmoothIdsHaveIndependentHistories) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"brightness", CustomUniformType::Float, true, "smooth(4, eyeBrightness.y / 240.0, 5, 5)"},
     {"rainMotion", CustomUniformType::Float, true, "smooth(4, if(eyeBrightness.y > 239.0, 1, 0), 2, 2)"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameTime = 0.05f;
 frame.eyeBrightness[1] = 120;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("brightness").asFloat(), 0.5f);
 EXPECT_FLOAT_EQ(runtime.values().at("rainMotion").asFloat(), 0.0f);
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("brightness").asFloat(), 0.5f);
 EXPECT_FLOAT_EQ(runtime.values().at("rainMotion").asFloat(), 0.0f);
}

TEST(CustomUniformParity, SmoothMatchesIrisHalfLifeAndZeroFade) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"fade", CustomUniformType::Float, true, "smooth(rainStrength, 1.0)"},
     {"snap", CustomUniformType::Float, true, "smooth(rainStrength, 0.0, 0.0)"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameTime = 0.05f;
 runtime.evaluate(frame);
 frame.rainStrength = 1.0f;
 runtime.evaluate(frame);
 EXPECT_NEAR(runtime.values().at("fade").asFloat(), 0.29289322f, 1e-6f);
 EXPECT_FLOAT_EQ(runtime.values().at("snap").asFloat(), 1.0f);
 runtime.evaluate(frame);
 EXPECT_NEAR(runtime.values().at("fade").asFloat(), 0.5f, 1e-6f);
 frame.frameTime = 0.0f;
 frame.rainStrength = 0.0f;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("snap").asFloat(), 0.0f);
}

TEST(CustomUniformParity, SmoothResolvesLiteralIdOverloads) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"two", CustomUniformType::Float, true, "smooth(7, rainStrength)"},
     {"three", CustomUniformType::Float, true, "smooth(7, rainStrength, 1.0)"},
     {"four", CustomUniformType::Float, true, "smooth(7, rainStrength, 1.0, 1.0)"},
     {"expression", CustomUniformType::Float, true, "smooth(1 + 2, 1.0)"}}, error)) << error;
 PackUniformValues frame{};
 frame.rainStrength = 0.25f;
 frame.frameTime = 0.05f;
 runtime.evaluate(frame);
 for(const char* name : {"two", "three", "four"}) EXPECT_FLOAT_EQ(runtime.values().at(name).asFloat(), 0.25f);
 EXPECT_FLOAT_EQ(runtime.values().at("expression").asFloat(), 3.0f);
}

TEST(CustomUniformParity, ConditionalSmoothCallsKeepStableState) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"selected", CustomUniformType::Float, true,
      "if(is_sprinting, smooth(rainStrength, 1.0), smooth(wetness, 1.0))"},
     {"after", CustomUniformType::Float, true, "smooth(eyeAltitude, 1.0)"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameTime = 0.05f;
 frame.isSprinting = 1;
 frame.wetness = 1.0f;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("selected").asFloat(), 0.0f);
 frame.isSprinting = 0;
 frame.rainStrength = 1.0f;
 frame.eyeAltitude = 1.0f;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("selected").asFloat(), 1.0f);
 EXPECT_NEAR(runtime.values().at("after").asFloat(), 0.29289322f, 1e-6f);
 frame.isSprinting = 1;
 runtime.evaluate(frame);
 EXPECT_NEAR(runtime.values().at("selected").asFloat(), 0.29289322f, 1e-6f);
 EXPECT_NEAR(runtime.values().at("after").asFloat(), 0.5f, 1e-6f);
}

TEST(CustomUniformParity, BooleanOperatorsStillEvaluateBothOperands) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({{"value", CustomUniformType::Bool, true,
                               "is_sprinting && smooth(rainStrength, 1.0) > 0.5"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameTime = 0.05f;
 runtime.evaluate(frame);
 frame.isSprinting = 1;
 frame.rainStrength = 1.0f;
 runtime.evaluate(frame);
 EXPECT_FALSE(runtime.values().at("value").asBool());
}

TEST(CustomUniformParity, MultiConditionDefaultMatchesIrisEvaluationOrder) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({{"value", CustomUniformType::Float, true,
                               "if(false, 9.0, is_sprinting, 7.0, smooth(rainStrength, 1.0))"}}, error)) << error;
 PackUniformValues frame{};
 frame.frameTime = 0.05f;
 frame.isSprinting = 1;
 runtime.evaluate(frame);
 EXPECT_FLOAT_EQ(runtime.values().at("value").asFloat(), 7.0f);
 frame.isSprinting = 0;
 frame.rainStrength = 1.0f;
 runtime.evaluate(frame);
 EXPECT_NEAR(runtime.values().at("value").asFloat(), 0.5f, 1e-6f);
}

TEST(CustomUniformParity, VectorClampAndIntegerEdgePreserveComponentsAndPrecision) {
 CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile({
     {"clamped", CustomUniformType::Vec3, true,
      "clamp(vec3(-1.0, 5.0, 20.0), vec3(0.0, 6.0, 10.0), vec3(2.0, 8.0, 15.0))"},
     {"edgeValue", CustomUniformType::Int, true, "edge(16777217, 16777216)"}}, error)) << error;
 runtime.evaluate({});
 const auto& clamped = runtime.values().at("clamped");
 EXPECT_FLOAT_EQ(clamped.f[0], 0.0f);
 EXPECT_FLOAT_EQ(clamped.f[1], 6.0f);
 EXPECT_FLOAT_EQ(clamped.f[2], 15.0f);
 EXPECT_EQ(runtime.values().at("edgeValue").asInt(), 0);
}

TEST(CustomUniformParity, RejectsUnsupportedFunctionArities) {
 CustomUniformRuntime runtime;
 std::string error;
 for(const char* expression : {"random(1.0)", "atan2(1.0)", "randomInt(1, 2, 3)"}) {
  EXPECT_FALSE(runtime.compile({{"value", CustomUniformType::Float, true, expression}}, error)) << expression;
 }
 ASSERT_TRUE(runtime.compile({{"value", CustomUniformType::Int, true, "randomInt(1)"}}, error)) << error;
 runtime.evaluate({});
 EXPECT_EQ(runtime.values().at("value").asInt(), 0);
}

TEST(MatrixInverseParity, LargeOrthographicProjectionRoundTrips) {
 using net::minecraft::util::math::Matrix4f;
 for(const float distance : {1024.0f, 2048.0f, 4096.0f, 16384.0f}) {
  Matrix4f projection;
  projection.ortho(-distance, distance, -distance, distance, -distance, distance);
  Matrix4f inverse = projection;
  inverse.invert();
  EXPECT_FLOAT_EQ(inverse.m[0], distance);
  EXPECT_FLOAT_EQ(inverse.m[5], distance);
  EXPECT_FLOAT_EQ(inverse.m[10], -distance);
  const Matrix4f result = projection * inverse;
  for(int i = 0; i < 16; ++i) EXPECT_NEAR(result.m[i], i % 5 == 0 ? 1.0f : 0.0f, 1e-6f);
 }
}

TEST(MatrixInverseParity, TranslatedOrthographicProjectionRoundTrips) {
 using net::minecraft::util::math::Matrix4f;
 Matrix4f projection;
 projection.ortho(-4000.0f, 5000.0f, -3000.0f, 6000.0f, -8000.0f, 9000.0f);
 Matrix4f inverse = projection;
 inverse.invert();
 const Matrix4f result = projection * inverse;
 for(int i = 0; i < 16; ++i) EXPECT_NEAR(result.m[i], i % 5 == 0 ? 1.0f : 0.0f, 1e-6f);
}
}
}
