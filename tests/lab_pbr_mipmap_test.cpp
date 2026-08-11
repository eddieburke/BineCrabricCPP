#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include "net/minecraft/client/render/PbrTextures.hpp"
namespace {
using PbrTextures = net::minecraft::client::render::PbrTextures;
using Type = PbrTextures::Type;
std::uint32_t argb(int a, int r, int g, int b) {
 return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(r) << 16) |
        (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b);
}
} // namespace
TEST(PbrMipmap, SelectTargetTypeMajorityAndTieRules) {
 // Port of DiscreteBlendFunction.selectTargetType branch logic.
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 0, 1, 1), 0);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(1, 0, 1, 0), 1);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 1, 0, 1), 0);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 1, 1, 1), 1);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 1, 2, 2), 2);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 1, 2, 3), 0);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 2, 1, 2), 2);
 EXPECT_EQ(PbrTextures::pbrSelectTargetType(0, 1, 1, 2), 1);
}
TEST(PbrMipmap, SpecularChannelClassifiers) {
 EXPECT_EQ(PbrTextures::pbrSpecularTypeG(0), 0);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeG(229), 0);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeG(230), 1);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeG(255), 26);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeB(64), 0);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeB(65), 1);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeB(255), 1);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeA(254), 0);
 EXPECT_EQ(PbrTextures::pbrSpecularTypeA(255), 1);
}
TEST(PbrMipmap, NormalBlendIsLinearOnAllChannels) {
 // Java: NORMAL uses LINEAR_MIPMAP_GENERATOR (all four channels linear).
 const std::uint32_t blended =
     PbrTextures::pbrBlendMip4(argb(255, 0, 200, 100), argb(255, 255, 200, 100),
                               argb(255, 255, 240, 100), argb(255, 0, 240, 0), Type::Normal,
                               true);
 EXPECT_EQ(blended, argb(255, 127, 220, 75));
}
TEST(PbrMipmap, SpecularLabPbrBlendIsChannelDiscrete) {
 // Java: labPBR SPECULAR_MIPMAP_GENERATOR — R linear, G/B/A discrete.
 const std::uint32_t blended =
     PbrTextures::pbrBlendMip4(argb(255, 0, 200, 100), argb(255, 255, 200, 100),
                               argb(255, 255, 240, 100), argb(255, 0, 240, 0), Type::Specular,
                               true);
 // R: (0+255+255+0)/4=127; G: type 0 wins over 11 -> (200+200)/2=200;
 // B: type 1 (>=65) wins over 0 -> (100+100+100)/3=100; A: 255.
 EXPECT_EQ(blended, argb(255, 127, 200, 100));
}
TEST(PbrMipmap, SpecularWithoutLabPbrFormatBlendsLinearly) {
 // Java: no texture format -> PBRSpriteContents falls back to the linear
 // generator even for specular maps.
 const std::uint32_t blended =
     PbrTextures::pbrBlendMip4(argb(255, 0, 200, 100), argb(255, 255, 200, 100),
                               argb(255, 255, 240, 100), argb(255, 0, 240, 0), Type::Specular,
                               false);
 EXPECT_EQ(blended, argb(255, 127, 220, 75));
}
TEST(PbrMipmap, DiscreteBlendAveragesOnlyTargetTypeMembers) {
 // R stays linear: (230+230+240+240)/4=235.
 EXPECT_EQ(PbrTextures::pbrBlendMip4(argb(255, 230, 0, 0), argb(255, 230, 0, 0),
                                     argb(255, 240, 0, 0), argb(255, 240, 0, 0),
                                     Type::Specular, true),
           argb(255, 235, 0, 0));
 // B channel: (64,64,65,65) -> types (0,0,1,1) -> target 0 -> average 64.
 EXPECT_EQ(PbrTextures::pbrBlendMip4(argb(255, 0, 0, 64), argb(255, 0, 0, 64),
                                     argb(255, 0, 0, 65), argb(255, 0, 0, 65),
                                     Type::Specular, true),
           argb(255, 0, 0, 64));
 // A channel: (255,255,255,0) -> types (1,1,1,0) -> target 1 -> average 255.
 EXPECT_EQ(PbrTextures::pbrBlendMip4(argb(255, 0, 0, 0), argb(255, 0, 0, 0),
                                     argb(255, 0, 0, 0), argb(0, 0, 0, 0),
                                     Type::Specular, true),
           argb(255, 0, 0, 0));
}
TEST(PbrMipmap, DownsampleHalvesLinearImage) {
 // 4x4 gradient RGBA(r=x*32, g=y*32, b=128, a=255) -> 2x2 averages.
 std::array<std::uint8_t, 4 * 4 * 4> src{};
 for(int y = 0; y < 4; ++y) {
  for(int x = 0; x < 4; ++x) {
   const std::size_t offset = (static_cast<std::size_t>(y) * 4 + x) * 4;
   src[offset + 0] = static_cast<std::uint8_t>(x * 32);
   src[offset + 1] = static_cast<std::uint8_t>(y * 32);
   src[offset + 2] = 128;
   src[offset + 3] = 255;
  }
 }
 std::array<std::uint8_t, 2 * 2 * 4> dst{};
 PbrTextures::pbrDownsampleMip(src.data(), 4, 4, dst.data(), 2, 2, Type::Normal, true);
 const auto pixel = [&dst](int x, int y) {
  const std::size_t offset = (static_cast<std::size_t>(y) * 2 + x) * 4;
  return std::array<std::uint8_t, 4>{dst[offset + 0], dst[offset + 1], dst[offset + 2],
                                     dst[offset + 3]};
 };
 EXPECT_EQ(pixel(0, 0), (std::array<std::uint8_t, 4>{16, 16, 128, 255}));
 EXPECT_EQ(pixel(1, 0), (std::array<std::uint8_t, 4>{80, 16, 128, 255}));
 EXPECT_EQ(pixel(0, 1), (std::array<std::uint8_t, 4>{16, 80, 128, 255}));
 EXPECT_EQ(pixel(1, 1), (std::array<std::uint8_t, 4>{80, 80, 128, 255}));
}
TEST(PbrMipmap, DownsampleAppliesSpecularDiscreteRules) {
 // B channel block (100,100,100,64) must average to 100, not 91: the 64
 // belongs to type 0 while the rest are type 1.
 std::array<std::uint8_t, 4 * 4 * 4> src{};
 for(int y = 0; y < 4; ++y) {
  for(int x = 0; x < 4; ++x) {
   const std::size_t offset = (static_cast<std::size_t>(y) * 4 + x) * 4;
   const bool target = x < 2 && y < 2;
   src[offset + 0] = target ? static_cast<std::uint8_t>(100 + x * 60) : 0;
   src[offset + 1] = 200;
   src[offset + 2] = target ? 100 : 64;
   src[offset + 3] = target ? 255 : 0;
  }
 }
 std::array<std::uint8_t, 2 * 2 * 4> dst{};
 PbrTextures::pbrDownsampleMip(src.data(), 4, 4, dst.data(), 2, 2, Type::Specular, true);
 const auto pixel = [&dst](int x, int y) {
  const std::size_t offset = (static_cast<std::size_t>(y) * 2 + x) * 4;
  return std::array<std::uint8_t, 4>{dst[offset + 0], dst[offset + 1], dst[offset + 2],
                                     dst[offset + 3]};
 };
 // R linear: (100+160+100+160)/4=130; G: 200; B: type 1 wins -> (100+100+100)/3=100;
 // A: type 1 wins -> 255.
 EXPECT_EQ(pixel(0, 0), (std::array<std::uint8_t, 4>{130, 200, 100, 255}));
 // All four pixels are type 0 for B (64) and A (0) -> linear averages.
 EXPECT_EQ(pixel(1, 1), (std::array<std::uint8_t, 4>{0, 200, 64, 0}));
}
TEST(PbrMipmap, DownsampleInPlaceMatchesOutOfPlace) {
 std::array<std::uint8_t, 4 * 4 * 4> src{};
 for(int i = 0; i < 4 * 4; ++i) {
  src[i * 4 + 0] = static_cast<std::uint8_t>(i * 13);
  src[i * 4 + 1] = static_cast<std::uint8_t>(i * 7);
  src[i * 4 + 2] = static_cast<std::uint8_t>(255 - i * 3);
  src[i * 4 + 3] = 255;
 }
 std::array<std::uint8_t, 2 * 2 * 4> outOfPlace{};
 PbrTextures::pbrDownsampleMip(src.data(), 4, 4, outOfPlace.data(), 2, 2, Type::Specular,
                               true);
 std::array<std::uint8_t, 4 * 4 * 4> inPlace = src;
 PbrTextures::pbrDownsampleMip(inPlace.data(), 4, 4, inPlace.data(), 2, 2, Type::Specular,
                               true);
 for(std::size_t i = 0; i < outOfPlace.size(); ++i) {
  EXPECT_EQ(inPlace[i], outOfPlace[i]);
 }
}
TEST(PbrMipmap, AtlasDownsampleKeepsNormalTilesIndependent) {
 std::array<std::uint8_t, 4 * 2 * 4> source{};
 for(int y = 0; y < 2; ++y) {
  for(int x = 0; x < 4; ++x) {
   const std::size_t offset = static_cast<std::size_t>(x + y * 4) * 4;
   source[offset + 0] = x < 2 ? 255 : 0;
   source[offset + 1] = 128;
   source[offset + 2] = x < 2 ? 0 : 255;
   source[offset + 3] = 255;
  }
 }
 std::array<std::uint8_t, 2 * 1 * 4> destination{};
 PbrTextures::pbrDownsampleMip(source.data(), 4, 2, destination.data(), 2, 1,
                               Type::Normal, true, 2, 1);
 EXPECT_EQ(destination, (std::array<std::uint8_t, 8>{255, 128, 0, 255, 0, 128, 255, 255}));
}
TEST(PbrMipmap, ScaleNearestUsesIntegerMultipleBlocks) {
 net::minecraft::client::texture::RasterImage source;
 source.width = 2;
 source.height = 2;
 source.argb = {argb(255, 10, 20, 30), argb(255, 40, 50, 60),
                argb(255, 70, 80, 90), argb(255, 100, 110, 120)};
 const net::minecraft::client::texture::RasterImage scaled =
     PbrTextures::pbrScaleNearest(source, 4, 4);
 EXPECT_EQ(scaled.width, 4);
 EXPECT_EQ(scaled.height, 4);
 EXPECT_EQ(scaled.argb[0], argb(255, 10, 20, 30));
 EXPECT_EQ(scaled.argb[static_cast<std::size_t>(3) * 4 + 3], argb(255, 100, 110, 120));
}
TEST(PbrMipmap, ScaleBilinearBlendsInteriorPixels) {
 net::minecraft::client::texture::RasterImage source;
 source.width = 2;
 source.height = 2;
 source.argb = {argb(255, 10, 20, 30), argb(255, 40, 50, 60),
                argb(255, 70, 80, 90), argb(255, 100, 110, 120)};
 const net::minecraft::client::texture::RasterImage scaled =
     PbrTextures::pbrScaleBilinear(source, 3, 3);
 EXPECT_EQ(scaled.width, 3);
 EXPECT_EQ(scaled.height, 3);
 // Interior pixel (1,1) averages the four corners with 0.25 weights.
 EXPECT_EQ(scaled.argb[static_cast<std::size_t>(1) * 3 + 1], argb(255, 55, 65, 75));
 // Out-of-range taps clamp to the nearest source pixel.
 EXPECT_EQ(scaled.argb[static_cast<std::size_t>(2) * 3 + 2], argb(255, 100, 110, 120));
 EXPECT_EQ(scaled.argb[0], argb(255, 10, 20, 30));
}
