#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "support/server_event_fixture.hpp"

namespace net::minecraft::test {

TEST(WaterSourceTest, InfiniteWaterSourceFormsBetweenTwoSources) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Build a 3x1 trench along X at y=64:
 // (7, 64, 8), (8, 64, 8), (9, 64, 8)
 // Floor at y=63 is stone; surrounding blocks at y=64 are stone.
 for(int x = 6; x <= 10; ++x) {
  for(int z = 7; z <= 9; ++z) {
   world.setBlock(x, 63, z, Block::STONE->id);
   if(z == 8 && x >= 7 && x <= 9) {
    world.setBlock(x, 64, z, 0);
   } else {
    world.setBlock(x, 64, z, Block::STONE->id);
   }
  }
 }

 // Place two water sources at opposite ends of the trench:
 world.setBlock(7, 64, 8, Block::FLOWING_WATER->id, 0);
 world.setBlock(9, 64, 8, Block::FLOWING_WATER->id, 0);

 // Tick until water converges and forms an infinite source at (8, 64, 8)
 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 // Middle block must now be a water source block (meta 0, settled to still water id 9)
 const int middleId = world.getBlockId(8, 64, 8);
 const int middleMeta = world.getBlockMeta(8, 64, 8);
 EXPECT_EQ(middleId, Block::WATER->id);
 EXPECT_EQ(middleMeta, 0)
     << "Expected source metadata 0 at (8, 64, 8), got " << middleMeta;
}

TEST(WaterSourceTest, InfiniteWaterSourceRefillsAfterScooping) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Build 3x1 trench at y=64
 for(int x = 6; x <= 10; ++x) {
  for(int z = 7; z <= 9; ++z) {
   world.setBlock(x, 63, z, Block::STONE->id);
   if(z == 8 && x >= 7 && x <= 9) {
    world.setBlock(x, 64, z, 0);
   } else {
    world.setBlock(x, 64, z, Block::STONE->id);
   }
  }
 }

 world.setBlock(7, 64, 8, Block::FLOWING_WATER->id, 0);
 world.setBlock(9, 64, 8, Block::FLOWING_WATER->id, 0);

 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 ASSERT_EQ(world.getBlockId(8, 64, 8), Block::WATER->id);
 ASSERT_EQ(world.getBlockMeta(8, 64, 8), 0);

 // Scoop the middle block (turn it to air, notifying neighbors)
 world.setBlock(8, 64, 8, 0);

 // Tick the world: the adjacent source blocks should flow in and replenish it
 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::WATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 8), 0);
}

TEST(WaterSourceTest, TwoByTwoInfiniteWaterSourcePool) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // 2x2 pool at x in [8, 9], z in [8, 9], y=64 with stone floor at y=63
 for(int x = 7; x <= 10; ++x) {
  for(int z = 7; z <= 10; ++z) {
   world.setBlock(x, 63, z, Block::STONE->id);
   if(x >= 8 && x <= 9 && z >= 8 && z <= 9) {
    world.setBlock(x, 64, z, 0);
   } else {
    world.setBlock(x, 64, z, Block::STONE->id);
   }
  }
 }

 // Place diagonal water sources: (8, 64, 8) and (9, 64, 9)
 world.setBlock(8, 64, 8, Block::FLOWING_WATER->id, 0);
 world.setBlock(9, 64, 9, Block::FLOWING_WATER->id, 0);

 // Tick until all 4 blocks become sources
 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 for(int x = 8; x <= 9; ++x) {
  for(int z = 8; z <= 9; ++z) {
   EXPECT_EQ(world.getBlockId(x, 64, z), Block::WATER->id)
       << "Block at (" << x << ", 64, " << z << ") did not become still water";
   EXPECT_EQ(world.getBlockMeta(x, 64, z), 0)
       << "Block at (" << x << ", 64, " << z << ") has non-source metadata";
  }
 }

 // Scoop one corner: (8, 64, 8)
 world.setBlock(8, 64, 8, 0);

 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::WATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 8), 0);
}

TEST(WaterSourceTest, WaterDoesNotFormInfiniteSourceInMidAir) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Clear a column below (8, 64, 8) down to y=50 so water falls into mid-air
 for(int y = 50; y <= 63; ++y) {
  world.setBlock(8, y, 8, 0);
 }

 // Trench at y=64, with stone walls and stone under (7, 64, 8) and (9, 64, 8)
 for(int x = 6; x <= 10; ++x) {
  for(int z = 7; z <= 9; ++z) {
   if(x != 8 || z != 8) {
    world.setBlock(x, 63, z, Block::STONE->id);
   }
   if(z == 8 && x >= 7 && x <= 9) {
    world.setBlock(x, 64, z, 0);
   } else {
    world.setBlock(x, 64, z, Block::STONE->id);
   }
  }
 }

 world.setBlock(7, 64, 8, Block::FLOWING_WATER->id, 0);
 world.setBlock(9, 64, 8, Block::FLOWING_WATER->id, 0);

 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 // Middle block must NOT be a water source (metadata must NOT be 0)
 const int middleMeta = world.getBlockMeta(8, 64, 8);
 EXPECT_NE(middleMeta, 0)
     << "Water in mid-air should not have formed a source block (meta 0), got meta " << middleMeta;
}

TEST(WaterSourceTest, WaterFormsInfiniteSourceOverWaterSource) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Floor at y=62
 for(int x = 6; x <= 10; ++x) {
  for(int z = 7; z <= 9; ++z) {
   world.setBlock(x, 62, z, Block::STONE->id);
  }
 }

 // Walls around trench at y=63 and y=64
 for(int y = 63; y <= 64; ++y) {
  for(int x = 6; x <= 10; ++x) {
   for(int z = 7; z <= 9; ++z) {
    if(z == 8 && x >= 7 && x <= 9) {
     world.setBlock(x, y, z, 0);
    } else {
     world.setBlock(x, y, z, Block::STONE->id);
    }
   }
  }
 }

 // Put water source below at (8, 63, 8)
 world.setBlock(8, 63, 8, Block::FLOWING_WATER->id, 0);

 // Place two water sources at y=64: (7, 64, 8) and (9, 64, 8)
 world.setBlock(7, 64, 8, Block::FLOWING_WATER->id, 0);
 world.setBlock(9, 64, 8, Block::FLOWING_WATER->id, 0);

 for(int t = 0; t < 50; ++t) {
  world.tick();
 }

 // Middle block at (8, 64, 8) must form an infinite source over the water source at y=63
 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::WATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 8), 0);
}

TEST(WaterSourceTest, LavaDoesNotFormInfiniteSource) {
 ServerEventFixture fixture;
 auto& world = fixture.world;

 for(int cx = -1; cx <= 2; ++cx) {
  for(int cz = -1; cz <= 2; ++cz) {
   world.chunkCache->loadChunk(cx, cz);
  }
 }

 // Trench at y=64 with solid stone floor at y=63
 for(int x = 6; x <= 10; ++x) {
  for(int z = 7; z <= 9; ++z) {
   world.setBlock(x, 63, z, Block::STONE->id);
   if(z == 8 && x >= 7 && x <= 9) {
    world.setBlock(x, 64, z, 0);
   } else {
    world.setBlock(x, 64, z, Block::STONE->id);
   }
  }
 }

 // Place two lava sources at opposite ends of the trench
 world.setBlock(7, 64, 8, Block::FLOWING_LAVA->id, 0);
 world.setBlock(9, 64, 8, Block::FLOWING_LAVA->id, 0);

 for(int t = 0; t < 100; ++t) {
  world.tick();
 }

 // Middle block must NOT be a lava source: its metadata must NOT be 0!
 const int middleMeta = world.getBlockMeta(8, 64, 8);
 EXPECT_NE(middleMeta, 0)
     << "Lava should never form a source block (meta 0) from convergence, got meta " << middleMeta;
}

} // namespace net::minecraft::test
