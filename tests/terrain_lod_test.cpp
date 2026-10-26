#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/render/chunk/MeshUploadBudget.hpp"
#include "net/minecraft/client/render/chunk/TerrainLod.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#define private public
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#undef private
namespace net::minecraft::test {
namespace {
using namespace client::render;
using namespace client::render::chunk;
TessellatorMesh plane(int width = 16, int depth = 16, int axis = 1) {
 std::vector<TessellatorVertex> vertices;
 const int ua = (axis + 1) % 3;
 const int va = (axis + 2) % 3;
 constexpr int corners[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
 for(int v = 0; v < depth; ++v) {
  for(int u = 0; u < width; ++u) {
   for(const auto& corner : corners) {
    TessellatorVertex vertex{};
    std::array<float, 3> xyz{};
    xyz[axis] = 8;
    xyz[ua] = static_cast<float>(u + corner[0]);
    xyz[va] = static_cast<float>(v + corner[1]);
    vertex.x = xyz[0]; vertex.y = xyz[1]; vertex.z = xyz[2];
    vertex.u = corner[0] ? 0.125f : 0.0625f;
    vertex.v = corner[1] ? 0.125f : 0.0625f;
    vertex.normal = 127 << (axis * 8);
    vertex.midU = vertex.midV = 0.09375f;
    vertex.entity[0] = 1;
    vertex.entity[1] = -1;
    vertex.tangent[ua] = 32767;
    vertices.push_back(vertex);
   }
  }
 }
 return {std::move(vertices), 7, true, true};
}
double meshArea(const TessellatorMesh& mesh) {
 double result = 0;
 for(std::size_t i = 0; i < mesh.vertices.size(); i += 4) {
  const auto& a = mesh.vertices[i];
  const auto& b = mesh.vertices[i + 1];
  const auto& c = mesh.vertices[i + 3];
  const double ab[3] = {b.x - a.x, b.y - a.y, b.z - a.z};
  const double ac[3] = {c.x - a.x, c.y - a.y, c.z - a.z};
  const double cross[3] = {ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0]};
  result += std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
 }
 return result;
}
struct LodWorld {
 ClientWorld world{nullptr, 12345ULL, 0};
 std::vector<net::minecraft::block::entity::BlockEntity*> entities;
 void load(int x, int z) {
  world.updateChunk(x, z, true);
  world.getChunkSource()->markChunkDataReady(x, z);
  Chunk& chunk = world.getChunk(x, z);
  for(int bx = 0; bx < 16; ++bx) {
   for(int bz = 0; bz < 16; ++bz) {
    for(int y = 0; y < 8; ++y) chunk.blocks[(bx << 11) | (bz << 7) | y] = 1;
    for(int y = 8; y < 128; ++y) chunk.skyLight.set(bx, y, bz, 15);
   }
  }
  chunk.populateHeightMapOnly();
 }
 std::shared_ptr<ChunkMeshJob> mesh(int level, int x = 0, int z = 0, int y = 0) {
  auto builder = std::make_shared<ChunkBuilder>(&world, entities, x * 16, y * 16, z * 16);
  builder->requestedLod = level;
  client::option::RenderSettings opts;
  opts.ambientOcclusionActive = true;
  opts.ambientOcclusionStrength = 1;
  auto job = ChunkMeshJob::capture(*builder, opts);
  if(job) {
   job->buildSnapshot();
   ChunkBuilder::buildMesh(*job);
  }
  return job;
 }
};
}
TEST(TerrainLod, DistanceBandsAndHysteresis) {
 EXPECT_EQ(terrainLodLevel(15, -1), 0);
 EXPECT_EQ(terrainLodLevel(16, -1), 1);
 EXPECT_EQ(terrainLodLevel(32, -1), 2);
 EXPECT_EQ(terrainLodLevel(64, -1), 3);
 EXPECT_EQ(terrainLodLevel(17, 0), 0);
 EXPECT_EQ(terrainLodLevel(18, 0), 1);
 EXPECT_EQ(terrainLodLevel(14, 1), 1);
 EXPECT_EQ(terrainLodLevel(13, 1), 0);
 EXPECT_EQ(terrainLodLevel(33, 1), 1);
 EXPECT_EQ(terrainLodLevel(34, 1), 2);
 EXPECT_EQ(terrainLodLevel(30, 2), 2);
 EXPECT_EQ(terrainLodLevel(29, 2), 1);
 EXPECT_EQ(terrainLodLevel(100, 3, false), 0);
}
TEST(TerrainLod, CoverageIsCircularAndHandlesNegativeCoordinates) {
 EXPECT_TRUE(terrainColumnInRange(-10, -20, -10, -20, 0));
 EXPECT_TRUE(terrainColumnInRange(-7, -16, -10, -20, 5));
 EXPECT_FALSE(terrainColumnInRange(-6, -16, -10, -20, 5));
 EXPECT_FALSE(terrainColumnInRange(0, 0, 0, 0, -1));
 EXPECT_FALSE(terrainColumnInRange(std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), 0, 80));
}
TEST(TerrainLod, FlatFacesReduceByTierWithoutChangingSurfaceArea) {
 for(int axis = 0; axis < 3; ++axis) {
  for(int level = 1; level <= 3; ++level) {
   auto mesh = plane(16, 16, axis);
   std::vector<std::uint8_t> eligible(256, 1);
   const auto saved = simplifyTerrainMesh(mesh, level, eligible);
   const std::size_t expectedQuads = level == 1 ? 16 : level == 2 ? 4 : 1;
   EXPECT_EQ(mesh.vertices.size(), expectedQuads * 4);
   EXPECT_EQ(mesh.vertexCount(), mesh.vertices.size());
   EXPECT_EQ(saved, 1024 - expectedQuads * 4);
   EXPECT_DOUBLE_EQ(meshArea(mesh), 256);
   EXPECT_FALSE(mesh.empty());
   for(const auto& vertex : mesh.vertices) {
    EXPECT_TRUE(vertex.u == 0.0625f || vertex.u == 0.125f);
    EXPECT_EQ(vertex.entity[0], 1);
    EXPECT_EQ(vertex.light, 0x00f000f0);
   }
  }
 }
}
TEST(TerrainLod, NearMeshAndUnsupportedGeometryRemainByteIdentical) {
 auto mesh = plane();
 const auto original = mesh.vertices;
 std::vector<std::uint8_t> eligible(256, 1);
 EXPECT_EQ(simplifyTerrainMesh(mesh, 0, eligible), 0u);
 EXPECT_EQ(std::memcmp(original.data(), mesh.vertices.data(), original.size() * sizeof(TessellatorVertex)), 0);
 std::fill(eligible.begin(), eligible.end(), 0);
 EXPECT_EQ(simplifyTerrainMesh(mesh, 3, eligible), 0u);
 EXPECT_EQ(std::memcmp(original.data(), mesh.vertices.data(), original.size() * sizeof(TessellatorVertex)), 0);
}
TEST(TerrainLod, MaterialTextureColorAndLightDiscontinuitiesPreventMerging) {
 for(int field = 0; field < 6; ++field) {
  auto mesh = plane(2, 1);
  for(int c = 4; c < 8; ++c) {
   auto& v = mesh.vertices[c];
   if(field == 0) v.entity[0] = 2;
   if(field == 1) v.u += 0.0625f;
   if(field == 2) v.color = 0xff808080;
   if(field == 3) v.light = 0;
   if(field == 4) v.entity[2] = 1;
   if(field == 5) v.tangent[3] = -32767;
  }
  const auto original = mesh.vertices;
  EXPECT_EQ(simplifyTerrainMesh(mesh, 3, std::vector<std::uint8_t>(2, 1)), 0u);
  EXPECT_EQ(std::memcmp(original.data(), mesh.vertices.data(), original.size() * sizeof(TessellatorVertex)), 0);
 }
}
TEST(TerrainLod, GapsAndOverlappingFacesArePreserved) {
 auto mesh = plane(4, 4);
 mesh.vertices.erase(mesh.vertices.begin() + 20, mesh.vertices.begin() + 24);
 mesh.resizeCpuVertices(mesh.vertices.size());
 EXPECT_GT(simplifyTerrainMesh(mesh, 3, std::vector<std::uint8_t>(15, 1)), 0u);
 EXPECT_DOUBLE_EQ(meshArea(mesh), 15);
 auto duplicates = plane(2, 1);
 const auto copy = duplicates.vertices;
 duplicates.vertices.insert(duplicates.vertices.end(), copy.begin(), copy.end());
 duplicates.resizeCpuVertices(16);
 EXPECT_EQ(simplifyTerrainMesh(duplicates, 3, std::vector<std::uint8_t>(4, 1)), 0u);
 EXPECT_DOUBLE_EQ(meshArea(duplicates), 4);
}
TEST(TerrainLod, NonUnitFacesAndSmoothLightingStayUnmerged) {
 auto mesh = plane(2, 1);
 mesh.vertices[0].color = 0xff808080;
 mesh.vertices[4].x += 0.25f;
 const auto original = mesh.vertices;
 EXPECT_EQ(simplifyTerrainMesh(mesh, 3, std::vector<std::uint8_t>(2, 1)), 0u);
 EXPECT_EQ(std::memcmp(original.data(), mesh.vertices.data(), original.size() * sizeof(TessellatorVertex)), 0);
}
TEST(TerrainLod, RealMeshJobsKeepVisibilityAndFluidGeometry) {
 LodWorld fixture;
 fixture.load(0, 0);
 Chunk& chunk = fixture.world.getChunk(0, 0);
 for(int x = 2; x < 6; ++x) {
  for(int z = 2; z < 6; ++z) chunk.blocks[(x << 11) | (z << 7) | 8] = 9;
 }
 chunk.populateHeightMapOnly();
 const auto near = fixture.mesh(0);
 const auto far = fixture.mesh(2);
 ASSERT_NE(near, nullptr);
 ASSERT_NE(far, nullptr);
 EXPECT_FALSE(near->failed);
 EXPECT_FALSE(far->failed);
 EXPECT_EQ(near->result.visibilityBits, far->result.visibilityBits);
 EXPECT_GT(far->lodSavedVertices, 0u);
 const auto& original = near->result.layers[terrain_layer::Translucent].vertices;
 const auto& distant = far->result.layers[terrain_layer::Translucent].vertices;
 ASSERT_FALSE(original.empty());
 ASSERT_EQ(original.size(), distant.size());
 EXPECT_EQ(std::memcmp(original.data(), distant.data(), original.size() * sizeof(TessellatorVertex)), 0);
 EXPECT_DOUBLE_EQ(meshArea(near->result.layers[terrain_layer::Solid]), meshArea(far->result.layers[terrain_layer::Solid]));
}
TEST(TerrainLod, PerformanceOptionPersistsAndVoxelPacksDisableSimplification) {
 client::option::GameOptions options;
 const auto spec = client::option::OptionRegistry::byKey("terrainLod");
 ASSERT_TRUE(spec.has_value());
 (*spec)->load(options, "false");
 EXPECT_FALSE(options.terrainLod);
 options.terrainLod = true;
 auto settings = client::option::renderSettings(options);
 EXPECT_TRUE(settings.terrainLod);
 PackDefinition pack;
 pack.voxelizeLightBlocks = true;
 client::option::applyShaderPack(settings, pack);
 EXPECT_FALSE(settings.terrainLod);
 const auto scale = client::option::OptionRegistry::byKey("renderScale");
 ASSERT_TRUE(scale.has_value());
 EXPECT_EQ((*scale)->onFloatChange & client::option::ApplyFlags::ReloadWorld, client::option::ApplyFlags::None);
}
TEST(TerrainLod, NegativeWorldCoordinatesStayInsideTheirRegionSection) {
 LodWorld fixture;
 fixture.load(-1, -1);
 const auto near = fixture.mesh(0, -1, -1);
 const auto far = fixture.mesh(3, -1, -1);
 ASSERT_NE(near, nullptr);
 ASSERT_NE(far, nullptr);
 EXPECT_GT(far->lodSavedVertices, 0u);
 EXPECT_DOUBLE_EQ(meshArea(near->result.layers[0]), meshArea(far->result.layers[0]));
 for(const auto& vertex : far->result.layers[0].vertices) {
  EXPECT_GE(vertex.x, 112);
  EXPECT_LE(vertex.x, 128);
  EXPECT_GE(vertex.z, 112);
  EXPECT_LE(vertex.z, 128);
 }
}
TEST(TerrainUploadBudget, BoundsCombinedLanesAndAllowsOneOversizeMeshToProgress) {
 MeshUploadBudget budget(100);
 EXPECT_TRUE(budget.tryConsume(60));
 EXPECT_FALSE(budget.tryConsume(50));
 EXPECT_TRUE(budget.tryConsume(40));
 EXPECT_FALSE(budget.tryConsume(1));
 EXPECT_EQ(budget.used(), 100u);
 MeshUploadBudget oversize(100);
 EXPECT_TRUE(oversize.tryConsume(200));
 EXPECT_FALSE(oversize.tryConsume(1));
}
TEST(TerrainUploadBudget, VertexReusePoolRejectsOversizedRetainedBuffers) {
 std::vector<std::vector<TessellatorVertex>> drained;
 for(int i = 0; i <= 256; ++i) {
  auto buffer = VertexBufferPool::acquire();
  if(buffer.capacity() == 0) break;
  drained.push_back(std::move(buffer));
 }
 std::vector<TessellatorVertex> oversized;
 oversized.reserve(64u * 1024u * 1024u / sizeof(TessellatorVertex) + 1);
 VertexBufferPool::release(std::move(oversized));
 EXPECT_EQ(VertexBufferPool::acquire().capacity(), 0u);
 for(auto& buffer : drained) VertexBufferPool::release(std::move(buffer));
}
TEST(TerrainLod, SectionRingReactsToMovementScaleAndToggleWithoutReload) {
 LodWorld fixture;
 fixture.load(0, 0);
 fixture.load(18, 0);
 fixture.load(21, 0);
 client::option::GameOptions options;
 options.renderScale = 1.25f;
 auto settings = client::option::renderSettings(options);
 entity::LivingEntity camera(&fixture.world);
 TerrainScene scene;
 scene.world = &fixture.world;
 scene.camera = &camera;
 scene.settings = &settings;
 scene.options = &options;
 ChunkCompilePipeline pipeline(scene);
 ChunkSectionSystem sections(scene);
 pipeline.setSectionSystem(sections);
 sections.setCompilePipeline(pipeline);
 int reloads = 0;
 scene.reloadRequested = [&] { ++reloads; };
 sections.reloadIfViewDistanceChanged();
 sections.createColumn(0, 0);
 sections.createColumn(18, 0);
 sections.createColumn(21, 0);
 ASSERT_NE(sections.sectionAt(21, 0, 0), nullptr);
 auto* original = sections.sectionAt(0, 0, 0);
 EXPECT_EQ(sections.sectionAt(18, 0, 0)->requestedLod, 1);
 options.renderScale = 1;
 settings = client::option::renderSettings(options);
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(0, 0, 0), original);
 EXPECT_EQ(sections.sectionAt(18, 0, 0), nullptr);
 EXPECT_EQ(sections.sectionAt(21, 0, 0), nullptr);
 camera.x = 18 * 16;
 sections.reloadIfViewDistanceChanged();
 sections.createColumn(18, 0);
 auto* moved = sections.sectionAt(18, 0, 0);
 ASSERT_NE(moved, nullptr);
 EXPECT_EQ(moved->requestedLod, 0);
 options.terrainLod = false;
 settings = client::option::renderSettings(options);
 sections.reloadIfViewDistanceChanged();
 EXPECT_EQ(sections.sectionAt(18, 0, 0), moved);
 EXPECT_EQ(reloads, 0);
 sections.clearSections();
}
TEST(NativeTerrainBudget, GeneratedColumnMeshBytes) {
 LodWorld fixture;
 OverworldChunkGenerator generator(nullptr, 12345ULL);
 generator.useLocalBiomeSource(true);
 generator.enableModGeneration(false);
 for(int x = -1; x <= 3; ++x) {
  for(int z = -1; z <= 3; ++z) {
   fixture.world.updateChunk(x, z, true);
   auto generated = generator.loadChunk(&generator, x, z);
   Chunk& chunk = fixture.world.getChunk(x, z);
   chunk.blocks = std::move(generated.blocks);
   chunk.meta = std::move(generated.meta);
   chunk.skyLight = std::move(generated.skyLight);
   chunk.blockLight = std::move(generated.blockLight);
   chunk.populateHeightMap(false);
   fixture.world.getChunkSource()->markChunkDataReady(x, z);
  }
 }
 std::array<std::size_t, 4> vertices{};
 std::array<double, 4> times{};
 for(int level = 0; level <= 3; ++level) {
  const auto start = std::chrono::steady_clock::now();
  for(int x = 0; x < 3; ++x) {
   for(int z = 0; z < 3; ++z) {
    for(int y = 0; y < 8; ++y) {
     const auto job = fixture.mesh(level, x, z, y);
     ASSERT_NE(job, nullptr);
     ASSERT_FALSE(job->failed);
     for(const auto& layer : job->result.layers) vertices[level] += layer.vertices.size();
    }
   }
  }
  times[level] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  std::printf("[native-lod] tier=%d columns=9 vertices=%zu bytes/column=%zu build_ms=%.3f\n",
              level, vertices[level], vertices[level] * sizeof(TerrainVertex) / 9, times[level]);
 }
 EXPECT_GT(vertices[0], 0u);
 EXPECT_LT(vertices[1], vertices[0]);
 EXPECT_LE(vertices[2], vertices[1]);
 EXPECT_LE(vertices[3], vertices[2]);
}
}
