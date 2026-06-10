#pragma once

#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockFaceRenderState.hpp"
#include "net/minecraft/util/math/Types.hpp"

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

    // Tessellator the renderers emit geometry into. Defaults to the shared
    // main-thread instance; chunk-mesh worker jobs point this at a private
    // capture-only tessellator so meshing never touches GL or shared state.
    Tessellator* tess = &Tessellator::INSTANCE;

    // Bounds of the block currently being rendered, in local 0..1 block space.
    // Owned by the context (not the Block singleton) so concurrent mesh jobs
    // and the main-thread tick can't race on Block::minX..maxZ.
    Box renderBounds {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

    void setRenderBounds(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) noexcept
    {
        renderBounds = Box {minX, minY, minZ, maxX, maxY, maxZ};
    }

    // Render options snapshotted when the owning BlockRenderManager is created
    // (or when a mesh job is enqueued), so renderers never read the live
    // GameOptions from a worker thread.
    option::ResolvedRenderOptions opts {};

    // Per-context copy of BlockRenderManager::fancyGraphics.
    bool fancyGraphics = true;

    // When >= 0, forces every face to this atlas tile (used by levers, fire,
    // redstone, ... that re-skin a block).
    int textureOverride = -1;

    // When >= 0, replaces only faceTextureSide (0-5) — e.g. piston head top
    // during block-entity animation without retexturing the whole block.
    int faceTextureOverride = -1;
    int faceTextureSide = -1;

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

    [[nodiscard]] int resolveTexture(int side, int texture) const
    {
        if (faceTextureOverride >= 0 && side == faceTextureSide) {
            return faceTextureOverride;
        }
        if (textureOverride >= 0) {
            return textureOverride;
        }
        return texture;
    }
};

} // namespace net::minecraft::client::render::block
