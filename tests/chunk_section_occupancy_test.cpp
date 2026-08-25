#include <gtest/gtest.h>
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::test {
TEST(ChunkSectionOccupancy, TracksBulkAndCellWrites) {
 Chunk chunk(nullptr, 0, 0);
 chunk.blocks[0] = 1;
 chunk.blocks[31] = 1;
 chunk.blocks[127] = 1;
 chunk.populateHeightMapOnly();
 EXPECT_TRUE(chunk.sectionHasBlocks(0));
 EXPECT_TRUE(chunk.sectionHasBlocks(1));
 EXPECT_TRUE(chunk.sectionHasBlocks(7));
 EXPECT_EQ(chunk.nonEmptySectionCount(), 3);
 EXPECT_TRUE(chunk.setBlock(0, 0, 0, 0));
 EXPECT_FALSE(chunk.sectionHasBlocks(0));
 EXPECT_TRUE(chunk.setBlock(0, 64, 0, 1));
 EXPECT_TRUE(chunk.sectionHasBlocks(4));
 EXPECT_EQ(chunk.nonEmptySectionCount(), 3);
 EXPECT_TRUE(chunk.setBlock(0, 64, 0, 2));
 EXPECT_TRUE(chunk.sectionHasBlocks(4));
 EXPECT_TRUE(chunk.setBlock(0, 64, 0, 0));
 EXPECT_FALSE(chunk.sectionHasBlocks(4));
}
TEST(ChunkSectionOccupancy, TracksBlockLightSections) {
 Chunk chunk(nullptr, 0, 0);
 chunk.setLight(LightType::Block, 0, 17, 0, 12);
 chunk.setLight(LightType::Block, 0, 95, 0, 5);
 EXPECT_EQ(chunk.blockLightSectionMask(), static_cast<std::uint8_t>((1U << 1) | (1U << 5)));
 chunk.setLight(LightType::Block, 0, 17, 0, 0);
 EXPECT_EQ(chunk.blockLightSectionMask(), static_cast<std::uint8_t>(1U << 5));
 chunk.blockLight.set(0, 127, 0, 15);
 chunk.refreshBlockLightCounts();
 EXPECT_EQ(chunk.blockLightSectionMask(), static_cast<std::uint8_t>((1U << 5) | (1U << 7)));
}
}
