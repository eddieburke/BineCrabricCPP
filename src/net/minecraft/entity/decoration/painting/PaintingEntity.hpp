#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingVariants.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::decoration::painting {
class PaintingEntity : public Entity {
public:
  static constexpr int kEntityId = 9;
  static constexpr const char* kEntityName = "Painting";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit PaintingEntity(World* world = nullptr);
  PaintingEntity(World* world, int x, int y, int z, int facingIn);
  PaintingEntity(World* world, int x, int y, int z, int facingIn, const std::string& variantId);
  void tick() override;
  void move(double dx, double dy, double dz) override;
  void addVelocity(double vx, double vy, double vz);
  bool damage(Entity* damageSource, int amount) override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] bool isCollidable() const override {
    return true;
  }
  [[nodiscard]] bool canStayAttached() const;
  void setFacing(int facingIn);
  int facing = 0;
  int attachmentX = 0;
  int attachmentY = 0;
  int attachmentZ = 0;
  PaintingVariant variant = KEBAB;

private:
  int obstructionCheckCounter = 0;
  [[nodiscard]] float getHorizontalOffset(int width) const;
  void dropPaintingItem();
};
} // namespace net::minecraft::entity::decoration::painting
