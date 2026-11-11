#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "support/server_event_fixture.hpp"

namespace net::minecraft::test {

TEST(LavaFlowDecayTest, FlowingLavaDisappearsWhenSourceRemoved) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 // Ensure region around (8, 64, 8) has all required chunks loaded for isRegionLoaded checks
 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Build a 1-wide trench along X at y=64 with a stone floor at y=63:
 // Channel runs along x from 8 to 14 at z=8, y=64.
 for(int x = 6; x <= 16; ++x) {
  for(int z = 6; z <= 10; ++z) {
   world.setBlock(x, 63, z, Block::STONE->id);
   if(z != 8 || x < 8 || x > 14) {
    world.setBlock(x, 64, z, Block::STONE->id);
   } else {
    world.setBlock(x, 64, z, 0);
   }
  }
 }

 // BucketItem places FLOWING_LAVA (id 10) with meta 0.
 // On placed, FlowingLiquidBlock schedules an update, which converts it to still lava and spreads.
 world.setBlock(8, 64, 8, Block::FLOWING_LAVA->id, 0);

 // Tick the world until lava spreads down the channel.
 // Lava tick rate is 30 ticks per step; over 200 ticks it should flow at least 2-3 blocks.
 for(int t = 0; t < 200; ++t) {
  world.tick();
 }

 // Verify lava flowed to adjacent channel blocks
 const int idAt9 = world.getBlockId(9, 64, 8);
 const int idAt10 = world.getBlockId(10, 64, 8);
 EXPECT_TRUE(idAt9 == Block::FLOWING_LAVA->id || idAt9 == Block::LAVA->id)
     << "Expected lava at (9, 64, 8), got id " << idAt9;
 EXPECT_TRUE(idAt10 == Block::FLOWING_LAVA->id || idAt10 == Block::LAVA->id)
     << "Expected lava at (10, 64, 8), got id " << idAt10;

 // Remove the source block at (8, 64, 8)
 world.setBlock(8, 64, 8, 0);

 // Tick the world to allow lava flow decay.
 // Under MC-4631, lava decay failed to schedule ticks and permanently froze at intermediate levels.
 // With the fix, lava decays and disappears completely.
 for(int t = 0; t < 1000; ++t) {
  world.tick();
 }

 // All lava in the channel must now have disappeared (turned to air)
 EXPECT_EQ(world.getBlockId(8, 64, 8), 0);
 EXPECT_EQ(world.getBlockId(9, 64, 8), 0);
 EXPECT_EQ(world.getBlockId(10, 64, 8), 0);
 EXPECT_EQ(world.getBlockId(11, 64, 8), 0);
}

TEST(LavaFlowDecayTest, VerticalLavaColumnDisappearsWhenSourceRemoved) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Floor at y=58
 for(int x = 7; x <= 9; ++x) {
  for(int z = 7; z <= 9; ++z) {
   world.setBlock(x, 58, z, Block::STONE->id);
  }
 }

 // Vertical column of air from y=59 to y=64 surrounded by stone
 for(int y = 59; y <= 64; ++y) {
  for(int x = 7; x <= 9; ++x) {
   for(int z = 7; z <= 9; ++z) {
    if(x == 8 && z == 8) {
     world.setBlock(x, y, z, 0);
    } else {
     world.setBlock(x, y, z, Block::STONE->id);
    }
   }
  }
 }

 // Place lava source at top of shaft: (8, 64, 8) as FLOWING_LAVA
 world.setBlock(8, 64, 8, Block::FLOWING_LAVA->id, 0);

 // Let lava flow down to the bottom
 for(int t = 0; t < 200; ++t) {
  world.tick();
 }

 // Verify lava reached bottom of shaft (8, 59, 8)
 const int idAtBottom = world.getBlockId(8, 59, 8);
 EXPECT_TRUE(idAtBottom == Block::FLOWING_LAVA->id || idAtBottom == Block::LAVA->id)
     << "Expected lava at bottom (8, 59, 8), got id " << idAtBottom;

 // Remove the lava source block at (8, 64, 8)
 world.setBlock(8, 64, 8, 0);

 // Tick until the column recedes
 for(int t = 0; t < 600; ++t) {
  world.tick();
 }

 // Verify all levels of the vertical shaft are now air
 for(int y = 59; y <= 64; ++y) {
  EXPECT_EQ(world.getBlockId(8, y, 8), 0) << "Lava lingering at y = " << y;
 }
}

} // namespace net::minecraft::test
