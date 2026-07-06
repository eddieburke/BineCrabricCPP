#pragma once
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
namespace net::minecraft::entity::mob {
class GiantEntity : public MonsterEntity {
public:
  static constexpr int kEntityId = 53;
  static constexpr const char* kEntityName = "Giant";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit GiantEntity(World* world = nullptr);

protected:
  float getPathfindingFavor(int x, int y, int z) const override;
};
} // namespace net::minecraft::entity::mob
