#pragma once

#include "net/minecraft/client/render/block/BlockFaceRenderState.hpp"

namespace net::minecraft {
class BlockView;
}

namespace net::minecraft::client::render::block {

// Mutable rendering state shared by the cooperating block renderers (faces,
// cube, fluid, plants, mechanisms, inventory icon). In the original beta 1.7.3
// RenderBlocks these were all fields of one god-class; pulling them into a
// context lets the renderers be split into focused classes while preserving the
// exact cross-method communication they relied on (texture overrides, face
// rotations, culling toggles, AO colours, ...).
struct BlockRenderContext {
    // World/chunk the blocks are read from while rendering in-place.
    const net::minecraft::BlockView* blockView = nullptr;

    // When >= 0, forces every face to this atlas tile (used by levers, pistons,
    // fire, redstone, ... that re-skin a block).
    int textureOverride = -1;

    // Mirrors a face's U axis (beds, some directional faces).
    bool flipTextureHorizontally = false;

    // Skip the neighbour-visibility test and always emit faces (item models,
    // extended pistons, ...).
    bool skipFaceCulling = false;

    // Per-face 90-degree texture rotations (0..3), set by directional blocks.
    int eastFaceRotation = 0;
    int westFaceRotation = 0;
    int southFaceRotation = 0;
    int northFaceRotation = 0;
    int topFaceRotation = 0;
    int bottomFaceRotation = 0;

    // Whether AO smooth lighting samples surrounding blocks.
    int useSurroundingBrightness = 1;

    // Smooth-lighting vertex colours produced for the face currently being drawn.
    BlockFaceRenderState faceState;

    // Apply the block's per-metadata colour multiplier when drawing inventory
    // icons (disabled by callers that tint themselves).
    bool inventoryColorEnabled = true;
};

} // namespace net::minecraft::client::render::block
