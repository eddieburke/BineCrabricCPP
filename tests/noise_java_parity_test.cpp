#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/math/noise/PerlinNoiseSampler.hpp"

namespace net::minecraft::test {
TEST(JavaNumericParity, DoubleToIntUsesJavaNarrowingRules) {
 EXPECT_EQ(java_double_to_int(std::numeric_limits<double>::quiet_NaN()), 0);
 EXPECT_EQ(java_double_to_int(std::numeric_limits<double>::infinity()), std::numeric_limits<int>::max());
 EXPECT_EQ(java_double_to_int(-std::numeric_limits<double>::infinity()), std::numeric_limits<int>::min());
 EXPECT_EQ(java_double_to_int(2147483647.75), std::numeric_limits<int>::max());
 EXPECT_EQ(java_double_to_int(-2147483648.75), std::numeric_limits<int>::min());
 EXPECT_EQ(java_double_to_int(-12.75), -12);
 EXPECT_EQ(java_double_to_int(12.75), 12);
}

TEST(JavaNumericParity, PerlinFarLandsCoordinatesRemainFinite) {
 JavaRandom random(0);
 PerlinNoiseSampler sampler(random);
 std::vector<double> values(5 * 17 * 5);
 sampler.create(values, 3'137'705.0, 0.0, 0.0, 5, 17, 5, 684.412, 684.412, 684.412, 1.0);
 for(double value : values) {
  EXPECT_TRUE(std::isfinite(value));
 }
}
}
