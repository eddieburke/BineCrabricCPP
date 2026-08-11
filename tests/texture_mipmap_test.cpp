#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>
#include "net/minecraft/client/texture/TextureMipmap.hpp"
namespace net::minecraft::test {
namespace {
using client::texture::buildRgbaMipmaps;
using client::texture::downsampleRgbaMipmap;
std::array<std::uint8_t, 4> pixel(const std::vector<std::uint8_t>& image, int width, int x, int y) {
 const std::size_t offset = static_cast<std::size_t>(x + y * width) * 4;
 return {image[offset], image[offset + 1], image[offset + 2], image[offset + 3]};
}
} // namespace
TEST(TextureMipmapTest, HalfCoverageSurvivesCutoutThreshold) {
 const std::array<std::uint8_t, 16> source = {
     24, 180, 48, 255, 24, 180, 48, 255, 255, 0, 255, 0, 255, 0, 255, 0};
 std::array<std::uint8_t, 4> destination{};
 downsampleRgbaMipmap(source, 2, 2, destination);
 EXPECT_EQ(destination[0], 24);
 EXPECT_EQ(destination[1], 180);
 EXPECT_EQ(destination[2], 48);
 EXPECT_EQ(destination[3], 128);
}
TEST(TextureMipmapTest, TransparentTexelColorsDoNotCreateEdgeHalos) {
 const std::array<std::uint8_t, 16> source = {
     0, 96, 224, 255, 255, 255, 255, 0, 255, 0, 0, 0, 0, 96, 224, 255};
 std::array<std::uint8_t, 4> destination{};
 downsampleRgbaMipmap(source, 2, 2, destination);
 EXPECT_EQ(destination[0], 0);
 EXPECT_EQ(destination[1], 96);
 EXPECT_EQ(destination[2], 224);
 EXPECT_EQ(destination[3], 128);
}
TEST(TextureMipmapTest, GridAtlasTilesNeverBleedIntoNeighbors) {
 std::vector<std::uint8_t> source(32 * 16 * 4);
 for(int y = 0; y < 16; ++y) {
  for(int x = 0; x < 32; ++x) {
   const std::size_t offset = static_cast<std::size_t>(x + y * 32) * 4;
   source[offset + (x < 16 ? 0 : 2)] = 255;
   source[offset + 3] = 255;
  }
 }
 const auto levels = buildRgbaMipmaps(source, 32, 16, 4, 2, 1);
 ASSERT_EQ(levels.size(), 4u);
 for(const auto& level : levels) {
  const auto left = pixel(level.pixels, level.width, level.width / 4, level.height / 2);
  const auto right = pixel(level.pixels, level.width, level.width * 3 / 4, level.height / 2);
  EXPECT_EQ(left, (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
  EXPECT_EQ(right, (std::array<std::uint8_t, 4>{0, 0, 255, 255}));
 }
}
TEST(TextureMipmapTest, GammaAwareBlendDoesNotDarkenHardContrasts) {
 const std::array<std::uint8_t, 16> source = {
     255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255};
 std::array<std::uint8_t, 4> destination{};
 downsampleRgbaMipmap(source, 2, 2, destination);
 EXPECT_NEAR(destination[0], 180, 1);
 EXPECT_NEAR(destination[1], 180, 1);
 EXPECT_NEAR(destination[2], 180, 1);
 EXPECT_EQ(destination[3], 255);
}
TEST(TextureMipmapTest, InvalidAtlasLayoutIsRejected) {
 std::array<std::uint8_t, 4 * 4 * 4> source{};
 std::array<std::uint8_t, 2 * 2 * 4> destination{};
 EXPECT_THROW(downsampleRgbaMipmap(source, 4, 4, destination, 3, 1), std::invalid_argument);
}
} // namespace net::minecraft::test
