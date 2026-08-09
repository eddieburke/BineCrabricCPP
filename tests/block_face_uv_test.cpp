#include <array>
#include <gtest/gtest.h>
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
namespace {
using net::minecraft::Box;
using net::minecraft::client::render::ResolvedTexture;
using net::minecraft::client::render::block::SideFaceDirection;
using net::minecraft::client::render::block::SideFaceUv;
using net::minecraft::client::render::block::sideFaceUvFor;
std::array<std::array<double, 2>, 4> corners(const SideFaceUv& uv) {
 return {{{uv.vertices[0].u, uv.vertices[0].v},
          {uv.vertices[1].u, uv.vertices[1].v},
          {uv.vertices[2].u, uv.vertices[2].v},
          {uv.vertices[3].u, uv.vertices[3].v}}};
}
} // namespace
TEST(BlockFaceUv, RotationsKeepFourDistinctCorners) {
 const ResolvedTexture texture{32.0 / 256.0,
                               48.0 / 256.0,
                               64.0 / 256.0,
                               80.0 / 256.0,
                               1.0 / 256.0,
                               1.0 / 256.0,
                               -1,
                               false};
 const Box bounds{0.125, 0.25, 0.25, 0.875, 0.75, 0.875};
 const std::array<SideFaceDirection, 4> faces = {
     SideFaceDirection::east,
     SideFaceDirection::west,
     SideFaceDirection::north,
     SideFaceDirection::south,
 };
 for(const SideFaceDirection face : faces) {
  for(int rotation = 0; rotation < 4; ++rotation) {
   const auto uv = sideFaceUvFor(texture, bounds, false, face, rotation);
   const auto faceCorners = corners(uv);
   for(const auto& corner : faceCorners) {
    EXPECT_GE(corner[0], texture.uMin);
    EXPECT_LE(corner[0], texture.uMax);
    EXPECT_GE(corner[1], texture.vMin);
    EXPECT_LE(corner[1], texture.vMax);
   }
   EXPECT_NE(faceCorners[0], faceCorners[1]);
   EXPECT_NE(faceCorners[1], faceCorners[2]);
   EXPECT_NE(faceCorners[2], faceCorners[3]);
  }
 }
}
TEST(BlockFaceUv, ModTextureScaleRemainsNormalizedWhenRotated) {
 const ResolvedTexture texture{0.0, 1.0, 0.0, 1.0, 1.0 / 16.0, 1.0 / 16.0, -1, true};
 const Box bounds{0.125, 0.25, 0.25, 0.875, 0.75, 0.875};
 for(int rotation = 0; rotation < 4; ++rotation) {
  const auto uv = sideFaceUvFor(texture, bounds, false, SideFaceDirection::east, rotation);
  for(const auto& corner : corners(uv)) {
   EXPECT_GE(corner[0], 0.0);
   EXPECT_LE(corner[0], 1.0);
   EXPECT_GE(corner[1], 0.0);
   EXPECT_LE(corner[1], 1.0);
  }
 }
}
TEST(BlockFaceUv, AtlasBoundariesAreInsetSymmetricallyForEveryRotation) {
 const ResolvedTexture texture{32.0 / 256.0,
                               48.0 / 256.0,
                               64.0 / 256.0,
                               80.0 / 256.0,
                               1.0 / 256.0,
                               1.0 / 256.0,
                               -1,
                               false};
 const Box bounds{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
 for(const SideFaceDirection face : {SideFaceDirection::east, SideFaceDirection::west,
                                     SideFaceDirection::north, SideFaceDirection::south}) {
  for(int rotation = 0; rotation < 4; ++rotation) {
   for(const auto& corner : corners(sideFaceUvFor(texture, bounds, false, face, rotation))) {
    EXPECT_GE(corner[0], texture.safeUMin());
    EXPECT_LE(corner[0], texture.safeUMax());
    EXPECT_GE(corner[1], texture.safeVMin());
    EXPECT_LE(corner[1], texture.safeVMax());
   }
  }
 }
}
