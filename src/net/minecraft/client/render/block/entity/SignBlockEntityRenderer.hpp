#pragma once
#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"
#include "net/minecraft/client/render/block/entity/SignModel.hpp"
namespace net::minecraft::client::render::block::entity {
class SignBlockEntityRenderer : public BlockEntityRenderer {
 public:
 void render(const net::minecraft::block::entity::BlockEntity& blockEntity,
             double x,
             double y,
             double z,
             float tickDelta) override;
 SignModel model{};
};
} // namespace net::minecraft::client::render::block::entity
