#pragma once
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"
namespace net::minecraft::client::render::block::entity {
class PistonBlockEntityRenderer : public BlockEntityRenderer {
public:
  void render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z,
              float tickDelta) override;
  void setWorld(net::minecraft::World* world) override;
  net::minecraft::client::render::block::BlockRenderManager blockRenderManager{};
};
} // namespace net::minecraft::client::render::block::entity
