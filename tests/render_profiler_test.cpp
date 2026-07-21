#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
namespace {
class TestMinecraft final : public net::minecraft::client::Minecraft {
 public:
 using net::minecraft::client::Minecraft::Minecraft;
 void handleCrash(const net::minecraft::util::crash::CrashReport&) override {
 }
};
std::filesystem::path makeTempWorldRoot(const std::string& name) {
 const std::filesystem::path root = std::filesystem::temp_directory_path() / "minecraft_profiler_tests" / name;
 std::filesystem::remove_all(root);
 std::filesystem::create_directories(root);
 return root;
}
} // namespace
namespace net::minecraft::test {
TEST(RenderProfilerTest, GrabProfilerMetricsInSingleplayerWorld) {
 const std::filesystem::path root = makeTempWorldRoot("singleplayer_profiler_world");
 TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
 client.options.debugHud = true;
 client.worldSession().ownedWorldStorageMut() =
     std::make_unique<net::minecraft::AlphaWorldStorage>(root, "ProfilerWorld", true);
 client.worldSession().ownedWorldMut() =
     std::make_unique<net::minecraft::World>(client.worldSession().ownedWorldStorage(), "ProfilerWorld", 12345, false);
 client.setWorld(client.worldSession().ownedWorld(), "Loading world");
 ASSERT_NE(client.world, nullptr);
 EXPECT_FALSE(client.isWorldRemote());
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 for(int frame = 0; frame < 5; ++frame) {
  profiler.beginFrame();
  {
   const client::debug::RenderProfiler::Scope skyScope(client::debug::RenderStage::Sky);
  }
  {
   const client::debug::RenderProfiler::Scope cullScope(client::debug::RenderStage::Cull);
  }
  {
   const client::debug::RenderProfiler::Scope compileScope(client::debug::RenderStage::Compile);
  }
  {
   const client::debug::RenderProfiler::Scope solidScope(client::debug::RenderStage::SolidTerrain);
  }
  {
   const client::debug::RenderProfiler::Scope lodScope(client::debug::RenderStage::Lod);
  }
  {
   const client::debug::RenderProfiler::Scope entityScope(client::debug::RenderStage::Entities);
  }
  {
   const client::debug::RenderProfiler::Scope particleScope(client::debug::RenderStage::Particles);
  }
  {
   const client::debug::RenderProfiler::Scope translucentScope(client::debug::RenderStage::TranslucentTerrain);
  }
  {
   const client::debug::RenderProfiler::Scope cloudScope(client::debug::RenderStage::Clouds);
  }
  {
   const client::debug::RenderProfiler::Scope handScope(client::debug::RenderStage::Hand);
  }
  profiler.endFrame();
 }
 const std::vector<std::string> lines = profiler.lines();
 ASSERT_FALSE(lines.empty());
 bool foundStageHeader = false;
 bool foundSolid = false;
 bool foundEntities = false;
 bool foundCull = false;
 bool foundSky = false;
 bool foundTranslucent = false;
 bool foundParticles = false;
 bool foundCompile = false;
 bool foundHand = false;
 bool foundLod = false;
 bool foundClouds = false;
 bool foundWorld = false;
 bool foundUnmeasured = false;
 for(const std::string& line : lines) {
  if(line.rfind("stage", 0) == 0) {
   foundStageHeader = true;
  }
  if(line.rfind("solid", 0) == 0) {
   foundSolid = true;
  }
  if(line.rfind("entities", 0) == 0) {
   foundEntities = true;
  }
  if(line.rfind("cull", 0) == 0) {
   foundCull = true;
  }
  if(line.rfind("sky", 0) == 0) {
   foundSky = true;
  }
  if(line.rfind("translucent", 0) == 0) {
   foundTranslucent = true;
  }
  if(line.rfind("particles", 0) == 0) {
   foundParticles = true;
  }
  if(line.rfind("compile", 0) == 0) {
   foundCompile = true;
  }
  if(line.rfind("hand", 0) == 0) {
   foundHand = true;
  }
  if(line.rfind("lod", 0) == 0) {
   foundLod = true;
  }
  if(line.rfind("clouds", 0) == 0) {
   foundClouds = true;
  }
  if(line.rfind("world", 0) == 0) {
   foundWorld = true;
  }
  if(line.rfind("unmeasured", 0) == 0) {
   foundUnmeasured = true;
  }
 }
 EXPECT_TRUE(foundStageHeader);
 EXPECT_TRUE(foundSolid);
 EXPECT_TRUE(foundEntities);
 EXPECT_TRUE(foundCull);
 EXPECT_TRUE(foundSky);
 EXPECT_TRUE(foundTranslucent);
 EXPECT_TRUE(foundParticles);
 EXPECT_TRUE(foundCompile);
 EXPECT_TRUE(foundHand);
 EXPECT_TRUE(foundLod);
 EXPECT_TRUE(foundClouds);
 EXPECT_TRUE(foundWorld);
 EXPECT_TRUE(foundUnmeasured);
 client.setWorld(nullptr);
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, SingleplayerStartGameAndGrabProfilerMetrics) {
 TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
 client.options.debugHud = true;
 client.startGame("SingleplayerProfilerWorld", "Singleplayer World", 987654321LL);
 ASSERT_NE(client.world, nullptr);
 EXPECT_FALSE(client.isWorldRemote());
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 profiler.beginFrame();
 {
  const client::debug::RenderProfiler::Scope solidScope(client::debug::RenderStage::SolidTerrain);
 }
 {
  const client::debug::RenderProfiler::Scope entityScope(client::debug::RenderStage::Entities);
 }
 profiler.endFrame();
 const std::vector<std::string> lines = profiler.lines();
 ASSERT_FALSE(lines.empty());
 client.setWorld(nullptr);
 profiler.setEnabled(false);
}
} // namespace net::minecraft::test
