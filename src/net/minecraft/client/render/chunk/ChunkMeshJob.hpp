#pragma once

#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace net::minecraft::client::render::chunk {

class ChunkBuilder;

// CPU-side output of a chunk mesh rebuild: one captured tessellation per
// render layer plus everything the main thread needs at upload time.
struct ChunkMeshResult {
    std::array<TessellatorMesh, 2> layers {};
    std::array<bool, 2> layerEmpty {true, true};
    bool hasSkyLight = false;
    // World positions of blocks that may own a renderable BlockEntity; the
    // main thread resolves live pointers at upload.
    std::vector<net::minecraft::Vec3i> blockEntityPositions;
};

// Everything a worker needs to rebuild one 16^3 section, fully detached from
// live world state. Created on the main thread, processed on a worker, then
// handed back to the main thread for the GL upload.
struct ChunkMeshJob {
    ~ChunkMeshJob();

    // Owning ChunkBuilder; dereferenced only on the main thread, and only
    // after validating version/position (the renderer cancels all jobs before
    // destroying its builders).
    ChunkBuilder* builder = nullptr;
    int version = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;
    std::unique_ptr<RegionSnapshot> snapshot;
    client::option::ResolvedRenderOptions opts {};
    bool fancyGraphics = true;
    ChunkMeshResult result {};
    // Set when the worker hit an exception; the main thread reschedules.
    bool failed = false;
    // Built on the dedicated near-camera worker; its upload skips the
    // per-frame time budget so block edits next to the player land the
    // frame their mesh finishes.
    bool nearLane = false;

    [[nodiscard]] static std::shared_ptr<ChunkMeshJob> capture(
        ChunkBuilder& owner, client::option::ResolvedRenderOptions options, bool fancyGraphics);
    [[nodiscard]] bool captureSnapshot();
    void releasePins() noexcept;

private:
    explicit ChunkMeshJob(ChunkBuilder& owner, client::option::ResolvedRenderOptions options, bool fancyGraphics,
        std::vector<RegionSnapshot::SourceChunk> sourceChunks, int ambientDarkness,
        const std::array<float, 16>& lightLevelToLuminance, std::unique_ptr<net::minecraft::BiomeSource> biomeSource);

    std::vector<RegionSnapshot::SourceChunk> sourceChunks_;
    int ambientDarkness_ = 0;
    std::array<float, 16> lightLevelToLuminance_ {};
    std::unique_ptr<net::minecraft::BiomeSource> biomeSource_;
    bool pinsReleased_ = false;
};

} // namespace net::minecraft::client::render::chunk
