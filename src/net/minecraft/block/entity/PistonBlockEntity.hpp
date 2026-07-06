#pragma once
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include <algorithm>
namespace net::minecraft::block::entity {
class PistonBlockEntity : public BlockEntity {
public:
  PistonBlockEntity() = default;
  PistonBlockEntity(int pushedBlockId, int pushedBlockData, int facing, bool extending, bool source);
  [[nodiscard]] int getPushedBlockId() const noexcept {
    return pushedBlockId_;
  }
  [[nodiscard]] int getPushedBlockData() const override {
    return pushedBlockData_;
  }
  [[nodiscard]] bool isExtending() const noexcept {
    return extending_;
  }
  [[nodiscard]] int getFacing() const noexcept {
    return facing_;
  }
  [[nodiscard]] bool isSource() const noexcept {
    return source_;
  }
  [[nodiscard]] float getProgress(float tickDelta) const noexcept;
  [[nodiscard]] float getRenderOffsetX(float tickDelta) const;
  [[nodiscard]] float getRenderOffsetY(float tickDelta) const;
  [[nodiscard]] float getRenderOffsetZ(float tickDelta) const;
  void finish();
  void tick() override;
  void readNbt(const NbtCompound& nbt) override;
  void writeNbt(NbtCompound& nbt) const override;
  [[nodiscard]] std::string id() const override {
    return "Piston";
  }

private:
  void pushEntities(float collisionShapeSizeMultiplier, float entityMoveMultiplier);
  void completeMovement();
  [[nodiscard]] float renderOffset(float tickDelta, int axisOffset) const;
  int pushedBlockId_ = 0;
  int pushedBlockData_ = 0;
  int facing_ = 0;
  bool extending_ = false;
  bool source_ = false;
  float lastProgress_ = 0.0f;
  float progress_ = 0.0f;
};
} // namespace net::minecraft::block::entity
