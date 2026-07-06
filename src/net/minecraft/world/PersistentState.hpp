#pragma once
#include "net/minecraft/nbt/NbtCompound.hpp"
#include <string>
namespace net::minecraft {
class PersistentState {
public:
  explicit PersistentState(std::string id) : id(std::move(id)) {}
  virtual ~PersistentState() = default;
  virtual void readNbt(const NbtCompound& nbt) = 0;
  virtual void writeNbt(NbtCompound& nbt) const = 0;
  void markDirty() {
    setDirty(true);
  }
  void setDirty(bool value) noexcept {
    dirty = value;
  }
  [[nodiscard]] bool isDirty() const noexcept {
    return dirty;
  }
  const std::string id;

private:
  bool dirty = false;
};
} // namespace net::minecraft
