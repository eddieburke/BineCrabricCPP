#pragma once
#include "net/minecraft/entity/AbstractLightningEntity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
namespace net::minecraft::entity {
class LightningEntity : public AbstractLightningEntity {
public:
  static constexpr int kEntityId = 99;
  static constexpr const char* kEntityName = "LightningBolt";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit LightningEntity(World* world = nullptr);
  LightningEntity(World* world, double x, double y, double z);
  void tick() override;
  long long seed = 0;

private:
  int ambientTick = 2;
  int remainingActions = 0;
};
} // namespace net::minecraft::entity
