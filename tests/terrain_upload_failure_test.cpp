#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <map>
#include <span>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#define private public
#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#undef private
namespace net::minecraft::test {
namespace {
void APIENTRY ignoreUpload(unsigned, intptr_t, intptr_t, const void*) {}
}
TEST(TerrainUploadFailure, FailedReplacementPreservesOldAllocationAndCpuVertices) {
 using namespace client::render;
 using namespace client::render::chunk;
 TerrainRegion region(0, 0, 0);
 auto& arena = region.layers_[0].blocks.emplace_back();
 arena.handle = 123;
 arena.capacityVertices = 64;
 arena.liveVertices = 64;
 TerrainAllocation allocation{0, 64, 64};
 std::vector<TessellatorVertex> vertices(128);
 vertices[0].x = 42;
 const auto oldGen = client::gl::GLCore::genBuffers;
 const auto oldUpload = client::gl::GLCore::bufferSubData;
 client::gl::GLCore::genBuffers = nullptr;
 client::gl::GLCore::bufferSubData = &ignoreUpload;
 const bool success = region.upload(0, allocation, vertices);
 client::gl::GLCore::genBuffers = oldGen;
 client::gl::GLCore::bufferSubData = oldUpload;
 EXPECT_FALSE(success);
 EXPECT_EQ(allocation.capacityVertices, 64u);
 EXPECT_EQ(allocation.vertexCount, 64);
 EXPECT_EQ(allocation.firstVertex, 0u);
 EXPECT_EQ(arena.liveVertices, 64u);
 EXPECT_EQ(arena.handle, 123u);
 EXPECT_EQ(vertices[0].x, 42);
 arena.handle = 0;
}
TEST(TerrainArena, GrowthAddsBlocksWithoutMovingLiveAllocations) {
 using namespace client::render;
 using namespace client::render::chunk;
 if(glfwInit() == GLFW_FALSE) GTEST_SKIP() << "no GLFW";
 glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 struct WindowScope {
  GLFWwindow* window = glfwCreateWindow(32, 32, "terrain-arena-test", nullptr, nullptr);
  ~WindowScope() {
   core::releaseGlResources();
   glfwMakeContextCurrent(nullptr);
   if(window != nullptr) glfwDestroyWindow(window);
  }
 } context;
 if(context.window == nullptr) GTEST_SKIP() << "no GL context";
 glfwMakeContextCurrent(context.window);
 client::gl::GLCore::ensureLoaded();
 TerrainRegion region(0, 0, 0);
 TerrainAllocation first;
 TerrainAllocation second;
 std::vector<TessellatorVertex> small(1024);
 std::vector<TessellatorVertex> large(8192);
 ASSERT_TRUE(region.upload(0, first, small));
 const std::size_t firstVertex = first.firstVertex;
 const std::size_t firstBlock = first.block;
 const unsigned firstHandle = region.layers_[0].blocks[firstBlock].handle;
 ASSERT_TRUE(region.upload(0, second, large));
 EXPECT_NE(second.block, first.block);
 EXPECT_EQ(first.firstVertex, firstVertex);
 EXPECT_EQ(first.block, firstBlock);
 EXPECT_EQ(region.layers_[0].blocks[firstBlock].handle, firstHandle);
 const std::size_t bothBlocks = region.gpuBytes();
 region.release(0, first);
 EXPECT_EQ(region.layers_[0].blocks[firstBlock].handle, 0u);
 EXPECT_LT(region.gpuBytes(), bothBlocks);
 TerrainAllocation reused;
 ASSERT_TRUE(region.upload(0, reused, small));
 EXPECT_EQ(region.layers_[0].blocks.size(), 2u);
 region.release(0, reused);
 region.release(0, second);
 EXPECT_EQ(region.gpuBytes(), 0u);
}
}
