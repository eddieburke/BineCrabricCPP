#pragma once
#include <cstdint>
namespace net::minecraft::util {
// Faithful port of net.minecraft.util.LongObjectHashMapEntry (beta 1.7.3).
// Templated on the stored value type (Java's Object).
template <typename V>
class LongObjectHashMapEntry {
public:
  const std::int64_t key;
  V value;
  LongObjectHashMapEntry* next;
  const int hash;
  LongObjectHashMapEntry(int hash, std::int64_t key, V value, LongObjectHashMapEntry* next)
      : key(key), value(value), next(next), hash(hash) {
  }
  [[nodiscard]] std::int64_t getKey() const {
    return this->key;
  }
  [[nodiscard]] const V& getValue() const {
    return this->value;
  }
  bool operator==(const LongObjectHashMapEntry& other) const {
    return this->key == other.key && this->value == other.value;
  }
  [[nodiscard]] int hashCode() const;
};
} // namespace net::minecraft::util
