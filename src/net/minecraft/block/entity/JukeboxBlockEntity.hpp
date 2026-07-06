#pragma once
#include "net/minecraft/block/entity/BlockEntity.hpp"
namespace net::minecraft::block::entity {
class JukeboxBlockEntity : public BlockEntity {
public:
  void readNbt(const NbtCompound& nbt) override {
    BlockEntity::readNbt(nbt);
    recordId = nbt.getInt("Record");
  }
  void writeNbt(NbtCompound& nbt) const override {
    BlockEntity::writeNbt(nbt);
    if(recordId > 0) {
      nbt.putInt("Record", recordId);
    }
  }
  [[nodiscard]] std::string id() const override {
    return "RecordPlayer";
  }
  int recordId = 0;
};
} // namespace net::minecraft::block::entity
