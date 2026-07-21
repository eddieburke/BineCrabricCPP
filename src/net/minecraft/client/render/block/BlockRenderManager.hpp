#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
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
 explicit BlockRenderManager(const net::minecraft::BlockView* view = nullptr) {
  ctx.blockView = view;
  ctx.tess = &Tessellator::INSTANCE;
  snapshotGlobals();
 }
 explicit BlockRenderManager(net::minecraft::World* world) {
  setBlockView(world);
  ctx.tess = &Tessellator::INSTANCE;
  snapshotGlobals();
 }
 // Worker-thread construction: render settings were captured on the main
 // thread at job-enqueue time; never touch Minecraft::INSTANCE here.
 BlockRenderManager(const net::minecraft::BlockView* view,
                    const option::ResolvedRenderOptions& opts) {
  ctx.blockView = view;
  ctx.opts = opts;
 }
 // Copy the global render settings (resolved GameOptions, fancy flags) into
 // the context. Mesh jobs overwrite ctx.opts with values captured at enqueue
 // time instead.
 void snapshotGlobals();
 void setBlockView(const net::minecraft::BlockView* view) {
  ctx.blockView = view;
 }
 void setBlockView(net::minecraft::World* world) {
  ctx.blockView = world;
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
 void renderFallingBlockEntity(
     net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z);
 bool renderStandardBlock(net::minecraft::block::Block& block, int x, int y, int z);
 CubeBlockRenderer& cubeRenderer() {
  return cube_;
 }
 CrossBlockRenderer& crossRenderer() {
  return cross_;
 }
 TorchBlockRenderer& torchRenderer() {
  return torch_;
 }
 BlockFaceRenderer& faceRenderer() {
  return faces_;
 }
 // Shared mutable render state (texture overrides, face rotations, AO, ...).
 BlockRenderContext ctx;
 inline static bool fancyLeaves = true;

 private:
 BlockFaceRenderer faces_{ctx};
 CubeBlockRenderer cube_{ctx, faces_};
 FluidBlockRenderer fluid_{ctx, faces_};
 CrossBlockRenderer cross_{ctx};
 CropBlockRenderer crop_{ctx};
 TorchBlockRenderer torch_{ctx};
 FireBlockRenderer fire_{ctx};
 RedstoneDustBlockRenderer redstoneDust_{ctx};
 LadderBlockRenderer ladder_{ctx};
 RailBlockRenderer rail_{ctx};
 DoorBlockRenderer door_{ctx, faces_};
 StairsBlockRenderer stairs_{ctx, cube_};
 FenceBlockRenderer fence_{ctx, cube_};
 BedBlockRenderer bed_{ctx, faces_};
 LeverBlockRenderer lever_{ctx, cube_};
 RepeaterBlockRenderer repeater_{ctx, cube_, torch_};
 PistonBlockRenderer piston_{ctx, cube_};
 InventoryBlockRenderer inventory_{ctx, faces_, cross_, crop_, torch_};
 FallingBlockRenderer falling_{ctx, faces_};
};
} // namespace net::minecraft::client::render::block
