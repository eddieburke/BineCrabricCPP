#include <gtest/gtest.h>
#include "net/minecraft/client/option/RenderDistance.hpp"
TEST(RenderDistanceTest, KeepsFogDistanceSeparateFromTheProjectionClipPlane) {
 net::minecraft::client::option::RenderDistance distance;
 distance.blocks = 256.0f;
 EXPECT_FLOAT_EQ(distance.fogEnd(), 256.0f);
 EXPECT_FLOAT_EQ(distance.farPlane(), 512.0f);
 EXPECT_GT(distance.farPlane(), distance.fogEnd());
}
