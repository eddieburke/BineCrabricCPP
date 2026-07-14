#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
namespace net::minecraft::entity {
class TntEntity : public Entity {
public:
  static constexpr int kEntityId = 20;
  static constexpr const char* kEntityName = "PrimedTnt";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit TntEntity(World* world = nullptr);
  TntEntity(World* world, double x, double y, double z);
  void tick() override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] bool isCollidable() const override {
    return !dead;
  }
  [[nodiscard]] bool bypassesSteppingEffects() const override {
    return false;
  }
  int fuse = 0;
};
} // namespace net::minecraft::entity
