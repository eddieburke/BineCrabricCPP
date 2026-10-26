#include <gtest/gtest.h>
#include <array>
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

namespace net::minecraft::test {
using client::render::chunk::RegionSnapshot;

TEST(RegionSnapshotLight, SamplesEveryChannelValueAndBothPackedNibbles) {
 Chunk chunk(nullptr, 0, 0);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   chunk.skyLight.set(x, 64, z, x);
   chunk.blockLight.set(x, 64, z, z);
   chunk.skyLight.set(x, 65, z, 15 - x);
   chunk.blockLight.set(x, 65, z, 15 - z);
  }
 }
 const std::array sources{RegionSnapshot::SourceChunk{0, 0, &chunk}};
 RegionSnapshot snapshot(sources, 0, {}, nullptr, 0, 64, 0, 15, 65, 15);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   int block = -1;
   int sky = -1;
   snapshot.sampleLight(x, 64, z, block, sky);
   EXPECT_EQ(block, z);
   EXPECT_EQ(sky, x);
   snapshot.sampleLight(x, 65, z, block, sky);
   EXPECT_EQ(block, 15 - z);
   EXPECT_EQ(sky, 15 - x);
  }
 }
 EXPECT_TRUE(snapshot.sawSkyLight());
}

TEST(RegionSnapshotLight, PreservesMissingChunksNegativeCoordinatesAndWorldBounds) {
 Chunk chunk(nullptr, -1, -1);
 chunk.skyLight.set(15, 64, 15, 7);
 chunk.blockLight.set(15, 64, 15, 12);
 const std::array sources{RegionSnapshot::SourceChunk{-1, -1, &chunk}};
 RegionSnapshot snapshot(sources, 0, {}, nullptr, -16, 0, -16, 0, 127, 0);
 struct Sample {
  int x, y, z, block, sky;
 };
 constexpr Sample samples[] = {
     {0, 64, 0, 0, 0}, {-1, -1, -1, 0, 0}, {-1, 128, -1, 0, 0},
     {1, 64, 1, 0, 0}, {-1, 0, -1, 0, 0}, {-1, 127, -1, 0, 0}};
 for(const Sample& sample : samples) {
  int block = -1;
  int sky = -1;
  snapshot.sampleLight(sample.x, sample.y, sample.z, block, sky);
  EXPECT_EQ(block, sample.block);
  EXPECT_EQ(sky, sample.sky);
 }
 EXPECT_FALSE(snapshot.sawSkyLight());
 int block = -1;
 int sky = -1;
 snapshot.sampleLight(-1, 64, -1, block, sky);
 EXPECT_EQ(block, 12);
 EXPECT_EQ(sky, 7);
 EXPECT_TRUE(snapshot.sawSkyLight());
}
}
