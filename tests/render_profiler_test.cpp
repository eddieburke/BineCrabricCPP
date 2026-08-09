#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/client/util/FramePipeline.hpp"
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
   {
    const client::debug::RenderProfiler::Scope presentScope(client::debug::RenderStage::Present);
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
 bool foundChunks = false;
 bool foundHand = false;
 bool foundClouds = false;
 bool foundPresent = false;
 bool foundMeasured = false;
 bool foundUnmeasured = false;
  for(const std::string& line : lines) {
   if(line.rfind("Stage", 0) == 0) {
    foundStageHeader = true;
   }
   if(line.rfind("Solid terrain", 0) == 0) {
    foundSolid = true;
   }
   if(line.rfind("Entities", 0) == 0) {
    foundEntities = true;
   }
   if(line.rfind("Frustum cull", 0) == 0) {
    foundCull = true;
   }
   if(line.rfind("Sky", 0) == 0) {
    foundSky = true;
   }
   if(line.rfind("Translucent terrain", 0) == 0) {
    foundTranslucent = true;
   }
   if(line.rfind("Particles", 0) == 0) {
    foundParticles = true;
   }
   if(line.rfind("Chunk meshes", 0) == 0) {
    foundChunks = true;
   }
   if(line.rfind("Hand", 0) == 0) {
    foundHand = true;
   }
   if(line.rfind("Clouds", 0) == 0) {
    foundClouds = true;
   }
   if(line.rfind("Present", 0) == 0) {
    foundPresent = true;
   }
   if(line.rfind("Total measured", 0) == 0) {
    foundMeasured = true;
   }
   if(line.rfind("Unmeasured", 0) == 0) {
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
 EXPECT_TRUE(foundChunks);
 EXPECT_TRUE(foundHand);
 EXPECT_TRUE(foundClouds);
 EXPECT_TRUE(foundPresent);
 EXPECT_TRUE(foundMeasured);
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
TEST(RenderProfilerTest, NestedScopesAccountSelfTimeOnce) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 for(int frame = 0; frame < 20; ++frame) {
  profiler.beginFrame();
  {
   const client::debug::RenderProfiler::Scope setup(client::debug::RenderStage::FrameSetup);
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
   {
    const client::debug::RenderProfiler::Scope hand(client::debug::RenderStage::Hand);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
  }
  profiler.endFrame();
 }
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::FrameSetup), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Hand), 0.0);
 EXPECT_LE(profiler.cpuAverageNs(client::debug::RenderStage::FrameSetup) +
               profiler.cpuAverageNs(client::debug::RenderStage::Hand),
           profiler.frameAverageNs() * 1.1);
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, FramePipelineAccountsEveryOuterPhase) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 client::util::FramePipeline pipeline;
 profiler.setEnabled(true);
 for(int frame = 0; frame < 5; ++frame) {
  profiler.beginFrame();
  pipeline.run([](client::util::FramePipeline::Phase) {
   std::this_thread::sleep_for(std::chrono::microseconds(100));
  });
  profiler.endFrame();
 }
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::FrameDrain), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Input), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Ticks), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::RenderOverhead), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Present), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Pace), 0.0);
 EXPECT_GT(profiler.cpuAverageNs(client::debug::RenderStage::Diagnostics), 0.0);
 profiler.setEnabled(false);
}
TEST(RenderProfilerTest, FrameMetricsResetAndAccumulateWithoutAllocation) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 profiler.beginFrame();
 profiler.record(client::debug::RenderMetric::EntityTraversals, 7);
 profiler.record(client::debug::RenderMetric::DrawCalls, 3);
 profiler.endFrame();
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::EntityTraversals), 7U);
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::DrawCalls), 3U);
 profiler.beginFrame();
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::EntityTraversals), 0U);
 EXPECT_EQ(profiler.frameMetric(client::debug::RenderMetric::DrawCalls), 0U);
 profiler.endFrame();
 profiler.setEnabled(false);
}
} // namespace net::minecraft::test
