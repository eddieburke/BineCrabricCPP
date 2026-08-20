#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft {
class Packet;
}
namespace net::minecraft::client::render::entity {
class EntityRenderer;
}
namespace net::minecraft::mod::lua {
class LuaModEntity : public entity::Entity {
 public:
 static constexpr int kRegistryIdTrackerKey = 1;
 static constexpr int kDataTrackerKey = 2;
 explicit LuaModEntity(entity::World* world = nullptr);
 [[nodiscard]] const std::string& registryId() const {
  return registryId_;
 }
 void setRegistryId(std::string id);
 void setClientLocal(bool value) noexcept {
  clientLocal_ = value;
 }
 [[nodiscard]] bool isClientLocal() const noexcept {
  return clientLocal_;
 }
 [[nodiscard]] NbtCompound& data() {
  return data_;
 }
 [[nodiscard]] const NbtCompound& data() const {
  return data_;
 }
 void setData(const NbtCompound& value);
 void writeNbt(NbtCompound& nbt) const override;
 void readNbt(const NbtCompound& nbt) override;
 void onTrackedDataUpdated(int key) override;
 void tick() override;
 void setPositionAndAnglesAvoidEntities(
     double x, double y, double z, float yaw, float pitch, int interpolationSteps) override;
 [[nodiscard]] bool isCollidable() const override {
  return !dead;
 }
 [[nodiscard]] std::optional<Box> getBoundingBox() const override {
  return boundingBox;
 }
 [[nodiscard]] bool takeDirty();
 [[nodiscard]] std::unique_ptr<net::minecraft::Packet> createUpdatePacket() const;
 struct ClientRenderer {
  static std::unique_ptr<net::minecraft::client::render::entity::EntityRenderer> create();
 };

  private:
  int clientInterpolationSteps_ = 0;
 double clientX_ = 0.0;
 double clientY_ = 0.0;
 double clientZ_ = 0.0;
 double clientTargetYaw_ = 0.0;
 double clientTargetPitch_ = 0.0;
 std::string registryId_;
 NbtCompound data_;
 bool dirty_ = false;
 bool clientLocal_ = false;
};
void registerLuaModEntityType();
} // namespace net::minecraft::mod::lua
