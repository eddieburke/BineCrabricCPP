#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "net/minecraft/client/debug/PerformanceCapture.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
namespace net::minecraft::test {
TEST(PerformanceCaptureTest, WaitsWarmsCapturesAndWritesStructuredReport) {
 const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
 const std::filesystem::path output =
     std::filesystem::temp_directory_path() / ("minecraft-render-capture-" + std::to_string(stamp) + ".json");
 client::debug::PerformanceCapture capture;
 client::debug::PerformanceCaptureConfig config;
 config.enabled = true;
 config.warmupFrames = 1;
 config.captureFrames = 2;
 config.output = output;
 config.shaderPack = "Rethinking Voxels";
 capture.configure(config);
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 for(int frame = 0; frame < 3; ++frame) {
  const bool profileFrame = capture.beginFrame(true, profiler);
  profiler.setEnabled(profileFrame);
  profiler.beginFrame();
  {
   client::debug::RenderProfileScope scope("shader", "composite");
  }
  profiler.endFrame();
  capture.endFrame(profiler);
 }
 EXPECT_TRUE(capture.complete());
 EXPECT_EQ(capture.capturedFrames(), 2);
 std::ifstream input(output, std::ios::binary);
 ASSERT_TRUE(input.good());
 std::ostringstream contents;
 contents << input.rdbuf();
 EXPECT_NE(contents.str().find("\"frames\": 2"), std::string::npos);
 EXPECT_NE(contents.str().find("Rethinking Voxels"), std::string::npos);
 EXPECT_NE(contents.str().find("shader/composite"), std::string::npos);
 EXPECT_NE(contents.str().find("\"contaminated\":"), std::string::npos);
 EXPECT_NE(contents.str().find("dropped_during_capture"), std::string::npos);
 profiler.setEnabled(false);
 std::error_code error;
 std::filesystem::remove(output, error);
}
} // namespace net::minecraft::test
