#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "net/minecraft/entity/ai/pathing/PathNode.hpp"
namespace net::minecraft::entity::ai::pathing {
// see third_party/mcp/net/minecraft/src/Pathfinder.java:168
class PathNodeCache {
 public:
 PathNodeCache() {
  slots_.resize(kInitialCapacity);
 }
 PathNode* findOrCreate(int x, int y, int z) {
  const std::uint64_t key = PathNode::keyOf(x, y, z);
  if (count_ * 4 >= slots_.size() * 3) {
   grow();
  }
  std::size_t index = mixKey(key) & mask();
  while (true) {
   Slot& slot = slots_[index];
   if (!slot.used) {
    slot.used = true;
    slot.key = key;
    slot.node = createNode(x, y, z);
    ++count_;
    return slot.node;
   }
   if (slot.key == key) {
    return slot.node;
   }
   index = (index + 1) & mask();
  }
 }
 void clear() {
  for (Slot& slot : slots_) {
   slot.used = false;
  }
  for (std::vector<PathNode>& slab : slabs_) {
   slab.clear();
  }
  count_ = 0;
  slabIndex_ = 0;
 }

 private:
 struct Slot {
  std::uint64_t key = 0;
  PathNode* node = nullptr;
  bool used = false;
 };
 static constexpr std::size_t kInitialCapacity = 512;
 static constexpr std::size_t kSlabSize = 512;
 static std::uint64_t mixKey(std::uint64_t z) noexcept {
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
 }
 std::size_t mask() const {
  return slots_.size() - 1;
 }
 PathNode* createNode(int x, int y, int z) {
  if (slabIndex_ >= slabs_.size()) {
   slabs_.emplace_back();
   slabs_.back().reserve(kSlabSize);
  }
  std::vector<PathNode>& slab = slabs_[slabIndex_];
  if (slab.size() >= kSlabSize) {
   ++slabIndex_;
   return createNode(x, y, z);
  }
  slab.emplace_back(x, y, z);
  return &slab.back();
 }
 void grow() {
  std::vector<Slot> grown(slots_.size() * 2);
  const std::size_t grownMask = grown.size() - 1;
  for (Slot& slot : slots_) {
   if (!slot.used) {
    continue;
   }
   std::size_t index = mixKey(slot.key) & grownMask;
   while (grown[index].used) {
    index = (index + 1) & grownMask;
   }
   grown[index].used = true;
   grown[index].key = slot.key;
   grown[index].node = slot.node;
  }
  slots_ = std::move(grown);
 }
 std::vector<Slot> slots_;
 std::vector<std::vector<PathNode>> slabs_;
 std::size_t slabIndex_ = 0;
 std::size_t count_ = 0;
};
} // namespace net::minecraft::entity::ai::pathing
