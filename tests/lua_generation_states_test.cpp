#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include "net/minecraft/mod/events/ClientEvents.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"

namespace {
using namespace net::minecraft;
namespace runtime = mod::runtime;
namespace lua = mod::lua;
namespace fs = std::filesystem;

class GenerationEntryRuntime {
 public:
 GenerationEntryRuntime(std::string_view mainScript, std::string_view generationScript) {
  root_ = fs::temp_directory_path() / ("minecraft_generation_states_" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  const auto target = root_ / "mods" / "gen_probe";
  fs::create_directories(target / "scripts");
  std::ofstream(target / "mod.json") << R"({"id":"gen_probe","version":"1.0.0","enabled":true,"side":"both",)"
                                        R"("entry":"scripts/main.lua","generation_entry":"scripts/generation.lua"})";
  std::ofstream(target / "scripts/main.lua") << mainScript;
  std::ofstream(target / "scripts/generation.lua") << generationScript;
  auto& host = runtime::host();
  host.shutdown();
  host.setRuntimeSide(runtime::ModRuntimeSide::Client);
  host.setPackageLoadingEnabled(true);
  host.initialize(root_);
 }
 ~GenerationEntryRuntime() {
  runtime::host().shutdown();
  std::error_code error;
  fs::remove_all(root_, error);
 }
 [[nodiscard]] runtime::ModHost::LoadedLuaMod* mod() const {
  const auto& mods = runtime::host().loadedMods();
  return mods.size() == 1 ? mods.front().get() : nullptr;
 }
 [[nodiscard]] std::optional<std::string> shared(const std::string& key) const {
  runtime::ModHost::LoadedLuaMod* owner = mod();
  const std::lock_guard lock(owner->sharedMutex);
  const auto it = owner->shared.find(key);
  if(it == owner->shared.end()) return std::nullopt;
  return it->second;
 }
 void share(const std::string& key, const std::string& value) const {
  runtime::ModHost::LoadedLuaMod* owner = mod();
  const std::lock_guard lock(owner->sharedMutex);
  owner->shared[key] = value;
  owner->sharedOrder.push_back(key);
 }
 [[nodiscard]] bool waitFor(const std::string& key) const {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(std::chrono::steady_clock::now() < deadline) {
   if(shared(key).has_value()) return true;
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
 }

 private:
 fs::path root_;
};

constexpr std::string_view kBlockingGeneration = R"lua(
minecraft.on("chunk_generation", { stage = "terrain", moment = "before" }, function(event)
  event.cancel_vanilla = true
  minecraft.shared.put("busy:" .. event.chunk_x, "1")
  local deadline = os.clock() + 10
  while minecraft.shared.get("release") == nil and os.clock() < deadline do end
  event.chunk:set_block(0, 100, 0, 45)
  return event
end)
)lua";

int generatedBlock(World& world, int chunkX) {
 OverworldChunkGenerator generator(&world, 42);
 const Chunk chunk = generator.loadChunk(&generator, chunkX, 0);
 return Generator::rawBlock(chunk, 0, 100, 0);
}
} // namespace

TEST(LuaGenerationStates, WorkerGenerationLeavesTheMainLuaStateFree) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationEntryRuntime runtime(R"lua(
ticks = 0
minecraft.on("client_tick", {}, function(event) ticks = ticks + 1 return event end)
)lua",
                                kBlockingGeneration);
 ASSERT_NE(runtime.mod(), nullptr);
 World world("generation-states-free", 42);
 int block = -1;
 std::thread worker([&] { block = generatedBlock(world, 0); });
 ASSERT_TRUE(runtime.waitFor("busy:0"));
 {
  std::unique_lock lock(runtime.mod()->stateMutex, std::try_to_lock);
  EXPECT_TRUE(lock.owns_lock());
 }
 mod::ClientTickEvent tick;
 runtime::luaHookClientTick(tick);
 runtime.share("release", "1");
 worker.join();
 EXPECT_EQ(block, 45);
}

TEST(LuaGenerationStates, WorkersRunTheSameModsGenerationConcurrently) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationEntryRuntime runtime("", kBlockingGeneration);
 ASSERT_NE(runtime.mod(), nullptr);
 World world("generation-states-parallel", 42);
 int first = -1;
 int second = -1;
 std::thread a([&] { first = generatedBlock(world, 0); });
 std::thread b([&] { second = generatedBlock(world, 1); });
 const bool bothInside = runtime.waitFor("busy:0") && runtime.waitFor("busy:1");
 runtime.share("release", "1");
 a.join();
 b.join();
 EXPECT_TRUE(bothInside);
 EXPECT_EQ(first, 45);
 EXPECT_EQ(second, 45);
}

TEST(LuaGenerationStates, WorldOpenOptionsReachNewAndExistingGenerationStates) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationEntryRuntime runtime("", R"lua(
local block = 0
minecraft.on("world_open", {}, function(event)
  block = tonumber(event.options["probe:block"] or "0")
  return event
end)
minecraft.on("chunk_generation", { stage = "terrain", moment = "before" }, function(event)
  event.cancel_vanilla = true
  event.chunk:set_block(0, 100, 0, block)
  return event
end)
)lua");
 ASSERT_NE(runtime.mod(), nullptr);
 World world("generation-states-options", 42);
 std::unordered_map<std::string, std::string> options{{"probe:block", "45"}};
 std::string name = "options";
 mod::WorldOpenEvent firstOpen{&name, true, &options};
 runtime::luaHookWorldOpen(firstOpen);
 EXPECT_EQ(generatedBlock(world, 0), 45);
 int fresh = -1;
 std::thread worker([&] { fresh = generatedBlock(world, 0); });
 worker.join();
 EXPECT_EQ(fresh, 45);
 options["probe:block"] = "46";
 mod::WorldOpenEvent secondOpen{&name, false, &options};
 runtime::luaHookWorldOpen(secondOpen);
 EXPECT_EQ(generatedBlock(world, 0), 46);
}

TEST(LuaGenerationStates, BrokenGenerationEntryFailsTheModLoad) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 GenerationEntryRuntime runtime("", "this is not lua");
 EXPECT_EQ(runtime.mod(), nullptr);
 const runtime::ModHost& host = runtime::host();
 const runtime::ModPackage* package = host.findPackageMod("gen_probe");
 ASSERT_NE(package, nullptr);
 EXPECT_FALSE(package->active);
 EXPECT_FALSE(package->error.empty());
}
