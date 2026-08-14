#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"

namespace net::minecraft::test {
namespace {
class ShaderpackLoadPerf : public ::testing::Test {
 protected:
 static void SetUpTestSuite() {
  ASSERT_EQ(glfwInit(), GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "shader-load-perf", nullptr, nullptr);
  ASSERT_NE(window_, nullptr);
  glfwMakeContextCurrent(window_);
  client::gl::GLCore::ensureLoaded();
 }
 static void TearDownTestSuite() {
  net::minecraft::client::render::core::releaseGlResources();
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
 }
 static GLFWwindow* window_;
};
GLFWwindow* ShaderpackLoadPerf::window_ = nullptr;
} // namespace

TEST(PerfTimingTest, RegionSnapshotCreationBenchmark) {
 const std::size_t chunkCount = 9;
 std::vector<std::unique_ptr<Chunk>> chunks;
 chunks.reserve(chunkCount);
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 sources.reserve(chunkCount);

 for(int cz = -1; cz <= 1; ++cz) {
  for(int cx = -1; cx <= 1; ++cx) {
   auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
   for(int y = 0; y < 64; ++y) {
    for(int z = 0; z < 16; ++z) {
     for(int x = 0; x < 16; ++x) {
      chunk->blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = 1;
     }
    }
   }
   sources.push_back({cx, cz, chunk.get()});
   chunks.push_back(std::move(chunk));
  }
 }

 std::array<float, 16> lightTable{};
 for(std::size_t i = 0; i < 16; ++i) {
  lightTable[i] = static_cast<float>(i) / 15.0f;
 }

 const int iterations = 1000;
 const auto start = std::chrono::high_resolution_clock::now();

 for(int i = 0; i < iterations; ++i) {
  client::render::chunk::RegionSnapshot snapshot(sources, 0, lightTable, nullptr, -16, 0, -16, 31, 63, 31);
  volatile int blockId = snapshot.getBlockId(0, 10, 0);
  (void)blockId;
 }

 const auto end = std::chrono::high_resolution_clock::now();
 const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

 std::printf("[PERF_TIMING] RegionSnapshot Creation: %d iterations in %.3f ms (%.3f us/op)\n",
             iterations,
             elapsedMs,
             (elapsedMs * 1000.0) / iterations);
 std::fflush(stdout);

 EXPECT_LT(elapsedMs, 5000.0);
}

TEST(PerfTimingTest, LightingEnginePushAndDrainBenchmark) {
 std::vector<std::unique_ptr<Chunk>> chunks;
 chunks.reserve(9);
 world::light::UnifiedLightRegistry registry;
 LightingEngine engine(registry);

 for(int cz = -1; cz <= 1; ++cz) {
  for(int cx = -1; cx <= 1; ++cx) {
   auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
   engine.registerChunk(chunk.get());
   for(int y = 0; y < 64; ++y) {
    for(int z = 0; z < 16; ++z) {
     for(int x = 0; x < 16; ++x) {
      chunk->blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = (y == 0 ? 7 : 0);
     }
    }
   }
   chunks.push_back(std::move(chunk));
  }
 }

 const int iterations = 500;
 const auto start = std::chrono::high_resolution_clock::now();

 for(int i = 0; i < iterations; ++i) {
  engine.push(LightType::Block, 0, 1, 0, 15, 10, 15, true);
 }

 const auto end = std::chrono::high_resolution_clock::now();
 const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
 engine.flushStaging();
 const auto settleStart = std::chrono::high_resolution_clock::now();
 const auto deadline = settleStart + std::chrono::seconds(2);
 std::size_t drainedRegions = 0;
 while((engine.busy() || engine.hasDirtyRegions()) && std::chrono::high_resolution_clock::now() < deadline) {
  drainedRegions += engine.drainDirtyRegions(16).size();
  std::this_thread::yield();
 }
 const double settleMs =
     std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - settleStart).count();

 std::printf("[PERF_TIMING] LightingEngine Push+Settle: %d pushes in %.3f ms (%.3f us/push), settle %.3f ms, %zu regions\n",
             iterations,
             elapsedMs,
             (elapsedMs * 1000.0) / iterations,
             settleMs,
             drainedRegions);
 std::fflush(stdout);
 const bool settled = !engine.busy() && !engine.hasDirtyRegions();

 engine.stop();
 for(const auto& chunk : chunks) {
  engine.unregisterChunk(chunk.get());
 }
 EXPECT_LT(elapsedMs, 2000.0);
 EXPECT_LT(settleMs, 2000.0);
 EXPECT_TRUE(settled);
}

TEST(PerfTimingTest, MeshLightingReadinessGateBenchmark) {
 std::vector<block::entity::BlockEntity*> blockEntities;
 client::render::chunk::ChunkBuilder builder(nullptr, blockEntities, 0, 0, 0, 16);
 constexpr int iterations = 2000000;
 volatile int legacyReadyCount = 0;
 const auto legacyStart = std::chrono::high_resolution_clock::now();
 for(int i = 0; i < iterations; ++i) {
  builder.dirty = (i & 3) != 0;
  builder.meshJobInFlight = (i & 7) == 0;
  legacyReadyCount += builder.dirty && !builder.meshJobInFlight ? 1 : 0;
 }
 const double legacyMs =
     std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - legacyStart).count();
 volatile int gatedReadyCount = 0;
 const auto gatedStart = std::chrono::high_resolution_clock::now();
 for(int i = 0; i < iterations; ++i) {
  builder.dirty = (i & 3) != 0;
  builder.meshJobInFlight = (i & 7) == 0;
  builder.lightingReady = (i & 1) != 0;
  gatedReadyCount += builder.readyForMeshCapture() ? 1 : 0;
 }
 const double gatedMs =
     std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - gatedStart).count();
 std::printf("[PERF_AB] Mesh Lighting Gate: %d checks, legacy %.3f ms, gated %.3f ms, overhead %.3f ms\n",
             iterations,
             legacyMs,
             gatedMs,
             gatedMs - legacyMs);
 std::fflush(stdout);
 EXPECT_GT(legacyReadyCount, gatedReadyCount);
 EXPECT_GT(gatedReadyCount, 0);
 EXPECT_LT(gatedMs, 1000.0);
}

TEST(PerfTimingTest, ChunkPacketSerializationBenchmark) {
 ChunkDataS2CPacket packet;
 packet.x = 0;
 packet.y = 0;
 packet.z = 0;
 packet.sizeX = 15;
 packet.sizeY = 127;
 packet.sizeZ = 15;
 packet.chunkData = std::vector<std::uint8_t>(16 * 16 * 128, 1);

 const int iterations = 500;
 const auto start = std::chrono::high_resolution_clock::now();

 for(int i = 0; i < iterations; ++i) {
  packet.chunkDataSize = 0;
  packet.compressForSend();
 }

 const auto end = std::chrono::high_resolution_clock::now();
 const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

 std::printf("[PERF_TIMING] ChunkDataS2CPacket compressForSend: %d iterations in %.3f ms (%.3f us/op)\n",
             iterations,
             elapsedMs,
             (elapsedMs * 1000.0) / iterations);
 std::fflush(stdout);

 EXPECT_LT(elapsedMs, 2000.0);
}

TEST(PerfTimingTest, SourceProcessorNormalizeBenchmark) {
 const std::string shaderSource = R"(
#version 330 compatibility
#define SHADOW_MAP_SIZE 1024
#ifdef MC_NORMAL_MAP
in vec3 vaNormal;
#endif
void main() {
    gl_Position = ftransform();
}
)";

 client::render::PackDefinition pack{};
 const int iterations = 1000;
 const auto start = std::chrono::high_resolution_clock::now();

 for(int i = 0; i < iterations; ++i) {
  const std::string processed = client::render::normalizePackSource(pack, shaderSource);
  volatile std::size_t sz = processed.size();
  (void)sz;
 }

 const auto end = std::chrono::high_resolution_clock::now();
 const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

 std::printf("[PERF_TIMING] SourceProcessor normalizePackSource: %d iterations in %.3f ms (%.3f us/op)\n",
             iterations,
             elapsedMs,
             (elapsedMs * 1000.0) / iterations);
 std::fflush(stdout);

 EXPECT_LT(elapsedMs, 2000.0);
}

TEST_F(ShaderpackLoadPerf, LoadBenchmark) {
 const std::filesystem::path root =
     std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9";
 ASSERT_TRUE(std::filesystem::is_directory(root));
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(root);
 const auto readText = [&root](std::string_view path) {
  std::ifstream file(root / std::filesystem::path(path), std::ios::binary);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
 };
 const int iterations = 3;
 const auto start = std::chrono::high_resolution_clock::now();
 for(int i = 0; i < iterations; ++i) {
  client::render::PackDefinition pack;
  std::unordered_map<std::string, client::render::PackSourceOption> options;
  std::string error;
  ASSERT_TRUE(client::render::PackLoader::load(resources, readText, pack, options, error)) << error;
 }
 const double elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
 std::printf("[PERF_TIMING] PackLoader::load rethinking-voxels: %d iterations in %.3f ms (%.3f ms/op)\n",
             iterations,
             elapsedMs,
             elapsedMs / iterations);
 std::fflush(stdout);
 EXPECT_LT(elapsedMs, 60000.0);
}

} // namespace net::minecraft::test
