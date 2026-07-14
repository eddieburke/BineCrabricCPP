#include <gtest/gtest.h>
#include "net/minecraft/client/util/Timer.hpp"

TEST(ClientTimer, StartsWithoutSyntheticCatchUpBurst) {
  net::minecraft::client::util::Timer timer(20.0f);
  timer.advance();
  EXPECT_LT(timer.ticksThisFrame, 10);
}
