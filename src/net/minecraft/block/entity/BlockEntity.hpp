#pragma once
#include <cmath>
#include <memory>
#include <string>
#include "net/minecraft/block/BlockTypes.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft {
class Packet;
class World;
} // namespace net::minecraft
namespace net::minecraft::block::entity {
class BlockEntity {
public:
  virtual ~BlockEntity() = default;
  virtual void readNbt(const NbtCompound& nbt);
  virtual void writeNbt(NbtCompound& nbt) const;
  virtual void tick() {
  }
  [[nodiscard]] virtual std::string id() const {
    return "BlockEntity";
  }
  [[nodiscard]] virtual int getPushedBlockData() const;
  [[nodiscard]] Block* getBlock() const;
  virtual void markDirty();
  [[nodiscard]] virtual std::unique_ptr<Packet> createUpdatePacket() const;
  [[nodiscard]] double distanceFrom(double targetX, double targetY, double targetZ) const {
    const double dx = static_cast<double>(x) + 0.5 - targetX;
    const double dy = static_cast<double>(y) + 0.5 - targetY;
    const double dz = static_cast<double>(z) + 0.5 - targetZ;
    return dx * dx + dy * dy + dz * dz;
  }
  [[nodiscard]] bool isRemoved() const noexcept {
    return removed_;
  }
  void markRemoved() noexcept {
    removed_ = true;
  }
  void cancelRemoval() noexcept {
    removed_ = false;
  }
  [[nodiscard]] static std::unique_ptr<BlockEntity> createFromNbt(const NbtCompound& nbt);
  World* world = nullptr;
  int x = 0;
  int y = 0;
  int z = 0;

protected:
  bool removed_ = false;
};
} // namespace net::minecraft::block::entity
