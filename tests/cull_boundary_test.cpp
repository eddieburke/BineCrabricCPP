#include <gtest/gtest.h>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/culling/PlaneSet.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
namespace net::minecraft::test {
TEST(CullBoundaryTest, PlaneKeepsTouchingSection) {
 client::render::PlaneSet<1> planes;
 planes.add({1.0f, 0.0f, 0.0f, 0.0f});
 EXPECT_TRUE(planes.intersectsAabb(-16.0f, 0.0f, 0.0f, 0.0f, 16.0f, 16.0f));
 EXPECT_FALSE(planes.intersectsAabb(-16.0f, 0.0f, 0.0f, -0.01f, 16.0f, 16.0f));
}
TEST(CullBoundaryTest, EntityNearPlaneDoesNotCpuCull) {
 client::render::FrameRenderCamera camera;
 camera.projectionX = 1.0f;
 camera.projectionY = 1.0f;
 camera.nearPlane = 0.05f;
 camera.farPlane = 16.0f;
 camera.viewForwardZ = 1.0f;

 float projection[16]{};
 float modelView[16]{};
 client::render::buildCameraProjection(projection, camera);
 client::render::buildCameraModelView(modelView, camera);
 net::minecraft::util::math::Matrix4f projectionMatrix;
 net::minecraft::util::math::Matrix4f modelViewMatrix;
 projectionMatrix.set(projection);
 modelViewMatrix.set(modelView);
 client::render::Frustum frustum;
 frustum.compute(projectionMatrix, modelViewMatrix, 0.0, 0.0, 0.0);

 const Box justBeforeNearPlane{-0.01, -0.01, 0.0, 0.01, 0.01, 0.04};
 EXPECT_FALSE(frustum.isVisible(justBeforeNearPlane));
 EXPECT_TRUE(frustum.isVisibleIgnoringNearPlane(justBeforeNearPlane));
}
TEST(CullBoundaryTest, UnorderedColumnsKeepSectionGraphConnected) {
 ClientWorld world(nullptr, 12345ULL, 0);
 client::render::TerrainScene scene;
 scene.world = &world;
 client::render::ChunkCompilePipeline pipeline(scene);
 client::render::ChunkSectionSystem sections(scene);
 sections.setCompilePipeline(pipeline);
 pipeline.setSectionSystem(sections);
 constexpr int columns[][2] = {{0, 0}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}};
 for(const auto& column : columns) {
  world.updateChunk(column[0], column[1], true);
  world.getChunkSource()->markChunkDataReady(column[0], column[1]);
  sections.createColumn(column[0], column[1]);
 }
 std::vector<client::render::chunk::ChunkBuilder*> queue{
     sections.sectionAt(0, 0, 0)};
 std::unordered_set<client::render::chunk::ChunkBuilder*> visited;
 for(std::size_t head = 0; head < queue.size(); ++head) {
  auto* section = queue[head];
  ASSERT_NE(section, nullptr);
  if(!visited.insert(section).second) {
   continue;
  }
  for(auto* neighbor : section->neighbors) {
   EXPECT_NE(neighbor, section);
   if(neighbor != nullptr && !visited.contains(neighbor)) {
    queue.push_back(neighbor);
   }
  }
 }
 EXPECT_EQ(visited.size(), std::size(columns) * client::render::kChunkSectionCountY);
 EXPECT_EQ(sections.sectionAt(0, 0, 0)->neighbors[2], nullptr);
}
} // namespace net::minecraft::test
