#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BedBlockRenderer.hpp"
#include "net/minecraft/client/render/block/BlockFaceRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/CropBlockRenderer.hpp"
#include "net/minecraft/client/render/block/CrossBlockRenderer.hpp"
#include "net/minecraft/client/render/block/CubeBlockRenderer.hpp"
#include "net/minecraft/client/render/block/DoorBlockRenderer.hpp"
#include "net/minecraft/client/render/block/FallingBlockRenderer.hpp"
#include "net/minecraft/client/render/block/FenceBlockRenderer.hpp"
#include "net/minecraft/client/render/block/FireBlockRenderer.hpp"
#include "net/minecraft/client/render/block/FluidBlockRenderer.hpp"
#include "net/minecraft/client/render/block/InventoryBlockRenderer.hpp"
#include "net/minecraft/client/render/block/LadderBlockRenderer.hpp"
#include "net/minecraft/client/render/block/LeverBlockRenderer.hpp"
#include "net/minecraft/client/render/block/PistonBlockRenderer.hpp"
#include "net/minecraft/client/render/block/RailBlockRenderer.hpp"
#include "net/minecraft/client/render/block/RedstoneDustBlockRenderer.hpp"
#include "net/minecraft/client/render/block/RepeaterBlockRenderer.hpp"
#include "net/minecraft/client/render/block/StairsBlockRenderer.hpp"
#include "net/minecraft/client/render/block/TorchBlockRenderer.hpp"
#include "net/minecraft/world/BlockView.hpp"
// M2.5 exception: World.hpp retained — WorldBlockViewAdapter stores World* and
// WorldBlockViewAdapter.hpp already includes World.hpp. std::optional<WorldBlockViewAdapter>
// member requires the complete type; a forward declaration is insufficient here.
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/WorldBlockViewAdapter.hpp"

#include <optional>

namespace net::minecraft {
class World;
}

namespace net::minecraft::block {
class RailBlock;
}

namespace net::minecraft::client::render::block {

// Faithful port of net.minecraft.client.render.block.BlockRenderManager (beta 1.7.3).
class BlockRenderManager {
public:
    explicit BlockRenderManager(const net::minecraft::BlockView* view = nullptr)
    {
        ctx.blockView = view;
        ctx.tess = &Tessellator::INSTANCE;
        snapshotGlobals();
    }

    explicit BlockRenderManager(net::minecraft::World* world)
    {
        setBlockView(world);
        ctx.tess = &Tessellator::INSTANCE;
        snapshotGlobals();
    }

    // Worker-thread construction: render settings were captured on the main
    // thread at job-enqueue time; never touch Minecraft::INSTANCE here.
    BlockRenderManager(const net::minecraft::BlockView* view, const option::ResolvedRenderOptions& opts,
        bool useFancyGraphics)
    {
        ctx.blockView = view;
        ctx.opts = opts;
        ctx.fancyGraphics = useFancyGraphics;
    }

    // Copy the global render settings (resolved GameOptions, fancy flags) into
    // the context. Mesh jobs overwrite ctx.opts/ctx.fancyGraphics with values
    // captured at enqueue time instead.
    void snapshotGlobals();

    // Snapshot resolved render options from the live client, or defaults when
    // Minecraft::INSTANCE is unset (sync rebuild fallback).
    [[nodiscard]] static option::ResolvedRenderOptions blockRenderManagerOptionsSnapshot();

    void setBlockView(const net::minecraft::BlockView* view)
    {
        worldAdapter_.reset();
        ctx.blockView = view;
    }

    void setBlockView(net::minecraft::World* world)
    {
        if (world == nullptr) {
            worldAdapter_.reset();
            ctx.blockView = nullptr;
            return;
        }
        worldAdapter_.emplace(world);
        ctx.blockView = &worldAdapter_.value();
    }

    void renderWithTexture(int blockId, int x, int y, int z, int textureOverride);
    void renderWithoutCulling(int blockId, int x, int y, int z);
    void renderExtendedPiston(int blockId, int x, int y, int z);
    void renderPistonHeadWithoutCulling(int blockId, int x, int y, int z, bool extendedHalfway);

    bool render(int blockId, int x, int y, int z);
    void render(int blockId, int metadata, float brightness);
    bool renderBlock(int blockId, int x, int y, int z);

    void renderWithTexture(net::minecraft::block::Block& block, int x, int y, int z, int textureOverride);
    void renderWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z);
    void renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z);
    void renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway);
    void render(net::minecraft::block::Block& block, int metadata, float brightness);
    bool render(net::minecraft::block::Block& block, int x, int y, int z);

    [[nodiscard]] static bool isSideLit(int renderType);

    void renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z);

    // Shared mutable render state (texture overrides, face rotations, AO, ...).
    BlockRenderContext ctx;
    std::optional<net::minecraft::WorldBlockViewAdapter> worldAdapter_;
    inline static bool fancyGraphics = true;
    inline static bool fancyLeaves = true;

private:
    BlockFaceRenderer faces_ { ctx };
    CubeBlockRenderer cube_ { ctx, faces_ };
    FluidBlockRenderer fluid_ { ctx, faces_ };
    CrossBlockRenderer cross_ { ctx };
    CropBlockRenderer crop_ { ctx };
    TorchBlockRenderer torch_ { ctx };
    FireBlockRenderer fire_ { ctx };
    RedstoneDustBlockRenderer redstoneDust_ { ctx };
    LadderBlockRenderer ladder_ { ctx };
    RailBlockRenderer rail_ { ctx };
    DoorBlockRenderer door_ { ctx, faces_ };
    StairsBlockRenderer stairs_ { ctx, cube_ };
    FenceBlockRenderer fence_ { ctx, cube_ };
    BedBlockRenderer bed_ { ctx, faces_ };
    LeverBlockRenderer lever_ { ctx, cube_ };
    RepeaterBlockRenderer repeater_ { ctx, cube_, torch_ };
    PistonBlockRenderer piston_ { ctx, cube_ };
    InventoryBlockRenderer inventory_ { ctx, faces_, cross_, crop_, torch_ };
    FallingBlockRenderer falling_ { ctx, faces_ };
};

} // namespace net::minecraft::client::render::block
