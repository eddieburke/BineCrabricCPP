#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::entity::vehicle {
class MinecartEntity : public Entity, public Inventory {
public:
  static constexpr int kEntityId = 40;
  static constexpr const char* kEntityName = "Minecart";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit MinecartEntity(World* world = nullptr);
  MinecartEntity(World* world, double x, double y, double z, int typeIn);
  void tick() override;
  void markDead() override;
  void setPositionAndAnglesAvoidEntities(
      double x, double y, double z, float yaw, float pitch, int interpolationSteps) override;
  void setVelocityClient(double x, double y, double z) override;
  bool damage(Entity* damageSource, int amount) override;
  void onCollision(Entity* otherEntity) override;
  bool interact(player::PlayerEntity* player) override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  [[nodiscard]] bool isPushable() const override {
    return true;
  }
  [[nodiscard]] std::optional<Box> getBoundingBox() const override {
    return std::nullopt;
  }
  [[nodiscard]] std::optional<Box> getCollisionAgainstShape(Entity* other) const override;
  [[nodiscard]] double getPassengerRidingHeight() const override {
    return static_cast<double>(height) * 0.0 - 0.3;
  }
  [[nodiscard]] float getShadowRadius() const override {
    return 0.0f;
  }
  std::size_t size() const override {
    return 27;
  }
  ItemStack getStack(std::size_t slot) const override;
  ItemStack removeStack(std::size_t slot, int amount) override;
  void setStack(std::size_t slot, ItemStack stack) override;
  [[nodiscard]] std::string getName() const override {
    return "Minecart";
  }
  [[nodiscard]] int getMaxCountPerStack() const override {
    return 64;
  }
  void markDirty() override {
  }
  [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override;
  [[nodiscard]] std::optional<Vec3d> snapPositionToRail(double xIn, double yIn, double zIn) const;
  [[nodiscard]] std::optional<Vec3d> snapPositionToRailWithOffset(double xIn,
                                                                  double yIn,
                                                                  double zIn,
                                                                  double offset) const;
  int damageWobbleTicks = 0;
  float damageWobbleStrength = 0.0f;
  int damageWobbleSide = 1;
  int type = 0;
  int fuel = 0;
  double pushX = 0.0;
  double pushZ = 0.0;

private:
  void dropInventoryContents();
  bool yawFlipped = false;
  int clientInterpolationSteps = 0;
  double clientX = 0.0;
  double clientY = 0.0;
  double clientZ = 0.0;
  // Java (CFR decompile) named these clientPitch/clientYaw, but the values are
  // swapped: clientPitch holds the interpolation-target yaw and clientYaw the
  // target pitch. Renamed to reflect the actual contents.
  double clientTargetYaw = 0.0;   // Java: clientPitch
  double clientTargetPitch = 0.0; // Java: clientYaw
  double clientVelocityX = 0.0;
  double clientVelocityY = 0.0;
  double clientVelocityZ = 0.0;
  std::vector<ItemStack> inventory_;
};
} // namespace net::minecraft::entity::vehicle
