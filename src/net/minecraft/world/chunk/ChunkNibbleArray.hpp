#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
// Nibble accessors are byte-atomic: the lighting thread and the main thread
// (Chunk::updateHeightMap sky columns) write light arrays concurrently, and
// two nibbles share a byte — a plain read-modify-write could drop the other
// nibble's update. set() uses a CAS loop; get() is a relaxed byte load.
// Bulk users of `bytes` (save, network) still copy non-atomically; torn
// nibbles there are transient and corrected by follow-up light updates.
class ChunkNibbleArray {
 public:
 explicit ChunkNibbleArray(int size) : bytes(static_cast<std::size_t>(size >> 1), 0) {
 }
 explicit ChunkNibbleArray(std::vector<std::uint8_t> data) : bytes(std::move(data)) {
 }
 [[nodiscard]] int get(int x, int y, int z) const {
  const int index = (x << 11) | (z << 7) | y;
  const int byteIndex = index >> 1;
  if(byteIndex < 0 || static_cast<std::size_t>(byteIndex) >= bytes.size()) {
   return 0;
  }
  const int nibble = index & 1;
  const std::uint8_t byte = std::atomic_ref(const_cast<std::uint8_t&>(bytes[static_cast<std::size_t>(byteIndex)]))
                                .load(std::memory_order_relaxed);
  if(nibble == 0) {
   return static_cast<int>(byte & 0x0F);
  }
  return static_cast<int>((byte >> 4) & 0x0F);
 }
 void set(int x, int y, int z, int value) {
  const int index = (x << 11) | (z << 7) | y;
  const int byteIndex = index >> 1;
  if(byteIndex < 0 || static_cast<std::size_t>(byteIndex) >= bytes.size()) {
   return;
  }
  const int nibble = index & 1;
  value &= 0x0F;
  std::atomic_ref<std::uint8_t> byte(bytes[static_cast<std::size_t>(byteIndex)]);
  std::uint8_t expected = byte.load(std::memory_order_relaxed);
  std::uint8_t desired;
  do {
   desired = nibble == 0
                 ? static_cast<std::uint8_t>((expected & 0xF0) | value)
                 : static_cast<std::uint8_t>((expected & 0x0F) | static_cast<std::uint8_t>(value << 4));
  } while(!byte.compare_exchange_weak(expected, desired, std::memory_order_relaxed));
 }
 void ensureSizeForBlockCount(std::size_t blockCount) {
  const std::size_t expectedBytes = blockCount / 2;
  if(bytes.size() == expectedBytes) {
   return;
  }
  std::vector<std::uint8_t> normalized(expectedBytes, 0);
  if(!bytes.empty()) {
   std::copy_n(bytes.begin(), std::min(bytes.size(), expectedBytes), normalized.begin());
  }
  bytes = std::move(normalized);
 }
 [[nodiscard]] bool isArrayInitialized() const noexcept {
  return !bytes.empty();
 }
 [[nodiscard]] bool hasExpectedSizeForBlockCount(std::size_t blockCount) const noexcept {
  return bytes.size() == blockCount / 2;
 }
 [[nodiscard]] bool isAllZero() const noexcept {
  for(std::uint8_t value : bytes) {
   if(value != 0) {
    return false;
   }
  }
  return true;
 }
 std::vector<std::uint8_t> bytes;
};
} // namespace net::minecraft
