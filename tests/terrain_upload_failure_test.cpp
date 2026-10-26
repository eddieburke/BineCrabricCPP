#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <map>
#include <span>
#include <vector>
#include "net/minecraft/client/gl/GLCore.hpp"
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
 auto& arena = region.layers_[0];
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
}
