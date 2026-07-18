#include "net/minecraft/client/render/terrain/TerrainRenderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include "net/minecraft/client/render/lod/LodPolicy.hpp"
#include "net/minecraft/client/render/terrain/TerrainMesher.hpp"
#include "net/minecraft/client/render/terrain/TerrainPalette.hpp"
#include "net/minecraft/client/render/terrain/TerrainSurface.hpp"
#include "net/minecraft/client/render/terrain/TerrainSurfaceData.hpp"
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::terrain {
namespace {
constexpr int kMaxJobs = 32;
constexpr int kScanPerFrame = 12;
constexpr int kChangesPerFrame = 64;
constexpr int kEvictFrames = 120;
[[nodiscard]] float regionDistance(int regionX, int regionZ, double cameraX, double cameraZ) noexcept {
  const double centerX = (static_cast<double>(regionX) + 0.5) * kTerrainRegionBlocks;
  const double centerZ = (static_cast<double>(regionZ) + 0.5) * kTerrainRegionBlocks;
  return static_cast<float>(std::max(0.0,
                                     std::max(std::abs(cameraX - centerX), std::abs(cameraZ - centerZ)) -
                                         kTerrainRegionBlocks * 0.5));
}
[[nodiscard]] float regionFarDistance(int regionX, int regionZ, double cameraX, double cameraZ) noexcept {
  const double centerX = (static_cast<double>(regionX) + 0.5) * kTerrainRegionBlocks;
  const double centerZ = (static_cast<double>(regionZ) + 0.5) * kTerrainRegionBlocks;
  return static_cast<float>(std::max(std::abs(cameraX - centerX), std::abs(cameraZ - centerZ)) +
                            kTerrainRegionBlocks * 0.5);
}
} // namespace
struct TerrainRenderer::Impl {
  struct BuildJob {
    std::uint64_t key = 0;
    int regionX = 0;
    int regionZ = 0;
    int level = 0;
    long long stamp = -1;
    std::uint64_t paletteGeneration = 0;
    std::vector<std::uint8_t> snapshot{};
    std::array<std::uint32_t, 256> topColors{};
    std::array<std::uint32_t, 256> sideColors{};
    TerrainMeshResult result{};
    bool failed = false;
  };
  struct RegionState {
    int regionX = 0;
    int regionZ = 0;
    long long stamp = -1;
    int level = -1;
    std::uint64_t paletteGeneration = 0;
    long long requestedStamp = -1;
    int requestedLevel = -1;
    std::uint64_t requestedPaletteGeneration = 0;
    float requestedDistance = 0.0f;
    int lastVisible = 0;
    chunk::ChunkRegionBuffer::Slot slot{};
    std::shared_ptr<BuildJob> job{};
  };
  ::net::minecraft::util::concurrent::WorkerHandoff<BuildJob> workers{
      ::net::minecraft::util::concurrent::WorkerPool::recommendedThreadCount(4, 2, 2)};
  chunk::ChunkRegionBuffer buffer{};
  std::unordered_map<std::uint64_t, RegionState> regions{};
  std::vector<std::pair<int, int>> offsets{};
  std::size_t scanCursor = 0;
  int offsetRadius = -1;
  int cameraRegionX = 0;
  int cameraRegionZ = 0;
  int activeJobs = 0;
  int frame = 0;
  std::uint64_t paletteGeneration = 0;
  gl::ProgramCache shaderCache{};
  gl::ShaderProgram* program = nullptr;
  bool shaderAttempted = false;
  bool running = false;
  ~Impl() {
    workers.cancelAll();
  }
  void clear() {
    workers.cancelAll();
    for(auto& [key, state] : regions) {
      (void)key;
      buffer.release(state.slot);
    }
    regions.clear();
    activeJobs = 0;
    scanCursor = 0;
    offsetRadius = -1;
    offsets.clear();
    running = false;
  }
  [[nodiscard]] bool ensureShader() {
    if(program != nullptr) {
      return true;
    }
    if(shaderAttempted) {
      return false;
    }
    shaderAttempted = true;
    if(!gl::ShaderProgram::supported()) {
      return false;
    }
    program = shaderCache.get("shaders/terrain/terrain.vsh", "shaders/terrain/terrain.fsh", "#version 330 core");
    return program != nullptr;
  }
  void rebuildOffsets(int radius) {
    if(radius == offsetRadius) {
      return;
    }
    offsets.clear();
    offsets.reserve(static_cast<std::size_t>((radius * 2 + 1) * (radius * 2 + 1)));
    for(int ring = 0; ring <= radius; ++ring) {
      for(int z = -ring; z <= ring; ++z) {
        for(int x = -ring; x <= ring; ++x) {
          if(std::max(std::abs(x), std::abs(z)) == ring) {
            offsets.emplace_back(x, z);
          }
        }
      }
    }
    offsetRadius = radius;
    scanCursor = 0;
  }
  bool queue(RegionState& state) {
    if(state.job != nullptr || activeJobs >= kMaxJobs) {
      return false;
    }
    TerrainRegionSnapshot snapshot;
    if(!TerrainSurface::instance().terrainRegionSnapshot(state.regionX, state.regionZ, snapshot)) {
      return false;
    }
    auto job = std::make_shared<BuildJob>();
    job->key = packChunkKey(state.regionX, state.regionZ);
    job->regionX = state.regionX;
    job->regionZ = state.regionZ;
    job->level = state.requestedLevel;
    job->stamp = snapshot.stamp;
    job->paletteGeneration = TerrainPalette::generation();
    job->snapshot = std::move(snapshot.bytes);
    job->topColors = TerrainPalette::table();
    job->sideColors = TerrainPalette::sideTable();
    state.job = job;
    ++activeJobs;
    workers.enqueue(
        job,
        [](BuildJob& build) {
          try {
            buildTerrainMesh(build.snapshot, build.level, build.topColors, build.sideColors, build.result);
            const float originX = static_cast<float>(build.regionX * kTerrainRegionBlocks);
            const float originZ = static_cast<float>(build.regionZ * kTerrainRegionBlocks);
            for(TessellatorVertex& vertex : build.result.vertices) {
              vertex.x += originX;
              vertex.z += originZ;
            }
          } catch(...) {
            build.failed = true;
          }
        },
        std::clamp(static_cast<int>(state.requestedDistance), 0, 1'000'000));
    return true;
  }
  void eraseRegion(std::uint64_t key) {
    const auto found = regions.find(key);
    if(found == regions.end()) {
      return;
    }
    buffer.release(found->second.slot);
    found->second.job.reset();
    regions.erase(found);
  }
  void observe(int regionX,
               int regionZ,
               double cameraX,
               double cameraZ,
               const TerrainRenderSettings& settings,
               long long observedStamp = -2) {
    const float distance = regionDistance(regionX, regionZ, cameraX, cameraZ);
    const float farDistance = regionFarDistance(regionX, regionZ, cameraX, cameraZ);
    if(distance > settings.distance || farDistance < settings.nearCutoff) {
      return;
    }
    const std::uint64_t key = packChunkKey(regionX, regionZ);
    const long long stamp = observedStamp == -2 ? TerrainSurface::instance().terrainRegionStamp(regionX, regionZ)
                                                : observedStamp;
    if(stamp < 0) {
      eraseRegion(key);
      return;
    }
    auto [it, inserted] = regions.try_emplace(key);
    RegionState& state = it->second;
    if(inserted) {
      state.regionX = regionX;
      state.regionZ = regionZ;
    }
    state.lastVisible = frame;
    state.requestedStamp = stamp;
    state.requestedLevel = lod::DistancePolicy{}.level(distance, settings.detail);
    state.requestedPaletteGeneration = paletteGeneration;
    state.requestedDistance = distance;
    if(state.stamp == state.requestedStamp && state.level == state.requestedLevel &&
       state.paletteGeneration == state.requestedPaletteGeneration) {
      return;
    }
    queue(state);
  }
  void drainCompleted(double cameraX, double cameraZ, const TerrainRenderSettings& settings) {
    for(std::shared_ptr<BuildJob>& job : workers.drainCompleted()) {
      activeJobs = std::max(0, activeJobs - 1);
      if(job == nullptr) {
        continue;
      }
      const auto found = regions.find(job->key);
      if(found == regions.end() || found->second.job != job) {
        continue;
      }
      RegionState& state = found->second;
      state.job.reset();
      const bool stale = TerrainSurface::instance().terrainRegionStamp(job->regionX, job->regionZ) != job->stamp ||
                         TerrainPalette::generation() != job->paletteGeneration ||
                         state.requestedStamp != job->stamp || state.requestedLevel != job->level ||
                         state.requestedPaletteGeneration != job->paletteGeneration;
      if(!job->failed && !stale) {
        buffer.upload(state.slot,
                      job->result.vertices.data(),
                      static_cast<int>(job->result.vertices.size()),
                      false,
                      true,
                      false);
        state.stamp = job->stamp;
        state.level = job->level;
        state.paletteGeneration = job->paletteGeneration;
      }
      observe(state.regionX, state.regionZ, cameraX, cameraZ, settings);
    }
  }
  void evict(double cameraX, double cameraZ, const TerrainRenderSettings& settings) {
    if(frame % kEvictFrames != 0) {
      return;
    }
    const float limit = settings.distance * 1.25f + kTerrainRegionBlocks;
    for(auto it = regions.begin(); it != regions.end();) {
      RegionState& state = it->second;
      const float distance = regionDistance(state.regionX, state.regionZ, cameraX, cameraZ);
      const float farDistance = regionFarDistance(state.regionX, state.regionZ, cameraX, cameraZ);
      if(distance > limit || farDistance < settings.nearCutoff) {
        buffer.release(state.slot);
        state.job.reset();
        it = regions.erase(it);
      } else {
        ++it;
      }
    }
  }
  void draw(double cameraX, double cameraY, double cameraZ, const TerrainRenderSettings& settings) {
    if(!ensureShader()) {
      return;
    }
    buffer.beginFrame();
    for(const auto& [key, state] : regions) {
      (void)key;
      if(state.slot.count <= 0 || regionDistance(state.regionX, state.regionZ, cameraX, cameraZ) > settings.distance ||
         regionFarDistance(state.regionX, state.regionZ, cameraX, cameraZ) < settings.nearCutoff) {
        continue;
      }
      buffer.addVisible(state.slot);
    }
    if(!buffer.hasVisible()) {
      return;
    }
    gl::setCap(gl::cap::Texture2D, false);
    gl::setCap(gl::cap::CullFace, false);
    gl::setCap(gl::cap::Blend, false);
    gl::setCap(gl::cap::AlphaTest, false);
    gl::depthMask(true);
    gl::setCap(gl::cap::PolygonOffsetFill, true);
    gl::polygonOffset(1.0f, 2.0f);
    program->bind();
    program->set1f("nearCutoff", std::max(0.0f, settings.nearCutoff));
    program->set1f("terrainBrightness", std::clamp(settings.brightness, 0.0f, 1.0f));
    program->set1f("uFogStart", gl::g_pipeline.fogStart);
    program->set1f("uFogEnd", gl::g_pipeline.fogEnd);
    program->set4f("uFogColor", gl::g_pipeline.fogColor[0], gl::g_pipeline.fogColor[1],
                   gl::g_pipeline.fogColor[2], gl::g_pipeline.fogColor[3]);
    program->set3f("uCameraPos", static_cast<float>(cameraX), static_cast<float>(cameraY),
                   static_cast<float>(cameraZ));
    {
      gl::MatrixGuard matrix;
      gl::translatef(-static_cast<float>(cameraX), -static_cast<float>(cameraY), -static_cast<float>(cameraZ));
      program->setMatrix4("uModelView", net::minecraft::util::math::g_modelView.top());
      program->setMatrix4("uProjection", net::minecraft::util::math::g_projection.top());
      buffer.flush(gl::prim::Triangles, false);
    }
    gl::ShaderProgram::unbind();
    gl::polygonOffset(0.0f, 0.0f);
    gl::setCap(gl::cap::PolygonOffsetFill, false);
    gl::setCap(gl::cap::Texture2D, true);
    gl::setCap(gl::cap::CullFace, true);
    gl::setCap(gl::cap::AlphaTest, true);
    gl::alphaFunc(gl::compare::Greater, 0.1f);
  }
  void render(double cameraX, double cameraY, double cameraZ) {
    const TerrainRenderSettings settings = TerrainSurface::instance().terrainRenderSettings();
    if(!settings.enabled || !TerrainSurface::instance().active()) {
      if(running) {
        clear();
      }
      return;
    }
    running = true;
    ++frame;
    const std::uint64_t currentPaletteGeneration = TerrainPalette::generation();
    if(currentPaletteGeneration != paletteGeneration) {
      paletteGeneration = currentPaletteGeneration;
      for(auto& [key, state] : regions) {
        (void)key;
        state.paletteGeneration = 0;
      }
    }
    const int nextCameraRegionX = MathHelper::floorDiv(MathHelper::floor(cameraX), kTerrainRegionBlocks);
    const int nextCameraRegionZ = MathHelper::floorDiv(MathHelper::floor(cameraZ), kTerrainRegionBlocks);
    if(nextCameraRegionX != cameraRegionX || nextCameraRegionZ != cameraRegionZ) {
      cameraRegionX = nextCameraRegionX;
      cameraRegionZ = nextCameraRegionZ;
      scanCursor = 0;
    }
    rebuildOffsets(static_cast<int>(std::ceil(settings.distance / kTerrainRegionBlocks)) + 1);
    for(const TerrainRegionChange& change : TerrainSurface::instance().drainChangedRegions(kChangesPerFrame)) {
      observe(change.regionX, change.regionZ, cameraX, cameraZ, settings, change.stamp);
    }
    for(int i = 0; i < kScanPerFrame && !offsets.empty(); ++i) {
      if(scanCursor >= offsets.size()) {
        scanCursor = 0;
      }
      const auto [offsetX, offsetZ] = offsets[scanCursor++];
      observe(cameraRegionX + offsetX, cameraRegionZ + offsetZ, cameraX, cameraZ, settings);
    }
    drainCompleted(cameraX, cameraZ, settings);
    evict(cameraX, cameraZ, settings);
    draw(cameraX, cameraY, cameraZ, settings);
  }
};
TerrainRenderer& TerrainRenderer::instance() {
  static TerrainRenderer renderer;
  return renderer;
}
TerrainRenderer::TerrainRenderer() : impl_(std::make_unique<Impl>()) {
}
TerrainRenderer::~TerrainRenderer() = default;
void TerrainRenderer::render(double cameraX, double cameraY, double cameraZ) {
  impl_->render(cameraX, cameraY, cameraZ);
}
void TerrainRenderer::reset() {
  impl_->clear();
}
} // namespace net::minecraft::client::render::terrain
