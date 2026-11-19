#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"

namespace {
using namespace net::minecraft;
namespace runtime = mod::runtime;
namespace lua = mod::lua;
namespace fs = std::filesystem;

class BundledProfileRuntime {
 public:
 BundledProfileRuntime() {
  root_ = fs::temp_directory_path() / ("minecraft_generation_stall_" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root_ / "mods");
  fs::copy(fs::path(MINECRAFT_TEST_SOURCE_DIR) / "mods/world_profiles", root_ / "mods/world_profiles",
           fs::copy_options::recursive | fs::copy_options::overwrite_existing);
  auto& host = runtime::host();
  host.shutdown();
  host.setRuntimeSide(runtime::ModRuntimeSide::Client);
  host.setPackageLoadingEnabled(true);
  host.initialize(root_);
 }
 ~BundledProfileRuntime() {
  runtime::host().shutdown();
  std::error_code error;
  fs::remove_all(root_, error);
 }

 private:
 fs::path root_;
};

struct StreamStats {
 double maxFrameMs = 0.0;
 double totalSeconds = 0.0;
 std::size_t loaded = 0;
 std::size_t desired = 0;
};

StreamStats streamProfile(const std::string& profile, int radius) {
 BundledProfileRuntime runtimeScope;
 mod::CreateWorldEvent creation;
 runtime::luaHookCreateWorld(creation);
 creation.options["world_profiles:dim:0"] = profile;
 mod::WorldOpenEvent open{nullptr, true, &creation.options};
 runtime::luaHookWorldOpen(open);
 World world("stall-" + profile, 42, creation.options);
 auto* cache = dynamic_cast<world::chunk::ChunkCache*>(world.getChunkSource());
 StreamStats stats;
 if(cache == nullptr) {
  return stats;
 }
 cache->setActiveRadius(radius);
 cache->prefetchChunksNear(0, 0);
 stats.desired = cache->desiredChunkCount();
 const auto start = std::chrono::steady_clock::now();
 while(cache->loadedChunkCount() < stats.desired) {
  const auto frameStart = std::chrono::steady_clock::now();
  cache->pumpChunkPublish();
  const double frameMs =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frameStart).count();
  stats.maxFrameMs = std::max(stats.maxFrameMs, frameMs);
  if(std::chrono::steady_clock::now() - start > std::chrono::seconds(120)) {
   break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
 }
 stats.totalSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
 stats.loaded = cache->loadedChunkCount();
 return stats;
}
} // namespace

TEST(LuaGenerationStall, LuaProfileTerrainDoesNotStallTheMainThread) {
 if(!lua::luaApi().ready()) GTEST_SKIP();
 util::concurrent::setMainThread();
 const StreamStats stats = streamProfile("infdev_20100415", 8);
 std::cout << "[stall] infdev_20100415 radius 8: loaded " << stats.loaded << "/" << stats.desired << " in "
           << stats.totalSeconds << " s, worst main-thread frame " << stats.maxFrameMs << " ms\n";
 EXPECT_EQ(stats.loaded, stats.desired);
 EXPECT_LT(stats.maxFrameMs, 50.0);
}
