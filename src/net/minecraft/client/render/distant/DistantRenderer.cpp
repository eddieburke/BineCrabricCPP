#include "net/minecraft/client/render/distant/DistantRenderer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::render::distant {
namespace {
constexpr std::size_t kSlabBytes = 256u * 1024u * 1024u;
constexpr std::size_t kUploadBytesPerFrame = 8u * 1024u * 1024u;
constexpr int kSubmitsPerFrame = 16;
constexpr int kMeshPriority = 1 << 24;
constexpr int kRetryFrames = 30;
int strideFor(double distance, double nativeBlocks) {
 const double band = std::max(64.0, nativeBlocks);
 if(distance < band * 2.0) return 1;
 if(distance < band * 4.0) return 2;
 if(distance < band * 8.0) return 4;
 return 8;
}
void normalMatrix(const util::math::Matrix4f& modelView, float out[9]) {
 util::math::Matrix4f inverse = modelView;
 inverse.invert();
 for(int column = 0; column < 3; ++column) {
  for(int row = 0; row < 3; ++row) {
   out[column * 3 + row] = inverse.m[row * 4 + column];
  }
 }
}
} // namespace
void DistantRenderer::PinnedChunks::release() noexcept {
 for(Chunk*& chunk : chunks) {
  if(chunk != nullptr) {
   chunk->releaseRenderPin();
   chunk = nullptr;
  }
 }
}
DistantRenderer::DistantRenderer() : palette_(defaultPalette()) {
}
DistantRenderer::~DistantRenderer() {
 reset();
 slab_.destroy();
}
void DistantRenderer::releaseTile(Tile& tile) {
 if(tile.opaque.has_value()) slab_.release(*tile.opaque);
 if(tile.water.has_value()) slab_.release(*tile.water);
 tile.opaque.reset();
 tile.water.reset();
}
void DistantRenderer::reset() {
 for(auto& [key, tile] : tiles_) {
  releaseTile(tile);
 }
 tiles_.clear();
 shared_ = std::make_shared<Shared>();
 biomes_.reset();
 inFlight_ = 0;
 drawnLastFrame_ = 0;
}
void DistantRenderer::markChunkDirty(int chunkX, int chunkZ) {
 const int tileX = MathHelper::floorDiv(chunkX, kTileChunks);
 const int tileZ = MathHelper::floorDiv(chunkZ, kTileChunks);
 const int localX = chunkX - tileX * kTileChunks;
 const int localZ = chunkZ - tileZ * kTileChunks;
 for(int dz = localZ == 0 ? -1 : 0; dz <= (localZ == kTileChunks - 1 ? 1 : 0); ++dz) {
  for(int dx = localX == 0 ? -1 : 0; dx <= (localX == kTileChunks - 1 ? 1 : 0); ++dx) {
   const auto it = tiles_.find(TileKey{tileX + dx, tileZ + dz});
   if(it != tiles_.end()) {
    ++it->second.generation;
    it->second.retryFrame = 0;
   }
  }
 }
}
void DistantRenderer::drainCompleted() {
 std::vector<Completed> completed;
 {
  const std::lock_guard lock(shared_->mutex);
  completed.swap(shared_->completed);
 }
 std::size_t uploaded = 0;
 std::vector<Completed> deferred;
 for(Completed& result : completed) {
  const auto it = tiles_.find(result.key);
  if(it == tiles_.end()) {
   --inFlight_;
   continue;
  }
  Tile& tile = it->second;
  const std::size_t bytes = (result.mesh.opaque.size() + result.mesh.water.size()) * sizeof(DhVertex);
  if(uploaded > 0 && uploaded + bytes > kUploadBytesPerFrame) {
   deferred.push_back(std::move(result));
   continue;
  }
  --inFlight_;
  tile.inFlight = false;
  if(result.generation != tile.generation || result.stride != tile.wantedStride) {
   continue;
  }
  std::optional<DistantSlab::Allocation> opaque;
  std::optional<DistantSlab::Allocation> water;
  if(!result.mesh.opaque.empty()) opaque = slab_.allocate(result.mesh.opaque.size());
  if(!result.mesh.water.empty()) water = slab_.allocate(result.mesh.water.size());
  if((!result.mesh.opaque.empty() && !opaque.has_value()) || (!result.mesh.water.empty() && !water.has_value())) {
   if(opaque.has_value()) slab_.release(*opaque);
   if(water.has_value()) slab_.release(*water);
   tile.retryFrame = static_cast<int>(frame_) + kRetryFrames;
   continue;
  }
  releaseTile(tile);
  if(opaque.has_value()) slab_.upload(*opaque, result.mesh.opaque);
  if(water.has_value()) slab_.upload(*water, result.mesh.water);
  tile.opaque = opaque;
  tile.water = water;
  tile.minY = result.mesh.minY;
  tile.maxY = result.mesh.maxY;
  tile.stride = result.stride;
  tile.builtGeneration = result.generation;
  uploaded += bytes;
 }
 if(!deferred.empty()) {
  const std::lock_guard lock(shared_->mutex);
  for(Completed& result : deferred) shared_->completed.push_back(std::move(result));
 }
}
void DistantRenderer::update(World& world, const option::RenderSettings& settings, double eyeX, double eyeY,
                             double eyeZ) {
 ++frame_;
 eyeX_ = eyeX;
 eyeY_ = eyeY;
 eyeZ_ = eyeZ;
 if(!settings.distantTerrain) {
  if(!tiles_.empty()) reset();
  return;
 }
 if(!slabReady_) {
  slabReady_ = slab_.create(kSlabBytes);
  if(!slabReady_) return;
 }
 drainCompleted();
 ChunkSource* source = world.getChunkSource();
 if(source == nullptr) return;
 const double viewBlocks = settings.renderDistance.blocks;
 const double nativeBlocks = static_cast<double>(settings.nativeChunkRadius * 16);
 const double innerBlocks = std::max(0.0, nativeBlocks - 16.0);
 const int reach = static_cast<int>(std::ceil(viewBlocks / kTileBlocks)) + 1;
 const int centerX = MathHelper::floorDiv(MathHelper::floor(eyeX), kTileBlocks);
 const int centerZ = MathHelper::floorDiv(MathHelper::floor(eyeZ), kTileBlocks);
 wanted_.clear();
 for(int tz = centerZ - reach; tz <= centerZ + reach; ++tz) {
  for(int tx = centerX - reach; tx <= centerX + reach; ++tx) {
   const double minX = static_cast<double>(tx * kTileBlocks);
   const double minZ = static_cast<double>(tz * kTileBlocks);
   const double nearX = std::clamp(eyeX, minX, minX + kTileBlocks) - eyeX;
   const double nearZ = std::clamp(eyeZ, minZ, minZ + kTileBlocks) - eyeZ;
   const double nearest = std::sqrt(nearX * nearX + nearZ * nearZ);
   if(nearest > viewBlocks) continue;
   const double farX = std::max(std::abs(minX - eyeX), std::abs(minX + kTileBlocks - eyeX));
   const double farZ = std::max(std::abs(minZ - eyeZ), std::abs(minZ + kTileBlocks - eyeZ));
   if(std::sqrt(farX * farX + farZ * farZ) < innerBlocks) continue;
   const double centerDx = minX + kTileBlocks * 0.5 - eyeX;
   const double centerDz = minZ + kTileBlocks * 0.5 - eyeZ;
   const double distance = std::sqrt(centerDx * centerDx + centerDz * centerDz);
   Tile& tile = tiles_[TileKey{tx, tz}];
   tile.seenFrame = frame_;
   const int stride = strideFor(distance, nativeBlocks);
   if(tile.wantedStride != stride) {
    tile.wantedStride = stride;
    ++tile.generation;
   }
   wanted_.emplace_back(static_cast<int>(distance), TileKey{tx, tz});
  }
 }
 for(auto it = tiles_.begin(); it != tiles_.end();) {
  if(it->second.seenFrame != frame_) {
   releaseTile(it->second);
   it = tiles_.erase(it);
  } else {
   ++it;
  }
 }
 std::sort(wanted_.begin(), wanted_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
 auto& pool = util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 const int maxInFlight = static_cast<int>(std::max(2u, pool.threadCount() * 2u));
 if(biomes_ == nullptr && world.getBiomeSource() != nullptr) {
  biomes_ = std::shared_ptr<BiomeSource>(world.getBiomeSource()->clone());
 }
 int submitted = 0;
 for(const auto& [distance, key] : wanted_) {
  if(inFlight_ >= maxInFlight || submitted >= kSubmitsPerFrame) break;
  Tile& tile = tiles_[key];
  if(tile.inFlight || (tile.builtGeneration == tile.generation && tile.stride == tile.wantedStride)) {
   continue;
  }
  if(tile.retryFrame > static_cast<int>(frame_)) continue;
  auto pins = std::make_shared<PinnedChunks>();
  pins->baseX = key.x * kTileChunks - 1;
  pins->baseZ = key.z * kTileChunks - 1;
  bool complete = true;
  for(int cz = 0; cz < kPinSide && complete; ++cz) {
   for(int cx = 0; cx < kPinSide; ++cx) {
    const int chunkX = pins->baseX + cx;
    const int chunkZ = pins->baseZ + cz;
    Chunk* chunk = source->isChunkDataReady(chunkX, chunkZ) ? source->getChunkIfLoaded(chunkX, chunkZ) : nullptr;
    if(chunk != nullptr && !chunk->tryAcquireRenderPin()) chunk = nullptr;
    pins->chunks[static_cast<std::size_t>(cz * kPinSide + cx)] = chunk;
    const bool own = cx >= 1 && cx <= kTileChunks && cz >= 1 && cz <= kTileChunks;
    if(own && chunk == nullptr) {
     complete = false;
     break;
    }
   }
  }
  if(!complete) {
   pins->release();
   tile.retryFrame = static_cast<int>(frame_) + kRetryFrames;
   continue;
  }
  tile.inFlight = true;
  ++inFlight_;
  ++submitted;
  const std::shared_ptr<Shared> shared = shared_;
  const std::shared_ptr<const BiomeSource> biomes = biomes_;
  const BlockPalette* palette = &palette_;
  const std::uint64_t generation = tile.generation;
  const int stride = tile.wantedStride;
  pool.submit(
      [shared, pins, biomes, palette, key, generation, stride] {
       Completed result;
       result.key = key;
       result.stride = stride;
       result.generation = generation;
       TileCells cells;
       const ChunkLookup lookup = [&pins](int chunkX, int chunkZ) -> const Chunk* {
        const int cx = chunkX - pins->baseX;
        const int cz = chunkZ - pins->baseZ;
        if(cx < 0 || cz < 0 || cx >= kPinSide || cz >= kPinSide) return nullptr;
        return pins->chunks[static_cast<std::size_t>(cz * kPinSide + cx)];
       };
       if(extractTile(lookup, key.x, key.z, stride, cells, biomes.get())) {
        meshTile(cells, *palette, result.mesh);
       } else {
        result.generation = generation + 1;
       }
       pins->release();
       const std::lock_guard lock(shared->mutex);
       shared->completed.push_back(std::move(result));
      },
      kMeshPriority + distance,
      this,
      [shared, pins, key, generation, stride] {
       pins->release();
       Completed result;
       result.key = key;
       result.stride = stride;
       result.generation = generation + 1;
       const std::lock_guard lock(shared->mutex);
       shared->completed.push_back(std::move(result));
      });
 }
}
void DistantRenderer::ensureDepthTextures(int width, int height) {
 if(depthWidth_ == width && depthHeight_ == height && depth_[0] && depth_[1]) return;
 for(gl::GlTexture& texture : depth_) {
  if(!texture) texture = gl::GlTexture(core::genTexture());
  core::bindTexture(static_cast<int>(texture.handle()));
  ::glTexImage2D(gl::cap::Texture2D, 0, 0x8CAC, width, height, 0, 0x1902, 0x1406, nullptr);
  ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x812F);
  ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x812F);
 }
 depthWidth_ = width;
 depthHeight_ = height;
}
void DistantRenderer::draw(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler,
                           bool water) {
 if(!settings.distantTerrain || !slabReady_ || tiles_.empty()) return;
 gl::ShaderProgram* program = pipeline.distantProgram(water);
 if(program == nullptr) return;
 PackInstance* scene = pipeline.distantScenePack();
 const FrameRenderCamera& frame = core::cameraFrame();
 util::math::Matrix4f projection;
 if(scene != nullptr) {
  ensureDepthTextures(scene->colorTargets.width(), scene->colorTargets.height());
  scene->dhDepthTexture = depth_[0].handle();
  scene->dhDepthTextureNoTranslucent = depth_[1].handle();
  if(water) {
   scene->colorTargets.bindGbuffersWithDepth(depth_[0].handle());
   core::bindTexture(static_cast<int>(depth_[1].handle()));
   ::glCopyTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, 0, 0, depthWidth_, depthHeight_);
  } else {
   scene->colorTargets.bindGbuffersWithDepth(depth_[0].handle());
   const core::DepthScope clearDepth(true, true);
   ::glClear(0x00000100);
  }
  FrameRenderCamera dhCamera = frame;
  dhCamera.nearPlane = frame.dhNearPlane;
  dhCamera.farPlane = frame.dhFarPlane;
  buildCameraProjection(projection.data(), dhCamera);
 } else {
  projection = core::drawProjection();
 }
 pipeline.bindDistantProgram(*program, water);
 const util::math::Matrix4f& modelView = core::drawModelView();
 util::math::Matrix4f modelViewInverse = modelView;
 modelViewInverse.invert();
 util::math::Matrix4f projectionInverse = projection;
 projectionInverse.invert();
 float normal[9]{};
 normalMatrix(modelView, normal);
 program->setMatrix4("iris_ModelViewMatrix", modelView.m);
 program->setMatrix4("iris_ModelViewMatrixInverse", modelViewInverse.m);
 program->setMatrix4("iris_ProjectionMatrix", projection.m);
 program->setMatrix4("iris_ProjectionMatrixInverse", projectionInverse.m);
 program->setMatrix3At(program->location("iris_NormalMatrix"), normal);
 const core::FogUniforms& fog = core::fog();
 program->set1f("betaFogStart", fog.start);
 program->set1f("betaFogEnd", fog.end);
 program->set1i("customFog", fog.custom ? 1 : 0);
 program->set1i("fogShape", fog.enabled ? fog.shape : -1);
 program->set1f("mircoOffset", 0.01f);
 program->set1f("clipDistance", static_cast<float>(settings.nativeChunkRadius * 16 + 8));
 const int modelOffset = program->location("modelOffset");
 const core::DepthScope depth(true, true);
 const core::CullScope cull(!water);
 const core::BlendScope blend(water);
 gl::GLCore::bindVertexArray(slab_.vao());
 int drawn = 0;
 for(const auto& [key, tile] : tiles_) {
  const std::optional<DistantSlab::Allocation>& allocation = water ? tile.water : tile.opaque;
  if(!allocation.has_value() || allocation->vertexCount == 0) continue;
  const double minX = static_cast<double>(key.x * kTileBlocks);
  const double minZ = static_cast<double>(key.z * kTileBlocks);
  if(culler != nullptr &&
     !culler->isVisible(minX, static_cast<double>(tile.minY), minZ, minX + kTileBlocks,
                        static_cast<double>(tile.maxY) + 1.0, minZ + kTileBlocks)) {
   continue;
  }
  const float offset[3] = {static_cast<float>(minX - eyeX_), static_cast<float>(-eyeY_),
                           static_cast<float>(minZ - eyeZ_)};
  program->set3fAt(modelOffset, offset);
  quad_index::ensure(allocation->vertexCount);
  gl::GLCore::drawElementsBaseVertex(gl::prim::Triangles, static_cast<int>(allocation->vertexCount / 4u * 6u),
                                     gl::pixel::UnsignedInt, nullptr, static_cast<int>(allocation->baseVertex()));
  ++drawn;
 }
 core::unbindVertexArray();
 if(scene != nullptr) {
  scene->colorTargets.bindGbuffersWithDepth(0);
 }
 if(!water) drawnLastFrame_ = drawn;
}
void DistantRenderer::renderOpaque(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler) {
 draw(pipeline, settings, culler, false);
}
void DistantRenderer::renderWater(Pipeline& pipeline, const option::RenderSettings& settings, const Frustum* culler) {
 draw(pipeline, settings, culler, true);
}
std::string DistantRenderer::debugInfo() const {
 constexpr std::size_t kMiB = 1024u * 1024u;
 return "Distant: tiles " + std::to_string(tiles_.size()) + ", drawn " + std::to_string(drawnLastFrame_) +
        ", flight " + std::to_string(inFlight_) + ", slab " + std::to_string(slab_.usedBytes() / kMiB) + "/" +
        std::to_string(slab_.capacityBytes() / kMiB) + " MiB";
}
} // namespace net::minecraft::client::render::distant
