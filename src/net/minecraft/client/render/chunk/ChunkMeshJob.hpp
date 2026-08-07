#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::chunk {
class ChunkBuilder;
// CPU-side output of a chunk mesh rebuild: one captured tessellation per
// render layer plus everything the main thread needs at upload time.
struct ChunkMeshResult {
 std::array<TessellatorMesh, terrain_layer::Count> layers{};
 std::array<std::vector<ModChunkMesh>, terrain_layer::Count> modLayers{};
 std::array<bool, terrain_layer::Count> layerEmpty{true, true, true, true};
 bool hasSkyLight = false;
 // 6x6 face-to-face connectivity through non-opaque blocks (bit a*6+b), fed
 // to the per-frame occlusion BFS. All-ones = fully see-through.
 std::uint64_t visibilityBits = ~0ULL;
 // World positions of blocks that may own a renderable BlockEntity; the
 // main thread resolves live pointers at upload.
 std::vector<net::minecraft::Vec3i> blockEntityPositions;
};
// Everything a worker needs to rebuild one 16^3 section, fully detached from
// live world state. Created on the main thread, processed on a worker, then
// handed back to the main thread for the GL upload.
struct ChunkMeshJob {
 ~ChunkMeshJob();
 // The section this job was captured from. Weak on purpose: the job must not
 // keep an evicted section alive (its GL buffers are freed on the main thread
 // at eviction), and must not be able to dereference a dead one. Workers never
 // touch it — buildMesh reads only this job's snapshot/opts.
 std::weak_ptr<ChunkBuilder> builder;
 int version = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 std::unique_ptr<RegionSnapshot> snapshot;
 client::option::RenderSettings opts{};
 ChunkMeshResult result{};
 // Set when the worker hit an exception; the main thread reschedules.
 bool failed = false;
 // Built on the dedicated near-camera worker; its upload skips the
 // per-frame time budget so block edits next to the player land the
 // frame their mesh finishes.
 bool nearLane = false;
 std::unordered_map<int, int> blockRenderLayers;
 [[nodiscard]] static std::shared_ptr<ChunkMeshJob> capture(ChunkBuilder& owner,
                                                            client::option::RenderSettings options);
 void captureSnapshot();
 void releasePins() noexcept;

 private:
 explicit ChunkMeshJob(ChunkBuilder& owner,
                       client::option::RenderSettings options,
                       std::vector<RegionSnapshot::SourceChunk> sourceChunks,
                       int ambientDarkness,
                       const std::array<float, 16>& lightLevelToLuminance,
                       std::unique_ptr<net::minecraft::BiomeSource> biomeSource);
 std::vector<RegionSnapshot::SourceChunk> sourceChunks_;
 int ambientDarkness_ = 0;
 std::array<float, 16> lightLevelToLuminance_{};
 std::unique_ptr<net::minecraft::BiomeSource> biomeSource_;
 bool pinsReleased_ = false;
};
} // namespace net::minecraft::client::render::chunk
