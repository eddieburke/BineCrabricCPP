#include <gtest/gtest.h>
#include <vector>
#include "net/minecraft/client/render/GlState.hpp"

namespace net::minecraft::client::render {

TEST(PackBlendDrawBufferTest, MapsColortexThroughRendertargets) {
 const std::vector<int> rts = {1, 2};
 EXPECT_EQ(colortexToDrawBufferIndex(rts, 1), 0);
 EXPECT_EQ(colortexToDrawBufferIndex(rts, 2), 1);
 EXPECT_EQ(colortexToDrawBufferIndex(rts, 0), -1);
}

TEST(PackBlendDrawBufferTest, EmptyRendertargetsUsesIdentity) {
 EXPECT_EQ(colortexToDrawBufferIndex({}, 3), 3);
 EXPECT_EQ(colortexToDrawBufferIndex({}, -1), -1);
}

TEST(PackBlendDrawBufferTest, PearlEntitiesTranslucentLayout) {
 // Pearl blend.gbuffers_entities_translucent.colortex1=... / colortex2=off.
 // Those programs #define DEFERRED_IGNORE themselves → RENDERTARGETS: 1,2 → colortex1→draw0, colortex2→draw1.
 // https://shaders.properties/current/reference/shadersproperties/rendering/
 const std::vector<int> rts = {1, 2};
 EXPECT_EQ(colortexToDrawBufferIndex(rts, 1), 0);
 EXPECT_EQ(colortexToDrawBufferIndex(rts, 2), 1);
}

TEST(PackBlendDrawBufferTest, MapsHighColortexIndices) {
 // https://shaders.properties/current/reference/buffers/colortex/
 EXPECT_EQ(colortexToDrawBufferIndex({16, 31}, 16), 0);
 EXPECT_EQ(colortexToDrawBufferIndex({16, 31}, 31), 1);
 EXPECT_EQ(blendFactor("SRC_ALPHA_SATURATE"), 0x0308u);
}

TEST(PackBlendDrawBufferTest, BlendFactorNamesMatchGlEnums) {
 // https://github.com/IrisShaders/Iris/blob/1.20.1/src/main/java/net/irisshaders/iris/gl/blending/BlendModeFunction.java
 EXPECT_EQ(blendFactor("ZERO"), 0u);
 EXPECT_EQ(blendFactor("ONE"), 1u);
 EXPECT_EQ(blendFactor("SRC_ALPHA"), 0x0302u);
 EXPECT_EQ(blendFactor("ONE_MINUS_SRC_ALPHA"), 0x0303u);
 EXPECT_EQ(blendFactor("SRC_COLOR"), 0x0300u);
 EXPECT_EQ(blendFactor("DST_COLOR"), 0x0306u);
}

} // namespace net::minecraft::client::render
