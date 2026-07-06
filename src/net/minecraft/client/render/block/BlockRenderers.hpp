#pragma once
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::block {
class Block;
class RailBlock;
namespace material {
class Material;
}
} // namespace net::minecraft::block
namespace net::minecraft::client::render::block {
class BlockFaceRenderer {
public:
  explicit BlockFaceRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  void renderBottomFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
  void renderTopFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
  void renderEastFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
  void renderWestFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
  void renderNorthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);
  void renderSouthFace(net::minecraft::block::Block& block, double x, double y, double z, int texture);

private:
  BlockRenderContext& ctx_;
};
class CubeBlockRenderer {
public:
  CubeBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}
  bool renderBlock(net::minecraft::block::Block& block, int x, int y, int z);
  bool renderSmooth(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);
  bool renderFlat(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);
  bool renderCactus(net::minecraft::block::Block& block, int x, int y, int z);
  bool renderCactus(net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue);

private:
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
};
class CrossBlockRenderer {
public:
  explicit CrossBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);
  void render(net::minecraft::block::Block& block, int metadata, double x, double y, double z);

private:
  void render(net::minecraft::block::Block& block, int metadata, double x, double y, double z, int lightX, int lightY,
              int lightZ);
  BlockRenderContext& ctx_;
};
class CropBlockRenderer {
public:
  explicit CropBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);
  void render(net::minecraft::block::Block& block, int metadata, double x, double y, double z);

private:
  BlockRenderContext& ctx_;
};
class TorchBlockRenderer {
public:
  explicit TorchBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);
  void renderTiltedTorch(net::minecraft::block::Block& block, double x, double y, double z, double xTilt,
                         double zTilt);

private:
  BlockRenderContext& ctx_;
};
class FluidBlockRenderer {
public:
  FluidBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}
  bool renderFluid(net::minecraft::block::Block& block, int x, int y, int z);

private:
  float getFluidHeight(int x, int y, int z, net::minecraft::block::material::Material& material);
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
};
class BedBlockRenderer {
public:
  BedBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
};
class DoorBlockRenderer {
public:
  DoorBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
};
class FallingBlockRenderer {
public:
  FallingBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces) : ctx_(ctx), faces_(faces) {}
  void renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y,
                                int z);

private:
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
};
class FenceBlockRenderer {
public:
  FenceBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube) : ctx_(ctx), cube_(cube) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  CubeBlockRenderer& cube_;
};
class LeverBlockRenderer {
public:
  LeverBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube) : ctx_(ctx), cube_(cube) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  CubeBlockRenderer& cube_;
};
class StairsBlockRenderer {
public:
  StairsBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube) : ctx_(ctx), cube_(cube) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  CubeBlockRenderer& cube_;
};
class RepeaterBlockRenderer {
public:
  RepeaterBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube, TorchBlockRenderer& torch)
      : ctx_(ctx), cube_(cube), torch_(torch) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
  CubeBlockRenderer& cube_;
  TorchBlockRenderer& torch_;
};
class PistonBlockRenderer {
public:
  PistonBlockRenderer(BlockRenderContext& ctx, CubeBlockRenderer& cube) : ctx_(ctx), cube_(cube) {}
  void renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z);
  bool renderPiston(net::minecraft::block::Block& block, int x, int y, int z, bool extended);
  void renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway);
  bool renderPistonHead(net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway);

private:
  void renderExtensionRod(int axis, double x1, double x2, double y1, double y2, double z1, double z2, float brightness,
                          double textureScrollU);
  BlockRenderContext& ctx_;
  CubeBlockRenderer& cube_;
};
class FireBlockRenderer {
public:
  explicit FireBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
};
class LadderBlockRenderer {
public:
  explicit LadderBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
};
class RailBlockRenderer {
public:
  explicit RailBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::RailBlock& rail, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
};
class RedstoneDustBlockRenderer {
public:
  explicit RedstoneDustBlockRenderer(BlockRenderContext& ctx) : ctx_(ctx) {}
  bool render(net::minecraft::block::Block& block, int x, int y, int z);

private:
  BlockRenderContext& ctx_;
};
class InventoryBlockRenderer {
public:
  InventoryBlockRenderer(BlockRenderContext& ctx, BlockFaceRenderer& faces, CrossBlockRenderer& cross,
                         CropBlockRenderer& crop, TorchBlockRenderer& torch)
      : ctx_(ctx), faces_(faces), cross_(cross), crop_(crop), torch_(torch) {}
  void render(net::minecraft::block::Block& block, int metadata, float brightness);

private:
  BlockRenderContext& ctx_;
  BlockFaceRenderer& faces_;
  CrossBlockRenderer& cross_;
  CropBlockRenderer& crop_;
  TorchBlockRenderer& torch_;
};
} // namespace net::minecraft::client::render::block
