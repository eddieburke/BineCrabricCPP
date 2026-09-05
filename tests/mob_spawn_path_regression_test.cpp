#include <algorithm>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/DoorBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include "net/minecraft/entity/ai/pathing/PathMinHeap.hpp"
#include "net/minecraft/entity/ai/pathing/PathNode.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/NaturalSpawner.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::test {
namespace {

using entity::Entity;
using entity::ai::pathing::Path;
using entity::ai::pathing::PathMinHeap;
using entity::ai::pathing::PathNode;
using entity::ai::pathing::PathNodeNavigator;

// ---------------------------------------------------------------------------
// Scripted BlockView: in-memory blocks over an optional infinite stone floor.
// Mirrors WorldRegion semantics for shouldSuffocate/getMaterial so path
// results match production lookups, while letting tests script walls, lava,
// water, doors and pits. Counts block fetches as an optimization budget guard.
// ---------------------------------------------------------------------------
class ScriptedBlockView : public BlockView {
 public:
  void setCell(int x, int y, int z, int blockId, int meta = 0) {
    cells_[std::make_tuple(x, y, z)] = std::make_pair(blockId, meta);
  }
  void setFloor(int groundY) {
    hasFloor_ = true;
    groundY_ = groundY;
  }

  [[nodiscard]] int getBlockId(int x, int y, int z) const override {
    ++blockIdCalls_;
    auto it = cells_.find(std::make_tuple(x, y, z));
    if (it != cells_.end()) {
      return it->second.first;
    }
    if (hasFloor_ && y <= groundY_) {
      return block::Block::STONE != nullptr ? block::Block::STONE->id : 1;
    }
    return 0;
  }
  block::entity::BlockEntity* getBlockEntity(int, int, int) override {
    return nullptr;
  }
  [[nodiscard]] float getNaturalBrightness(int, int, int, int) const override {
    return 1.0f;
  }
  [[nodiscard]] float getLightBrightness(int, int, int) const override {
    return 1.0f;
  }
  [[nodiscard]] int getBlockMeta(int x, int y, int z) const override {
    ++blockMetaCalls_;
    auto it = cells_.find(std::make_tuple(x, y, z));
    return it != cells_.end() ? it->second.second : 0;
  }
  [[nodiscard]] block::material::Material& getMaterial(int x, int y, int z) const override {
    const int id = getBlockId(x, y, z);
    if (id > 0 && id < block::Block::BLOCK_COUNT && block::Block::BLOCKS[id] != nullptr) {
      return block::Block::BLOCKS[id]->material;
    }
    return block::material::Material::AIR;
  }
  [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override {
    const int id = getBlockId(x, y, z);
    block::Block* block = (id > 0 && id < block::Block::BLOCK_COUNT) ? block::Block::BLOCKS[id] : nullptr;
    return block != nullptr && block->isOpaque();
  }
  [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override {
    const int id = getBlockId(x, y, z);
    block::Block* block = (id > 0 && id < block::Block::BLOCK_COUNT) ? block::Block::BLOCKS[id] : nullptr;
    return block != nullptr && block->material.blocksMovement() && block->isFullCube();
  }
  BiomeSource* getBiomeSource() const override {
    return nullptr;
  }

  mutable int blockIdCalls_ = 0;
  mutable int blockMetaCalls_ = 0;

 private:
  std::map<std::tuple<int, int, int>, std::pair<int, int>> cells_;
  bool hasFloor_ = false;
  int groundY_ = 0;
};

Entity makeWalker(double x, double y, double z) {
  Entity entity;
  entity.width = 0.6f;
  entity.height = 1.8f;
  entity.setPosition(x, y, z);
  return entity;
}

using NodeTuple = std::tuple<int, int, int>;

// Drain a path through the same public API mobs use (getNodePosition/next),
// recovering integer node coords by inverting the width offset.
std::vector<NodeTuple> collectNodes(Path path, const Entity& walker) {
  std::vector<NodeTuple> out;
  const double offset = static_cast<double>(static_cast<int>(walker.width + 1.0f)) * 0.5;
  while (!path.isFinished()) {
    const Vec3d pos = path.getNodePosition(walker);
    out.emplace_back(static_cast<int>(std::lround(pos.x - offset)),
                     static_cast<int>(std::lround(pos.y)),
                     static_cast<int>(std::lround(pos.z - offset)));
    path.next();
  }
  return out;
}

std::string nodesToString(const std::vector<NodeTuple>& nodes) {
  std::string out;
  for (const auto& node : nodes) {
    out += "(" + std::to_string(std::get<0>(node)) + "," + std::to_string(std::get<1>(node)) + "," +
           std::to_string(std::get<2>(node)) + ") ";
  }
  return out;
}

int stoneId() {
  return block::Block::STONE != nullptr ? block::Block::STONE->id : 1;
}
int doorId() {
  return block::Block::DOOR != nullptr ? block::Block::DOOR->id : 64;
}
int waterId() {
  return block::Block::WATER != nullptr ? block::Block::WATER->id : 9;
}
int lavaId() {
  return block::Block::LAVA != nullptr ? block::Block::LAVA->id : 11;
}

class MonsterProbe : public entity::mob::MonsterEntity {
 public:
  using MonsterEntity::MonsterEntity;
  using MonsterEntity::getPathfindingFavor;
};

}  // namespace

// --- PathNode / heap parity with MCP originals (PathPoint.java / Path.java) ---

TEST(PathRegression, NodeHashMatchesMcpFormula) {
  // PathPoint.func_22203_a: (y&255)|((x&32767)<<8)|((z&32767)<<24)|(x<0?MIN)|(z<0?32768)
  EXPECT_EQ(PathNode::hash(0, 0, 0), 0);
  EXPECT_EQ(PathNode::hash(1, 2, 3), (2 | (1 << 8) | (3 << 24)));
  // x=-1 keeps its low 15 bits shifted, plus the sign bit (same as Java int math).
  EXPECT_EQ(PathNode::hash(-1, 0, 0), (0x7FFF << 8) | static_cast<int>(0x80000000U));
  // (z&32767)<<24 overflows 32 bits exactly like Java's int shift: 0xFF000000, plus bit 15.
  EXPECT_EQ(PathNode::hash(0, 0, -1), static_cast<int>(0xFF008000U));
  // y wraps to 8 bits exactly like Java
  EXPECT_EQ(PathNode::hash(0, 256, 0), 0);
}

TEST(PathRegression, HeapPopsLightestFirst) {
  PathNode a(0, 0, 0);
  PathNode b(1, 0, 0);
  PathNode c(2, 0, 0);
  a.heapWeight = 5.0f;
  b.heapWeight = 1.0f;
  c.heapWeight = 3.0f;
  PathMinHeap heap;
  heap.push(&a);
  heap.push(&b);
  heap.push(&c);
  EXPECT_EQ(heap.pop(), &b);
  EXPECT_EQ(heap.pop(), &c);
  EXPECT_EQ(heap.pop(), &a);
  EXPECT_TRUE(heap.isEmpty());
}

// --- Golden straight path on flat ground (1:1 guard) ---

TEST(PathRegression, FlatGroundFindsStraightGoldenPath) {
  ScriptedBlockView view;
  view.setFloor(0);
  PathNodeNavigator nav(&view);
  Entity walker = makeWalker(0.5, 1.0, 0.5);
  // int overload adds +0.5 like EntityCreature wander calls: end node (6,1,0).
  Path path = nav.findPath(&walker, 6, 1, 0, 10.0f);
  EXPECT_GT(path.length, 1);
  ASSERT_EQ(path.length, 7);
  const std::vector<NodeTuple> nodes = collectNodes(std::move(path), walker);
  ASSERT_EQ(static_cast<int>(nodes.size()), 7) << nodesToString(nodes);
  for (int i = 0; i < 7; ++i) {
    EXPECT_EQ(std::get<0>(nodes[static_cast<std::size_t>(i)]), i) << nodesToString(nodes);
    EXPECT_EQ(std::get<1>(nodes[static_cast<std::size_t>(i)]), 1) << nodesToString(nodes);
    EXPECT_EQ(std::get<2>(nodes[static_cast<std::size_t>(i)]), 0) << nodesToString(nodes);
  }
}

TEST(PathRegression, WallForcesDetourWithoutEnteringWall) {
  ScriptedBlockView view;
  view.setFloor(0);
  const int stone = stoneId();
  for (int z = -2; z <= 2; ++z) {
    for (int y = 1; y <= 2; ++y) {
      view.setCell(3, y, z, stone);
    }
  }
  PathNodeNavigator nav(&view);
  Entity walker = makeWalker(0.5, 1.0, 0.5);
  Path path = nav.findPath(&walker, 6, 1, 0, 16.0f);
  EXPECT_GT(path.length, 1);
  const std::vector<NodeTuple> nodes = collectNodes(std::move(path), walker);
  ASSERT_GT(nodes.size(), 0U) << nodesToString(nodes);
  for (const auto& node : nodes) {
    const bool insideWall =
        (std::get<0>(node) == 3 && std::get<2>(node) >= -2 && std::get<2>(node) <= 2 &&
         std::get<1>(node) >= 1 && std::get<1>(node) <= 2);
    EXPECT_FALSE(insideWall) << "node inside wall: " << nodesToString(nodes);
  }
  // Detour must be longer than the straight 7-node golden path.
  EXPECT_GT(nodes.size(), 7U) << nodesToString(nodes);
  EXPECT_EQ(nodes.back(), NodeTuple(6, 1, 0)) << nodesToString(nodes);
}

TEST(PathRegression, LavaVetoesDescentButWaterAllowsStepOver) {
  const int water = waterId();
  const int lava = lavaId();
  // Water strip: vanilla returns -1 (no veto) so the path steps up and over.
  {
    ScriptedBlockView view;
    view.setFloor(0);
    for (int x = 2; x <= 4; ++x) {
      view.setCell(x, 1, 0, water);
    }
    PathNodeNavigator nav(&view);
    Entity walker = makeWalker(0.5, 1.0, 0.5);
    Path path = nav.findPath(&walker, 6, 1, 0, 10.0f);
    EXPECT_GT(path.length, 1);
    const std::vector<NodeTuple> nodes = collectNodes(std::move(path), walker);
    ASSERT_GT(nodes.size(), 0U) << "water: " << nodesToString(nodes);
    for (const auto& node : nodes) {
      if (std::get<0>(node) >= 2 && std::get<0>(node) <= 4 && std::get<2>(node) == 0) {
        EXPECT_EQ(std::get<1>(node), 2) << "water crossing must ride above: " << nodesToString(nodes);
      }
    }
  }
  // Lava strip: vanilla returns -2 (veto) so the path must detour around.
  {
    ScriptedBlockView view;
    view.setFloor(0);
    for (int x = 2; x <= 4; ++x) {
      view.setCell(x, 1, 0, lava);
    }
    PathNodeNavigator nav(&view);
    Entity walker = makeWalker(0.5, 1.0, 0.5);
    Path path = nav.findPath(&walker, 6, 1, 0, 16.0f);
    EXPECT_GT(path.length, 1);
    const std::vector<NodeTuple> nodes = collectNodes(std::move(path), walker);
    ASSERT_GT(nodes.size(), 0U) << "lava: " << nodesToString(nodes);
    for (const auto& node : nodes) {
      const bool aboveLava =
          (std::get<0>(node) >= 2 && std::get<0>(node) <= 4 && std::get<2>(node) == 0 &&
           std::get<1>(node) <= 2);
      EXPECT_FALSE(aboveLava) << "path must not cross lava: " << nodesToString(nodes);
    }
  }
}

TEST(PathRegression, SingleStepNeverExceedsVanillaClimbDropLimits) {
  // Sheer geometry: plateau (top y=4) west, lowland (top y=0) east, void gap.
  // Vanilla getSafePoint allows +1 up and at most -3 down per move.
  ScriptedBlockView view;
  for (int x = -4; x <= 2; ++x) {
    for (int z = -4; z <= 4; ++z) {
      view.setCell(x, 4, z, stoneId());
    }
  }
  for (int x = 6; x <= 12; ++x) {
    for (int z = -4; z <= 4; ++z) {
      view.setCell(x, 0, z, stoneId());
    }
  }
  PathNodeNavigator nav(&view);
  Entity walker = makeWalker(0.5, 5.0, 0.5);
  Path path = nav.findPath(&walker, 9, 1, 0, 32.0f);
  EXPECT_GT(path.length, 0);
  const std::vector<NodeTuple> nodes = collectNodes(std::move(path), walker);
  ASSERT_GT(nodes.size(), 0U) << nodesToString(nodes);
  for (std::size_t i = 1; i < nodes.size(); ++i) {
    const int dy = std::get<1>(nodes[i]) - std::get<1>(nodes[i - 1]);
    EXPECT_LE(dy, 1) << "climb limit at step " << i << ": " << nodesToString(nodes);
    EXPECT_GE(dy, -3) << "drop limit at step " << i << ": " << nodesToString(nodes);
    const int dx = std::abs(std::get<0>(nodes[i]) - std::get<0>(nodes[i - 1]));
    const int dz = std::abs(std::get<2>(nodes[i]) - std::get<2>(nodes[i - 1]));
    EXPECT_EQ(dx + dz, 1) << "4-neighbour moves only at step " << i << ": " << nodesToString(nodes);
  }
}

TEST(PathRegression, ClosedDoorBlocksButOpenDoorPasses) {
  const int door = doorId();
  ScriptedBlockView closedView;
  closedView.setFloor(0);
  closedView.setCell(3, 1, 0, door, 0);  // meta bit 4 clear = closed
  closedView.setCell(3, 2, 0, door, 0);
  ScriptedBlockView openView;
  openView.setFloor(0);
  openView.setCell(3, 1, 0, door, 4);  // DoorBlock::getOpen(meta): bit 4 set
  openView.setCell(3, 2, 0, door, 4);

  PathNodeNavigator closedNav(&closedView);
  Entity closedWalker = makeWalker(0.5, 1.0, 0.5);
  Path closedPath = closedNav.findPath(&closedWalker, 6, 1, 0, 16.0f);

  PathNodeNavigator openNav(&openView);
  Entity openWalker = makeWalker(0.5, 1.0, 0.5);
  Path openPath = openNav.findPath(&openWalker, 6, 1, 0, 16.0f);

  EXPECT_GT(openPath.length, 1);
  const std::vector<NodeTuple> openNodes = collectNodes(std::move(openPath), openWalker);
  bool openGoesThrough = false;
  for (const auto& node : openNodes) {
    if (std::get<0>(node) == 3 && std::get<2>(node) == 0 && std::get<1>(node) <= 2) {
      openGoesThrough = true;
    }
  }
  EXPECT_TRUE(openGoesThrough) << "open door must be walkable: " << nodesToString(openNodes);
  if (closedPath.length > 0) {
    const std::vector<NodeTuple> closedNodes = collectNodes(std::move(closedPath), closedWalker);
    for (const auto& node : closedNodes) {
      const bool throughDoor =
          (std::get<0>(node) == 3 && std::get<2>(node) == 0 && std::get<1>(node) <= 2);
      EXPECT_FALSE(throughDoor) << "closed door must be avoided: " << nodesToString(closedNodes);
    }
  }
}

TEST(PathRegression, BlockLookupBudgetIsRecorded) {  ScriptedBlockView view;
  view.setFloor(0);
  PathNodeNavigator nav(&view);
  Entity walker = makeWalker(0.5, 1.0, 0.5);
  Path path = nav.findPath(&walker, 6, 1, 0, 10.0f);
  EXPECT_GT(path.length, 1);
  RecordProperty("BlockIdCalls", view.blockIdCalls_);
  RecordProperty("BlockMetaCalls", view.blockMetaCalls_);
  // Generous guard: fails only on pathological blowup, tightens after optimization.
  EXPECT_LT(view.blockIdCalls_, 50000);
}

TEST(PathRegression, KeyOfIsInjectiveOverLegalCoords) {
  const int xs[] = {-40000, -32769, -32768, -32767, -1, 0, 1, 5, 32767, 32768, 32769, 40000};
  const int ys[] = {-2, -1, 0, 1, 64, 127};
  std::vector<std::uint64_t> keys;
  for (int x : xs) {
    for (int y : ys) {
      for (int z : xs) {
        keys.push_back(PathNode::keyOf(x, y, z));
        EXPECT_EQ(PathNode::keyOf(x, y, z), PathNode::keyOf(x, y, z));
      }
    }
  }
  std::sort(keys.begin(), keys.end());
  EXPECT_EQ(std::adjacent_find(keys.begin(), keys.end()), keys.end());
}

TEST(PathRegression, SameSearchTwiceGivesIdenticalNodes) {
  auto runWall = []() {
    ScriptedBlockView view;
    view.setFloor(0);
    const int stone = stoneId();
    for (int z = -2; z <= 2; ++z) {
      for (int y = 1; y <= 2; ++y) {
        view.setCell(3, y, z, stone);
      }
    }
    PathNodeNavigator nav(&view);
    Entity walker = makeWalker(0.5, 1.0, 0.5);
    Path path = nav.findPath(&walker, 6, 1, 0, 16.0f);
    return collectNodes(std::move(path), walker);
  };
  const std::vector<NodeTuple> first = runWall();
  const std::vector<NodeTuple> second = runWall();
  ASSERT_GT(first.size(), 0U);
  EXPECT_EQ(first, second);
}

// --- Mob spawn predicates (ports must agree with MCP semantics) ---

class LivingProbe : public entity::LivingEntity {
 public:
  using LivingEntity::LivingEntity;
};

// Test platform above any generated Beta terrain: floor at kFloor, open sky above.
constexpr int kFloorY = 89;

void buildPlatform(World& world) {
  for (int x = -2; x <= 2; ++x) {
    for (int z = -2; z <= 2; ++z) {
      // Return value only reports whether the block changed; assert final state instead.
      (void)world.setBlock(x, kFloorY, z, stoneId());
      for (int y = kFloorY + 1; y <= kFloorY + 21; ++y) {
        (void)world.setBlock(x, y, z, 0);
      }
    }
  }
}

TEST(SpawnRegression, AirAboveSolidGroundSpawnsInsideSolidDoesNot) {
  World world("spawn-path-regression", 12345ULL);
  // Production server worlds process events; without this, ChunkCache::getChunk
  // returns the shared empty chunk and isPosLoaded() stays false, so collision
  // queries (getEntityCollisions/canSpawn) see nothing. Mirrors ServerEventFixture.
  world.setEventProcessingEnabled(true);
  buildPlatform(world);
  ASSERT_TRUE(world.isPosLoaded(0, kFloorY + 1, 0));
  ASSERT_EQ(world.getBlockId(0, kFloorY, 0), stoneId());
  ASSERT_EQ(world.getBlockId(0, kFloorY + 1, 0), 0);
  EXPECT_TRUE(world.shouldSuffocate(0, kFloorY, 0));
  EXPECT_FALSE(world.shouldSuffocate(0, kFloorY + 1, 0));
  EXPECT_FALSE(world.getMaterial(0, kFloorY, 0).isFluid());
  EXPECT_FALSE(world.getMaterial(0, kFloorY + 1, 0).isFluid());

  LivingProbe airMob(&world);
  airMob.setPosition(0.5, static_cast<double>(kFloorY + 1), 0.5);
  EXPECT_TRUE(airMob.canSpawn());

  LivingProbe buriedMob(&world);
  buriedMob.setPosition(0.5, static_cast<double>(kFloorY), 0.5);
  EXPECT_FALSE(buriedMob.canSpawn());
}

TEST(SpawnRegression, StandingWaterFailsLandSpawnRule) {
  World world("spawn-path-regression-water", 777ULL);
  world.setEventProcessingEnabled(true);
  buildPlatform(world);
  const int water = waterId();
  (void)world.setBlock(0, kFloorY + 1, 0, water);
  ASSERT_EQ(world.getBlockId(0, kFloorY + 1, 0), water);
  EXPECT_TRUE(world.getMaterial(0, kFloorY + 1, 0).isFluid());
  LivingProbe swimmer(&world);
  swimmer.setPosition(0.5, static_cast<double>(kFloorY + 1), 0.5);
  EXPECT_FALSE(swimmer.canSpawn());
}

TEST(SpawnRegression, PerChunkLimitsMatchMcpValues) {
  LivingProbe base(nullptr);
  entity::passive::WolfEntity wolf(nullptr);
  entity::mob::GhastEntity ghast(nullptr);
  // EntityLiving.java:823 getMaxSpawnedInChunk=4, EntityWolf.java:457=8, EntityGhast.java:188=1
  EXPECT_EQ(base.getLimitPerChunk(), 4);
  EXPECT_EQ(wolf.getLimitPerChunk(), 8);
  EXPECT_EQ(ghast.getLimitPerChunk(), 1);
}

TEST(SpawnRegression, AnimalGrassFavorIsTenLikeEntityAnimal) {
  World world("spawn-path-regression-favor", 4242ULL);
  world.setEventProcessingEnabled(true);
  ASSERT_NE(block::Block::GRASS_BLOCK, nullptr);
  buildPlatform(world);
  for (int x = -2; x <= 2; ++x) {
    for (int z = -2; z <= 2; ++z) {
      (void)world.setBlock(x, kFloorY, z, block::Block::GRASS_BLOCK->id);
    }
  }
  ASSERT_EQ(world.getBlockId(0, kFloorY, 0), block::Block::GRASS_BLOCK->id);
  entity::passive::AnimalEntity animal(&world);
  animal.setPosition(0.5, static_cast<double>(kFloorY + 1), 0.5);
  EXPECT_FLOAT_EQ(animal.getPathfindingFavor(0, kFloorY + 1, 0), 10.0f);
  (void)animal.canSpawn();  // light-dependent outcome; this only asserts it never crashes
}

TEST(SpawnRegression, MonsterFavorAvoidsLightLikeEntityMob) {
  World world("spawn-path-regression-monster-favor", 999ULL);
  world.setEventProcessingEnabled(true);
  buildPlatform(world);
  MonsterProbe monster(&world);
  monster.setPosition(0.5, static_cast<double>(kFloorY + 1), 0.5);
  // EntityMob.java:83 favor = 0.5 - lightBrightness
  const float expected = 0.5f - world.getLightBrightness(0, kFloorY + 1, 0);
  EXPECT_FLOAT_EQ(monster.getPathfindingFavor(0, kFloorY + 1, 0), expected);
}

TEST(SpawnRegression, NaturalSpawnerEarlyOutsAndChunkPosBounds) {
  World world("spawn-path-regression-tick", 31415ULL);
  EXPECT_EQ(NaturalSpawner::tick(nullptr, true, true), 0);
  EXPECT_EQ(NaturalSpawner::tick(&world, false, false), 0);
  for (int i = 0; i < 50; ++i) {
    const Vec3i pos = NaturalSpawner::getRandomPosInChunk(&world, 16, -32);
    EXPECT_GE(pos.x, 16);
    EXPECT_LT(pos.x, 32);
    EXPECT_GE(pos.y, 0);
    EXPECT_LT(pos.y, 128);
    EXPECT_GE(pos.z, -32);
    EXPECT_LT(pos.z, -16);
  }
}

bool legacySpawnPos(bool waterSpawn, World& world, int x, int y, int z) {
  if (waterSpawn) {
    return world.getMaterial(x, y, z).isFluid() && !world.shouldSuffocate(x, y + 1, z);
  }
  return world.shouldSuffocate(x, y - 1, z) && !world.shouldSuffocate(x, y, z) &&
         !world.getMaterial(x, y, z).isFluid() && !world.shouldSuffocate(x, y + 1, z);
}

TEST(SpawnRegression, IsValidSpawnPosMatchesLegacyComposition) {
  World world("spawn-path-regression-fused", 5150ULL);
  world.setEventProcessingEnabled(true);
  buildPlatform(world);
  (void)world.setBlock(1, kFloorY + 1, 0, waterId());
  ASSERT_EQ(world.getBlockId(1, kFloorY + 1, 0), waterId());
  const SpawnRules land{0, 70, false, false};
  const SpawnRules water{2, 5, true, true};
  for (int x = -2; x <= 2; ++x) {
    for (int z = -2; z <= 2; ++z) {
      for (int y = kFloorY - 1; y <= kFloorY + 2; ++y) {
        EXPECT_EQ(isValidSpawnPos(land, &world, x, y, z), legacySpawnPos(false, world, x, y, z))
            << "land at (" << x << "," << y << "," << z << ")";
        EXPECT_EQ(isValidSpawnPos(water, &world, x, y, z), legacySpawnPos(true, world, x, y, z))
            << "water at (" << x << "," << y << "," << z << ")";
      }
    }
  }
  EXPECT_FALSE(isValidSpawnPos(land, nullptr, 0, kFloorY + 1, 0));
}

}  // namespace net::minecraft::test
