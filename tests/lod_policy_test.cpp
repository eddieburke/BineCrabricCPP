#include <gtest/gtest.h>
#include "net/minecraft/client/render/lod/LodPolicy.hpp"

namespace net::minecraft::client::render::lod {
TEST(LodPolicy, SelectsStableDistanceBands) {
  const DistancePolicy policy{};
  EXPECT_EQ(policy.level(0.0f, 0), 0);
  EXPECT_EQ(policy.level(767.0f, 0), 0);
  EXPECT_EQ(policy.level(768.0f, 0), 1);
  EXPECT_EQ(policy.level(1536.0f, 0), 2);
  EXPECT_EQ(policy.level(768.0f, -1), 2);
  EXPECT_EQ(policy.level(1536.0f, 1), 1);
  EXPECT_EQ(policy.level(-1.0f, 0), 0);
  const DistancePolicy capped{768.0f, 2};
  EXPECT_EQ(capped.level(1000000.0f, 0), 2);
}
}
