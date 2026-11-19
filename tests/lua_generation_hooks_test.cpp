#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

namespace {
using namespace net::minecraft;
namespace runtime = mod::runtime;
namespace lua = mod::lua;
namespace gen = world::gen;
namespace fs = std::filesystem;

class GenerationRuntime {
 public:
 explicit GenerationRuntime(std::string_view script, bool bundled = false) {
  root_ = fs::temp_directory_path() / ("minecraft_generation_hooks_" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  const auto target = root_ / "mods" / (bundled ? "world_profiles" : "probe");
  fs::create_directories(target);
  if(bundled) {
   fs::copy(fs::path(MINECRAFT_TEST_SOURCE_DIR) / "mods/world_profiles", target,
            fs::copy_options::recursive | fs::copy_options::overwrite_existing);
  } else {
   fs::create_directories(target / "scripts");
   std::ofstream(target / "mod.json") <<
       R"({"id":"probe","version":"1.0.0","enabled":true,"side":"both","entry":"scripts/main.lua"})";
   std::ofstream(target / "scripts/main.lua") << script;
  }
  auto& host = runtime::host();
  host.shutdown();
  host.setRuntimeSide(runtime::ModRuntimeSide::Client);
  host.setPackageLoadingEnabled(true);
  host.initialize(root_);
 }
 ~GenerationRuntime() {
  runtime::host().shutdown();
  std::error_code error;
  fs::remove_all(root_, error);
 }
 void execute(std::string_view source) {
  ASSERT_EQ(runtime::host().loadedMods().size(), 1U);
  auto* state = static_cast<lua_State*>(runtime::host().loadedMods().front()->state);
  auto& api = lua::luaApi();
  const int top = api.gettop(state);
  int status = api.loadbufferx(state, source.data(), source.size(), "@generation_probe", "t");
  if(status == lua::kLuaOk) status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  const std::string error = status == lua::kLuaOk ? std::string{} : lua::luaString(state, -1);
  api.settop(state, top);
  ASSERT_EQ(status, lua::kLuaOk) << error;
 }
 private:
 fs::path root_;
};

TEST(LuaGenerationHooks, OfflineSeedAndCancellationSurviveEveryListener) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
minecraft.on("chunk_generation", { moment = "before", priority = 10 }, function(event)
  assert(event.world_seed == 0x123456789abcdef)
  assert(event.dimension_id == 7 and event.chunk_x == -3 and event.chunk_z == 8)
  assert(event.has_world == false and event.mod_generation and event.has_chunk)
  assert(minecraft.world.get_time() == 0)
  event.chunk:set_block(1, 2, 3, 1)
  event.cancel_vanilla = true
end)
minecraft.on("chunk_generation", { moment = "before" }, function(event)
  assert(event.cancel_vanilla)
  event.cancel_vanilla = false
end)
minecraft.on("chunk_generation", { moment = "after" }, function(event)
  assert(event.cancel_vanilla and not event.vanilla_stage_ran)
  assert(event.chunk:get_block(1, 2, 3) == 1)
end)
)lua");
 ASSERT_EQ(runtime::host().loadedMods().size(), 1U);
 Chunk chunk(nullptr, -3, 8);
 gen::ChunkGenerationContext context;
 context.chunk = &chunk;
 context.worldSeed = 0x123456789abcdefULL;
 context.chunkX = -3;
 context.chunkZ = 8;
 context.dimensionId = 7;
 context.modGeneration = true;
 bool vanillaRan = false;
 EXPECT_FALSE(gen::runVanillaStage(gen::ChunkStage::Terrain, context, [&] { vanillaRan = true; }));
 EXPECT_FALSE(vanillaRan);
 EXPECT_EQ(Generator::rawBlock(chunk, 1, 2, 3), 1);
 EXPECT_FALSE(lua::LuaChunkContext::hasActiveChunk());
}

TEST(LuaGenerationHooks, FailedReplacementAbortsAndRestoresLuaStack) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
local fail = true
minecraft.on("chunk_generation", { moment = "before" }, function(event)
  if fail then fail = false error("broken generator") end
  event.cancel_vanilla = true
end)
)lua");
 ASSERT_EQ(runtime::host().loadedMods().size(), 1U);
 gen::ChunkGenerationContext context;
 bool vanillaRan = false;
 EXPECT_THROW(gen::runVanillaStage(gen::ChunkStage::Terrain, context, [&] { vanillaRan = true; }), std::runtime_error);
 EXPECT_FALSE(vanillaRan);
 EXPECT_FALSE(gen::runVanillaStage(gen::ChunkStage::Terrain, context, [&] { vanillaRan = true; }));
 EXPECT_FALSE(vanillaRan);
 runtime.execute("assert(1 + 1 == 2)");
}

TEST(LuaGenerationHooks, SampleUsesFullSeedAndReturnsGenerationFailures) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
minecraft.on("chunk_generation", { moment = "before", mod_generation = true }, function(event)
  assert(event.world_seed == 0x123456789abcdef, "seed truncated")
  if event.stage == "terrain" then event.chunk:fill(0, 0, 0, 15, 4, 15, 1) end
  event.cancel_vanilla = true
end)
)lua");
 runtime.execute(R"lua(
local sample, err = minecraft.world.sample(0x123456789abcdef, 0, 0,
  { radius_chunks = 1, max_side = 8, channel = "height", mod_generation = true })
assert(sample, err)
for _, height in ipairs(sample.channels.height) do assert(height == 5) end
local bad, message = minecraft.world.sample(10, 0, 0,
  { radius_chunks = 1, max_side = 8, channel = "height", mod_generation = true })
assert(bad == false and message:find("seed truncated"))
)lua");
}

TEST(LuaGenerationHooks, SpawnUsesEventWorldAndPreservesResolvedPosition) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
minecraft.on("world_spawn_search", { priority = 10 }, function(event)
  assert(event.has_world and event.dimension_id == 0)
  assert(minecraft.world.get_time() == 1234)
  event.x, event.y, event.z, event.resolved = 8, 90, -8, true
end)
minecraft.on("world_spawn_search", {}, function(event)
  event.x, event.y, event.z, event.resolved = 0, 1, 0, false
end)
)lua");
 World target("target", 42);
 World enclosing("enclosing", 13);
 target.setTime(1234);
 const lua::ModContextScope outerScope(&enclosing);
 mod::WorldSpawnSearchEvent event{&target};
 runtime::luaHookWorldSpawnSearch(event);
 EXPECT_TRUE(event.resolved);
 EXPECT_EQ(event.x, 8);
 EXPECT_EQ(event.y, 90);
 EXPECT_EQ(event.z, -8);
 EXPECT_EQ(lua::activeModWorld(), &enclosing);
}

TEST(LuaGenerationHooks, NestedChunkScopesRestoreAllStateOnException) {
 Chunk outer(nullptr, 1, 2);
 Chunk inner(nullptr, -3, 4);
 {
  const lua::LuaChunkContext::Scope outerScope(&outer, nullptr, 1, 2, lua::ChunkWriteMode::RawGeneration);
  ASSERT_TRUE(lua::LuaChunkContext::setBlock(1, 2, 3, 1));
  try {
   const lua::LuaChunkContext::Scope innerScope(&inner, nullptr, -3, 4, lua::ChunkWriteMode::RawGeneration);
   ASSERT_EQ(lua::LuaChunkContext::activeChunkX(), -3);
   ASSERT_EQ(lua::LuaChunkContext::activeChunkZ(), 4);
   ASSERT_TRUE(lua::LuaChunkContext::setBlock(1, 2, 3, 3));
   throw std::runtime_error("nested");
  } catch(const std::runtime_error&) {
  }
  EXPECT_EQ(lua::LuaChunkContext::activeChunk(), &outer);
  EXPECT_EQ(lua::LuaChunkContext::activeChunkX(), 1);
  EXPECT_EQ(lua::LuaChunkContext::activeChunkZ(), 2);
  EXPECT_EQ(lua::LuaChunkContext::writeMode(), lua::ChunkWriteMode::RawGeneration);
  EXPECT_EQ(lua::LuaChunkContext::getBlock(1, 2, 3), 1);
 }
 EXPECT_EQ(Generator::rawBlock(inner, 1, 2, 3), 3);
 EXPECT_FALSE(lua::LuaChunkContext::hasActiveChunk());
}

TEST(LuaGenerationHooks, FeatureFailureRestoresInstantFallMode) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
 fail_features = true
 minecraft.on("chunk_generation", { stage = "features" }, function(event)
   if fail_features then error("feature failed") end
   event.cancel_vanilla = true
 end)
)lua");
 World world("features", 42);
 OverworldChunkGenerator generator(&world, 42);
 block::FallingBlock::fallInstantly = false;
 EXPECT_THROW(generator.decorate(&generator, 0, 0), std::runtime_error);
 EXPECT_FALSE(block::FallingBlock::fallInstantly);
 block::FallingBlock::fallInstantly = true;
 EXPECT_THROW(generator.decorate(&generator, 0, 0), std::runtime_error);
 EXPECT_TRUE(block::FallingBlock::fallInstantly);
 block::FallingBlock::fallInstantly = false;
 auto* cache = world.getChunkSource();
 EXPECT_THROW(cache->decorate(cache, 0, 0), std::runtime_error);
 EXPECT_FALSE(world.getChunk(0, 0).terrainPopulated);
 runtime.execute("fail_features = false");
 EXPECT_NO_THROW(cache->decorate(cache, 0, 0));
 EXPECT_TRUE(world.getChunk(0, 0).terrainPopulated);
}

TEST(LuaGenerationHooks, BundledFlatlandsLoadsSavedOptionsAndFindsSpawn) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime({}, true);
 ASSERT_EQ(runtime::host().loadedMods().size(), 1U);
 mod::CreateWorldEvent creation;
 runtime::luaHookCreateWorld(creation);
 creation.options["world_profiles:dim:0"] = "flatlands";
 creation.options["world_profiles:option:0:flat_height"] = "12";
 mod::WorldOpenEvent open{nullptr, false, &creation.options};
 ASSERT_NO_THROW(runtime::luaHookWorldOpen(open));
 World world("flatlands", 42, creation.options);
 const auto& chunk = world.getChunkSource()->loadChunk(0, 0);
 EXPECT_EQ(Generator::rawBlock(chunk, 0, 0, 0), Block::BEDROCK->id);
 EXPECT_EQ(Generator::rawBlock(chunk, 0, 11, 0), Block::DIRT->id);
 EXPECT_EQ(Generator::rawBlock(chunk, 0, 12, 0), Block::GRASS_BLOCK->id);
 EXPECT_EQ(Generator::rawBlock(chunk, 0, 13, 0), 0);
 ASSERT_NO_THROW(world.initializeSpawnPoint());
 EXPECT_EQ(world.getSpawnPos().y, 13);
 mod::CreateWorldEvent nextCreation;
 runtime::luaHookCreateWorld(nextCreation);
 EXPECT_EQ(nextCreation.options.at("world_profiles:dim:0"), "default");
 EXPECT_EQ(nextCreation.options.count("world_profiles:option:0:flat_height"), 0U);
 creation.options["world_profiles:dim:0"] = "unknown";
 EXPECT_THROW(runtime::luaHookWorldOpen(open), std::runtime_error);
}

TEST(LuaGenerationHooks, BundledModernUsesLuaDensityHooksAndFindsSpawn) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime({}, true);
 ASSERT_EQ(runtime::host().loadedMods().size(), 1U);
 std::unordered_map<std::string, std::string> options{
     {"world_profiles:schema", "3"},
     {"world_profiles:dim:0", "modern"},
     {"world_profiles:dim:-1", "default"},
     {"world_profiles:dim:1", "default"},
     {"terrain_style:0", "beta"},
 };
 mod::WorldOpenEvent open{nullptr, false, &options};
 ASSERT_NO_THROW(runtime::luaHookWorldOpen(open));
 constexpr std::uint64_t kCrashSeed = 0xbb20b45fd4d95138ULL;
 World world("modern", kCrashSeed, options);
 ASSERT_NO_THROW(world.initializeSpawnPoint());
 const Vec3i spawn = world.getSpawnPos();
 EXPECT_GE(spawn.y, 65);
 EXPECT_LE(spawn.y, 104);
}

TEST(LuaGenerationHooks, WorldConstructorExceptionUnwindsLightingBeforeChunks) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime(R"lua(
minecraft.on("world_spawn_search", {}, function(event)
  for dx = -2, 2 do
    for dz = -2, 2 do
      minecraft.world.get_top_y(dx * 16, dz * 16)
    end
  end
  error("simulated spawn failure after scheduling lighting workers")
end)
)lua");
 EmptyWorldStorage storage;
 EXPECT_THROW(
     {
      World world(&storage, "ThrowingWorld", 42LL, /*deferSpawnInit=*/false);
     },
     std::runtime_error);
}

TEST(LuaGenerationHooks, UnknownEventStopsModLoading) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationRuntime runtime("minecraft.on('unknown_event', {}, function() end)");
 EXPECT_TRUE(runtime::host().loadedMods().empty());
 ASSERT_EQ(runtime::host().packageMods().size(), 1U);
 EXPECT_NE(runtime::host().packageMods().front().error.find("unknown Lua event"), std::string::npos);
}
}
