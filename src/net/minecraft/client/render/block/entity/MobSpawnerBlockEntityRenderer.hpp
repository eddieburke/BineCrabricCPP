#pragma once
#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include <memory>
#include <string>
#include <unordered_map>
namespace net::minecraft::client::render::block::entity {
class MobSpawnerBlockEntityRenderer : public BlockEntityRenderer {
public:
  void render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z,
              float tickDelta) override;
  std::unordered_map<std::string, std::unique_ptr<net::minecraft::Entity>> models{};
};
} // namespace net::minecraft::client::render::block::entity
