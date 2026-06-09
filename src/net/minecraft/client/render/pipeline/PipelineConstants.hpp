#pragma once

#include <cstddef>
#include <cstdint>

namespace net::minecraft::client::render::pipeline {

// Java WorldRenderer.compileChunks hardcodes n6 = 2 for the distant rebuild queue.
constexpr int kDistantRebuildSlots = 2;

// Java: squaredDistanceTo(camera) > 256.0f marks a chunk as "distant".
constexpr float kNearChunkRebuildDistSq = 256.0f;

// Java WorldRenderer.render: re-sort when camera delta exceeds 4 blocks (16 = 4^2).
constexpr double kCameraResortDistanceSq = 16.0;

// Java: new ChunkRenderer[4] — one batch renderer per distinct camera-offset group.
constexpr std::size_t kCameraOffsetGroupCount = 4;

// Enforced ordering for a single world-render slice inside GameRenderer.renderFrame.
enum class FramePhase : std::uint8_t {
    Idle = 0,
    Culled,
    Compiled,
    SolidLayerDone,
};

} // namespace net::minecraft::client::render::pipeline
