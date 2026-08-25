// Terrain culling throughput at extreme render distance.
//
// Culling pays per resident section, twice a frame, whether or not anything changed --
// the cost that scales purely with render distance. 64x64 columns x 8 = 32,768
// sections, a ~32-chunk distance.
//
// It drives the same cullByFrustum / cullByOcclusionWalk the
// frame does; standing up ChunkSectionSystem itself needs a live World and chunk
// source, which culling has nothing to do with.
#include <gtest/gtest.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::test {
namespace {
namespace render = net::minecraft::client::render;
namespace world_cull = net::minecraft::client::render::world;
using render::chunk::ChunkBuilder;
using render::chunk::kSectionBlocks;
constexpr int kColumns = 64;
constexpr int kSectionsY = render::kChunkSectionCountY;
constexpr int kFrames = 240;
// Mirrors ChunkSectionSystem::createColumn: same region assignment, same neighbour
// wiring, same face numbering.
constexpr int kFaceDirX[6] = {-1, 1, 0, 0, 0, 0};
constexpr int kFaceDirY[6] = {0, 0, -1, 1, 0, 0};
constexpr int kFaceDirZ[6] = {0, 0, 0, 0, -1, 1};
struct SectionWorld {
 std::vector<net::minecraft::block::entity::BlockEntity*> blockEntities;
 std::vector<std::unique_ptr<ChunkBuilder>> sections;
 world_cull::RegionMap regions;
 std::vector<ChunkBuilder*> byPos;
 [[nodiscard]] ChunkBuilder* at(int sectionX, int sectionY, int sectionZ) const {
  if(sectionX < 0 || sectionZ < 0 || sectionX >= kColumns || sectionZ >= kColumns || sectionY < 0 ||
     sectionY >= kSectionsY) {
   return nullptr;
  }
  return byPos[static_cast<std::size_t>((sectionX * kColumns + sectionZ) * kSectionsY + sectionY)];
 }
};
// `openness` is the fraction of columns that are open sky rather than solid rock.
// It drives the visibility graph: a solid section connects no face pair, so the
// occlusion walk stops at it, which is the whole point of the graph. A world that
// is entirely open is the worst case for the walk and entirely solid is the best,
// and neither is what a real world looks like.
// `scattered` allocates the sections out of the order the cull walks them, which is
// what a session that has been loading and dropping columns converges to. In order it
// is 46% faster; that difference is the ceiling on packing the cull fields per region.
std::unique_ptr<SectionWorld> buildSectionWorld(double openness, bool scattered = false) {
 auto built = std::make_unique<SectionWorld>();
 built->sections.reserve(static_cast<std::size_t>(kColumns * kColumns * kSectionsY));
 built->byPos.assign(static_cast<std::size_t>(kColumns * kColumns * kSectionsY), nullptr);
 std::vector<int> order(static_cast<std::size_t>(kColumns * kColumns * kSectionsY));
 for(std::size_t i = 0; i < order.size(); ++i) {
  order[i] = static_cast<int>(i);
 }
 if(scattered) {
  std::uint32_t shuffle = 99991U;
  for(std::size_t i = order.size(); i > 1; --i) {
   shuffle = shuffle * 1664525U + 1013904223U;
   std::swap(order[i - 1], order[shuffle % static_cast<std::uint32_t>(i)]);
  }
 }
 std::uint32_t state = 12345U;
 for(const int index : order) {
  {
   {
    const int sectionX = index / (kColumns * kSectionsY);
    const int sectionZ = (index / kSectionsY) % kColumns;
    const int sectionY = index % kSectionsY;
    auto section = std::make_unique<ChunkBuilder>(nullptr, built->blockEntities, sectionX * kSectionBlocks,
                                                  sectionY * kSectionBlocks, sectionZ * kSectionBlocks);
    ChunkBuilder* raw = section.get();
    raw->built = true;
    raw->renderLayerEmpty = {false, true, true, true};
    // Only the solid layer carries geometry, as in the benchmark world the mesher
    // produces: WorldRenderer's draw-list build still reads all four.
    raw->terrainAllocations_[0].capacityVertices = 4096;
    raw->terrainAllocations_[0].vertexCount = 2048;
    state = state * 1664525U + 1013904223U;
    const bool open = sectionY >= 4 || static_cast<double>(state >> 8U) / 16777216.0 < openness;
    raw->occlusion.visBits = open ? ~0ULL : 0ULL;
    const render::world::SectionPos pos{sectionX, sectionY, sectionZ};
    const render::world::SectionPos regionPos = render::world::regionOf(pos);
    auto& region = built->regions[regionPos];
    if(region == nullptr) {
     region = std::make_unique<render::chunk::TerrainRegion>(
         regionPos.x * render::world::kRegionSectionsX * kSectionBlocks,
         regionPos.y * render::world::kRegionSectionsY * kSectionBlocks,
         regionPos.z * render::world::kRegionSectionsZ * kSectionBlocks);
    }
    raw->setTerrainRegion(*region);
    region->addSection(raw);
    built->byPos[static_cast<std::size_t>((sectionX * kColumns + sectionZ) * kSectionsY + sectionY)] = raw;
    built->sections.push_back(std::move(section));
   }
  }
 }
 for(int sectionX = 0; sectionX < kColumns; ++sectionX) {
  for(int sectionZ = 0; sectionZ < kColumns; ++sectionZ) {
   for(int sectionY = 0; sectionY < kSectionsY; ++sectionY) {
    ChunkBuilder* self = built->at(sectionX, sectionY, sectionZ);
    for(int face = 0; face < 6; ++face) {
     self->neighbors[face] =
         built->at(sectionX + kFaceDirX[face], sectionY + kFaceDirY[face], sectionZ + kFaceDirZ[face]);
    }
   }
  }
 }
 return built;
}
// A real perspective frustum, built through the engine's own camera maths.
render::Frustum frustumFor(double eyeX, double eyeY, double eyeZ, float yawDegrees) {
 render::FrameRenderCamera camera{};
 const float aspect = 2560.0f / 1387.0f;
 const float halfFov = static_cast<float>(70.0 * 0.5 * 3.14159265358979 / 180.0);
 camera.projectionY = 1.0f / std::tan(halfFov);
 camera.projectionX = camera.projectionY / aspect;
 camera.nearPlane = 0.05f;
 camera.farPlane = 1024.0f;
 const float yaw = yawDegrees * static_cast<float>(3.14159265358979 / 180.0);
 camera.viewForwardX = std::sin(yaw);
 camera.viewForwardY = 0.0f;
 camera.viewForwardZ = std::cos(yaw);
 camera.viewUpX = 0.0f;
 camera.viewUpY = 1.0f;
 camera.viewUpZ = 0.0f;
 camera.viewRightX = std::cos(yaw);
 camera.viewRightY = 0.0f;
 camera.viewRightZ = -std::sin(yaw);
 float projection[16]{};
 float modelView[16]{};
 render::buildCameraProjection(projection, camera);
 render::buildCameraModelView(modelView, camera);
 net::minecraft::util::math::Matrix4f projectionMatrix;
 net::minecraft::util::math::Matrix4f modelViewMatrix;
 projectionMatrix.set(projection);
 modelViewMatrix.set(modelView);
 render::Frustum frustum;
 frustum.compute(projectionMatrix, modelViewMatrix, eyeX, eyeY, eyeZ);
 return frustum;
}
struct CullResult {
 double millis = 0.0;
 std::size_t visible = 0;
 int frames = 0;
};
void report(const char* label, const CullResult& result, std::size_t sections) {
 std::printf("[%-14s] %6zu sections  %4d frames  %7.3f ms/frame  %6zu visible/frame  %5.1f ns/section\n", label,
             sections, result.frames, result.millis / result.frames,
             result.visible / static_cast<std::size_t>(result.frames),
             1e6 * result.millis / static_cast<double>(result.frames) / static_cast<double>(sections));
 std::fflush(stdout);
}
// The camera turns a degree and a half a frame -- a steady look-around, which is
// what makes the visible set change and the walk redo its work.
template <typename Cull>
CullResult sweep(Cull&& cull) {
 CullResult result;
 std::vector<ChunkBuilder*> visible;
 visible.reserve(1 << 16);
 const double eye = kColumns * kSectionBlocks * 0.5;
 for(int frame = 0; frame < kFrames; ++frame) {
  const render::Frustum frustum = frustumFor(eye, 40.0, eye, static_cast<float>(frame) * 1.5f);
  visible.clear();
  const auto start = std::chrono::steady_clock::now();
  cull(frustum, frame + 1, visible);
  const auto end = std::chrono::steady_clock::now();
  result.millis += std::chrono::duration<double, std::milli>(end - start).count();
  result.visible += visible.size();
  ++result.frames;
 }
 return result;
}
// WorldRenderer::renderChunkLayer walks the whole visible list once per layer, and
// the frame does that for the camera pass and again for the shadow pass. Each visit
// reads one TerrainAllocation and one ModChunkMesh vector out of a ~500-byte
// ChunkBuilder, so four separate walks stream the whole visible working set through
// the cache four times. The four allocations are one cache line apart -- reading all
// four in a single visit is the same loads with a quarter of the misses. This
// measures the difference before anything is restructured around it.
struct LayerBuckets {
 std::vector<const render::chunk::TerrainAllocation*> byLayer[render::chunk::terrain_layer::Count];
 void clear() {
  for(auto& bucket : byLayer) bucket.clear();
 }
};
void buildPerLayerWalks(const std::vector<ChunkBuilder*>& visible, LayerBuckets& out) {
 for(int layer = 0; layer < render::chunk::terrain_layer::Count; ++layer) {
  for(ChunkBuilder* chunk : visible) {
   const render::chunk::TerrainAllocation& allocation = chunk->terrainAllocation(layer);
   if(allocation.valid() && chunk->terrainRegion() != nullptr) {
    out.byLayer[layer].push_back(&allocation);
   }
   for(const render::chunk::ModChunkMesh& modMesh : chunk->modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    (void)modMesh;
   }
  }
 }
}
void buildSingleWalk(const std::vector<ChunkBuilder*>& visible, LayerBuckets& out) {
 for(ChunkBuilder* chunk : visible) {
  render::chunk::TerrainRegion* region = chunk->terrainRegion();
  for(int layer = 0; layer < render::chunk::terrain_layer::Count; ++layer) {
   const render::chunk::TerrainAllocation& allocation = chunk->terrainAllocation(layer);
   if(allocation.valid() && region != nullptr) {
    out.byLayer[layer].push_back(&allocation);
   }
   for(const render::chunk::ModChunkMesh& modMesh : chunk->modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    (void)modMesh;
   }
  }
 }
}
} // namespace
TEST(TerrainCullPerf, DrawListBuildWalksTheVisibleSetFourTimes) {
 const std::unique_ptr<SectionWorld> world = buildSectionWorld(0.35, /*scattered=*/true);
 std::vector<world_cull::OcclusionQueueEntry> queue;
 std::vector<ChunkBuilder*> visible;
 queue.reserve(1 << 16);
 visible.reserve(1 << 16);
 world_cull::cullByOcclusionWalk(world->at(kColumns / 2, 4, kColumns / 2), frustumFor(0, 0, 0, 0), 1, queue,
                                 visible);
 LayerBuckets buckets;
 double perLayerMs = 0.0;
 double singleMs = 0.0;
 constexpr int kRounds = 60;
 for(int round = 0; round < kRounds; ++round) {
  buckets.clear();
  auto start = std::chrono::steady_clock::now();
  buildPerLayerWalks(visible, buckets);
  auto end = std::chrono::steady_clock::now();
  perLayerMs += std::chrono::duration<double, std::milli>(end - start).count();
  buckets.clear();
  start = std::chrono::steady_clock::now();
  buildSingleWalk(visible, buckets);
  end = std::chrono::steady_clock::now();
  singleMs += std::chrono::duration<double, std::milli>(end - start).count();
 }
 std::printf("[draw list    ]  %6zu visible  per-layer walk %6.3f ms  single walk %6.3f ms  (x%.2f)\n",
             visible.size(), perLayerMs / kRounds, singleMs / kRounds,
             perLayerMs / (singleMs > 0.0 ? singleMs : 1.0));
 std::fflush(stdout);
 EXPECT_GT(visible.size(), 0u);
}
TEST(TerrainCullPerf, FrustumSweepAtExtremeDistance) {
 const std::unique_ptr<SectionWorld> world = buildSectionWorld(1.0, /*scattered=*/true);
 const CullResult result = sweep([&world](const render::Frustum& frustum, int stamp,
                                          std::vector<ChunkBuilder*>& out) {
  world_cull::cullByFrustum(world->regions, frustum, stamp, out);
 });
 report("frustum", result, world->sections.size());
 EXPECT_GT(result.visible, 0u);
}
TEST(TerrainCullPerf, OcclusionWalkAtExtremeDistance) {
 const std::unique_ptr<SectionWorld> world = buildSectionWorld(0.35, /*scattered=*/true);
 std::vector<world_cull::OcclusionQueueEntry> queue;
 queue.reserve(1 << 16);
 ChunkBuilder* start = world->at(kColumns / 2, 4, kColumns / 2);
 ASSERT_NE(start, nullptr);
 const CullResult result = sweep([&](const render::Frustum& frustum, int stamp, std::vector<ChunkBuilder*>& out) {
  world_cull::cullByOcclusionWalk(start, frustum, stamp, queue, out);
 });
 report("occlusion walk", result, world->sections.size());
 EXPECT_GT(result.visible, 0u);
}
} // namespace net::minecraft::test
