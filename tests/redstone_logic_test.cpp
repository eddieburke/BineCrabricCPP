#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "support/server_event_fixture.hpp"

namespace net::minecraft::test {

namespace {

void loadChunksAround(ServerEventFixture& fixture, int minChunk, int maxChunk) {
 for(int cx = minChunk; cx <= maxChunk; ++cx) {
  for(int cz = minChunk; cz <= maxChunk; ++cz) {
   fixture.world.chunkCache->loadChunk(cx, cz);
  }
 }
 for(int t = 0; t < 10; ++t) {
  fixture.world.tick();
 }
}

void buildStonePlatform(World& world, int x0, int z0, int x1, int z1, int yFloor = 63) {
 for(int x = x0; x <= x1; ++x) {
  for(int z = z0; z <= z1; ++z) {
   world.setBlock(x, yFloor, z, Block::STONE->id);
   world.setBlock(x, yFloor + 1, z, 0);
   world.setBlock(x, yFloor + 2, z, 0);
   world.setBlock(x, yFloor + 3, z, 0);
  }
 }
}

} // namespace

TEST(RedstoneLogicTest, WireSignalDecaysOnePerBlockOverDistance) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, -1, 5, 20, 11, 63);

 // Place a lit redstone torch on top of stone at (0, 64, 8)
 world.setBlock(0, 64, 8, Block::LIT_REDSTONE_TORCH->id, 5);

 // Place a line of 16 redstone wires along +X: (1, 64, 8) to (16, 64, 8)
 for(int x = 1; x <= 16; ++x) {
  world.setBlock(x, 64, 8, Block::REDSTONE_WIRE->id, 0);
 }

 // Trigger neighbor update from the torch to wire 1
 world.neighborUpdate(1, 64, 8, Block::LIT_REDSTONE_TORCH->id);

 // Java BlockRedstoneWire decay: wire 1 has strength 15, wire 15 has strength 1, wire 16 has strength 0
 for(int i = 1; i <= 15; ++i) {
  const int expectedPower = 16 - i;
  const int meta = world.getBlockMeta(i, 64, 8);
  EXPECT_EQ(meta, expectedPower) << "Wire at distance " << i << " expected power " << expectedPower << " but got " << meta;
 }
 EXPECT_EQ(world.getBlockMeta(16, 64, 8), 0) << "Wire at distance 16 should be completely unpowered";

 // Breaking the torch causes the entire wire line to power down to 0
 world.setBlock(0, 64, 8, 0);
 for(int x = 1; x <= 16; ++x) {
  EXPECT_EQ(world.getBlockMeta(x, 64, 8), 0) << "Wire at x=" << x << " should decay to 0 after torch removed";
 }
}

TEST(RedstoneLogicTest, WireBranchingPropagatesDecayToAllBranches) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Torch at (8, 64, 6)
 world.setBlock(8, 64, 6, Block::LIT_REDSTONE_TORCH->id, 5);

 // Wire line south: (8, 64, 7) and (8, 64, 8)
 world.setBlock(8, 64, 7, Block::REDSTONE_WIRE->id, 0);
 world.setBlock(8, 64, 8, Block::REDSTONE_WIRE->id, 0);

 // T-branch at (8, 64, 8): east to (9, 64, 8), west to (7, 64, 8)
 world.setBlock(9, 64, 8, Block::REDSTONE_WIRE->id, 0);
 world.setBlock(7, 64, 8, Block::REDSTONE_WIRE->id, 0);

 world.neighborUpdate(8, 64, 7, Block::LIT_REDSTONE_TORCH->id);

 EXPECT_EQ(world.getBlockMeta(8, 64, 7), 15);
 EXPECT_EQ(world.getBlockMeta(8, 64, 8), 14);
 EXPECT_EQ(world.getBlockMeta(9, 64, 8), 13);
 EXPECT_EQ(world.getBlockMeta(7, 64, 8), 13);
}

TEST(RedstoneLogicTest, WireVerticalTransmissionStepsUpAndCutBySolidBlock) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 6, 12, 10, 63);

 // Step: block at (9, 64, 8) is solid stone
 world.setBlock(9, 64, 8, Block::STONE->id);

 // Torch at (7, 64, 8)
 world.setBlock(7, 64, 8, Block::LIT_REDSTONE_TORCH->id, 5);

 // Lower wire at (8, 64, 8)
 world.setBlock(8, 64, 8, Block::REDSTONE_WIRE->id, 0);
 // Higher wire on top of step at (9, 65, 8)
 world.setBlock(9, 65, 8, Block::REDSTONE_WIRE->id, 0);

 world.neighborUpdate(8, 64, 8, Block::LIT_REDSTONE_TORCH->id);

 EXPECT_EQ(world.getBlockMeta(8, 64, 8), 15);
 EXPECT_EQ(world.getBlockMeta(9, 65, 8), 14);

 // Now cut the connection by placing a solid stone block directly above the lower wire at (8, 65, 8)
 world.setBlock(8, 65, 8, Block::STONE->id);

 // The upper wire should lose power from the lower wire
 EXPECT_EQ(world.getBlockMeta(9, 65, 8), 0);
}

TEST(RedstoneLogicTest, TorchInverterTogglesWithTwoTickDelay) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Inverter setup:
 // Stone block at (8, 64, 8)
 world.setBlock(8, 64, 8, Block::STONE->id);
 // Lit redstone torch on top of the stone block at (8, 65, 8)
 world.setBlock(8, 65, 8, Block::LIT_REDSTONE_TORCH->id, 5);

 // Torch should be initially lit
 EXPECT_EQ(world.getBlockId(8, 65, 8), Block::LIT_REDSTONE_TORCH->id);

 // Place a lever on the floor next to stone block at (7, 64, 8) pointing into it, or wire at (7, 64, 8)
 // Powered wire at (7, 64, 8) with power 15
 world.setBlock(6, 64, 8, Block::LIT_REDSTONE_TORCH->id, 5);
 world.setBlock(7, 64, 8, Block::REDSTONE_WIRE->id, 0);
 world.neighborUpdate(7, 64, 8, Block::LIT_REDSTONE_TORCH->id);
 ASSERT_EQ(world.getBlockMeta(7, 64, 8), 15);

 // The stone block at (8, 64, 8) is now powered by wire at (7, 64, 8).
 // Redstone torch has tickRate = 2 game ticks (1 redstone tick).
 // Tick 1: Torch should still be lit.
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 65, 8), Block::LIT_REDSTONE_TORCH->id);

 // Tick 2: Torch should now unpower and become unlit REDSTONE_TORCH (id 75).
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 65, 8), Block::REDSTONE_TORCH->id);

 // Now remove power to the inverter block
 world.setBlock(6, 64, 8, 0);
 world.setBlock(7, 64, 8, 0);

 // Tick 1 after power cut: still unlit
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 65, 8), Block::REDSTONE_TORCH->id);

 // Tick 2 after power cut: torch turns back on (LIT_REDSTONE_TORCH)
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 65, 8), Block::LIT_REDSTONE_TORCH->id);
}

TEST(RedstoneLogicTest, TorchDirectPowerUpwardVersusWeakHorizontal) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Redstone torch at (8, 64, 8) on top of stone at (8, 63, 8)
 world.setBlock(8, 64, 8, Block::LIT_REDSTONE_TORCH->id, 5);

 // Solid stone block directly above torch at (8, 65, 8)
 world.setBlock(8, 65, 8, Block::STONE->id);

 // Solid stone block horizontally adjacent to torch at (9, 64, 8)
 world.setBlock(9, 64, 8, Block::STONE->id);

 // Wire placed next to top block at (8, 66, 8) or on adjacent block (8, 65, 9)
 world.setBlock(8, 64, 9, Block::STONE->id);
 world.setBlock(8, 65, 9, Block::REDSTONE_WIRE->id, 0);

 // Wire adjacent to horizontal stone block at (10, 64, 8)
 world.setBlock(10, 64, 8, Block::REDSTONE_WIRE->id, 0);

 // Block above torch receives strong power (canTransferPower == true)
 EXPECT_TRUE(world.canTransferPower(8, 65, 8));

 // Horizontal block next to torch does NOT receive strong power (canTransferPower == false)
 EXPECT_FALSE(world.canTransferPower(9, 64, 8));

 // Wire adjacent to strongly powered block receives power 15
 world.neighborUpdate(8, 65, 9, Block::STONE->id);
 EXPECT_EQ(world.getBlockMeta(8, 65, 9), 15);

 // Wire adjacent to weakly powered horizontal block receives NO power (0)
 world.neighborUpdate(10, 64, 8, Block::STONE->id);
 EXPECT_EQ(world.getBlockMeta(10, 64, 8), 0);
}

TEST(RedstoneLogicTest, RepeaterDelaySettingsAndOneWayConduction) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Setup repeater at (8, 64, 8) facing North/-Z (meta 0: input at z+1, output at z-1)
 // Default delay is 1 redstone tick (2 game ticks)
 world.setBlock(8, 64, 8, Block::REPEATER->id, 0);

 // Input wire at (8, 64, 9)
 world.setBlock(8, 64, 9, Block::REDSTONE_WIRE->id, 0);
 // Output wire at (8, 64, 7)
 world.setBlock(8, 64, 7, Block::REDSTONE_WIRE->id, 0);

 // Initially output wire is unpowered
 EXPECT_EQ(world.getBlockMeta(8, 64, 7), 0);

 // Power input wire via torch at (8, 64, 10)
 world.setBlock(8, 64, 10, Block::LIT_REDSTONE_TORCH->id, 5);
 world.neighborUpdate(8, 64, 9, Block::LIT_REDSTONE_TORCH->id);
 ASSERT_EQ(world.getBlockMeta(8, 64, 9), 15);

 // Tick 1: repeater has not switched yet
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::REPEATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 7), 0);

 // Tick 2: repeater switches to POWERED_REPEATER and output wire receives power 15
 world.tick();
 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::POWERED_REPEATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 7), 15);

 // Now test one-way diode: unpower everything, then power ONLY the output side (8, 64, 7)
 world.setBlock(8, 64, 10, 0);
 world.setBlock(8, 64, 9, 0);
 for(int t = 0; t < 5; ++t) {
  world.tick();
 }
 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::REPEATER->id);

 // Power output side at (8, 64, 6)
 world.setBlock(8, 64, 9, Block::REDSTONE_WIRE->id, 0);
 world.setBlock(8, 64, 6, Block::LIT_REDSTONE_TORCH->id, 5);
 world.neighborUpdate(8, 64, 7, Block::LIT_REDSTONE_TORCH->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 7), 15);

 // Repeater should remain unpowered idle, and input wire at (8, 64, 9) stays 0
 for(int t = 0; t < 4; ++t) {
  world.tick();
 }
 EXPECT_EQ(world.getBlockId(8, 64, 8), Block::REPEATER->id);
 EXPECT_EQ(world.getBlockMeta(8, 64, 9), 0);
}

TEST(RedstoneLogicTest, RepeaterCyclesDelayOnInteraction) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Place repeater facing North (meta 0, delay index 0)
 world.setBlock(8, 64, 8, Block::REPEATER->id, 0);
 EXPECT_EQ((world.getBlockMeta(8, 64, 8) & 0xC) >> 2, 0);

 // onUse cycles delay: 0 -> 1 -> 2 -> 3 -> 0
 Block::REPEATER->onUse(&world, 8, 64, 8, nullptr);
 EXPECT_EQ((world.getBlockMeta(8, 64, 8) & 0xC) >> 2, 1);

 Block::REPEATER->onUse(&world, 8, 64, 8, nullptr);
 EXPECT_EQ((world.getBlockMeta(8, 64, 8) & 0xC) >> 2, 2);

 Block::REPEATER->onUse(&world, 8, 64, 8, nullptr);
 EXPECT_EQ((world.getBlockMeta(8, 64, 8) & 0xC) >> 2, 3);

 Block::REPEATER->onUse(&world, 8, 64, 8, nullptr);
 EXPECT_EQ((world.getBlockMeta(8, 64, 8) & 0xC) >> 2, 0);
}

TEST(RedstoneLogicTest, PistonQuasiConnectivityDetectsPowerFromAbove) {
 ServerEventFixture fixture;
 auto& world = fixture.world;
 loadChunksAround(fixture, -1, 2);
 buildStonePlatform(world, 5, 5, 12, 12, 63);

 // Piston at (8, 64, 8) facing UP (facing 1)
 ASSERT_NE(Block::PISTON, nullptr);
 world.setBlock(8, 64, 8, Block::PISTON->id, 1);
 EXPECT_FALSE(block::PistonBlock::isExtended(world.getBlockMeta(8, 64, 8)));

 // Solid stone block at (8, 66, 8) (2 blocks above the piston)
 world.setBlock(8, 66, 8, Block::STONE->id);
 // Strongly power that stone block using a lever placed on top of it at (8, 67, 8)
 // meta 5 + 8 = attached to top face of block below, powered ON
 world.setBlock(8, 67, 8, Block::LEVER->id, 5 + 8);

 // In Java Beta 1.7.3 BlockPistonBase.java:146:
 // if(world.isBlockIndirectlyProvidingPowerTo(i, j + 2, k, 1)) return true;
 EXPECT_TRUE(world.isEmittingRedstonePowerInDirection(8, 66, 8, 1));

 // Piston updates upon neighbor update and extends due to quasi-connectivity
 world.neighborUpdate(8, 64, 8, 0);
 EXPECT_TRUE(block::PistonBlock::isExtended(world.getBlockMeta(8, 64, 8)));

 // Turning off the lever unpowers the block and updates the piston to retract
 world.setBlock(8, 67, 8, Block::LEVER->id, 5); // unpowered lever
 EXPECT_FALSE(world.isEmittingRedstonePowerInDirection(8, 66, 8, 1));
 world.neighborUpdate(8, 64, 8, 0);
 EXPECT_FALSE(block::PistonBlock::isExtended(world.getBlockMeta(8, 64, 8)));
}

} // namespace net::minecraft::test
