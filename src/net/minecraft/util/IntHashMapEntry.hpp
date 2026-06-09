#pragma once

#include <cstdint>
#include <string>

namespace net::minecraft::util {

// Faithful port of net.minecraft.util.IntHashMapEntry (beta 1.7.3).
// Templated on the stored value type (Java's Object).
template <typename V>
class IntHashMapEntry {
public:
    const int key;
    V value;
    IntHashMapEntry* next;
    const int hash;

    IntHashMapEntry(int hash, int key, V value, IntHashMapEntry* next)
        : key(key), value(value), next(next), hash(hash) {}

    [[nodiscard]] int getKey() const { return this->key; }

    [[nodiscard]] const V& getValue() const { return this->value; }

    bool operator==(const IntHashMapEntry& other) const {
        return this->key == other.key && this->value == other.value;
    }

    [[nodiscard]] int hashCode() const;
};

} // namespace net::minecraft::util
