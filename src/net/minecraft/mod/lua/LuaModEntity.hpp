#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include "net/minecraft/block/entity/BlockEntity.hpp"
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
  // Client replica only: feed interpolation targets from the same generic
  // move/rotate packets (and resent snapshots) every packet-driven entity gets,
  // so the replica glides instead of snapping. Mirrors the MinecartEntity /
  // BoatEntity client-side pattern; the server instance never calls this.
  void setPositionAndAnglesAvoidEntities(
      double x, double y, double z, float yaw, float pitch, int interpolationSteps) override;
  [[nodiscard]] bool isCollidable() const override {
    return !dead;
  }
  [[nodiscard]] std::optional<Box> getBoundingBox() const override {
    return boundingBox;
  }
  [[nodiscard]] virtual bool isModEntity() const {
    return true;
  }
  [[nodiscard]] bool takeDirty();
  [[nodiscard]] std::unique_ptr<net::minecraft::Packet> createUpdatePacket() const;
  struct ClientRenderer {
    static std::unique_ptr<net::minecraft::client::render::entity::EntityRenderer> create();
  };

private:
  // Client-side interpolation targets (MinecartEntity/BoatEntity pattern). Unused
  // on the server, which is the simulating authority and never sets these.
  int clientInterpolationSteps_ = 0;
  double clientX_ = 0.0;
  double clientY_ = 0.0;
  double clientZ_ = 0.0;
  double clientTargetYaw_ = 0.0;
  double clientTargetPitch_ = 0.0;
  std::string registryId_;
  NbtCompound data_;
  bool dirty_ = false;
};
class LuaModBlockEntity : public block::entity::BlockEntity {
public:
  explicit LuaModBlockEntity(std::string registryId) : registryId_(std::move(registryId)) {
  }
  [[nodiscard]] std::string id() const override {
    return registryId_;
  }
  [[nodiscard]] NbtCompound& data() noexcept {
    return data_;
  }
  [[nodiscard]] const NbtCompound& data() const noexcept {
    return data_;
  }
  void writeNbt(NbtCompound& nbt) const override {
    BlockEntity::writeNbt(nbt);
    nbt.put("Data", data_);
  }
  void readNbt(const NbtCompound& nbt) override {
    BlockEntity::readNbt(nbt);
    if(nbt.contains("Data")) {
      data_ = nbt.getCompound("Data");
    }
  }
  [[nodiscard]] std::unique_ptr<net::minecraft::Packet> createUpdatePacket() const override;

private:
  std::string registryId_;
  NbtCompound data_;
};
void registerLuaModEntityType();
} // namespace net::minecraft::mod::lua
