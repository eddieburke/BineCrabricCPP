#pragma once
#include "net/minecraft/util/IntHashMapEntry.hpp"
#include <cstdint>
#include <vector>
namespace net::minecraft::util {
// Faithful port of net.minecraft.util.IntHashMap (beta 1.7.3).
// Templated on the stored value type (Java's Object). Uses nullptr to denote
// an absent value, matching Java's null return.
template <typename V>
class IntHashMap {
public:
  using Entry = IntHashMapEntry<V>;
  IntHashMap() : table(16, nullptr), loadFactor(0.75f) {}
  ~IntHashMap() {
    clear();
  }
  IntHashMap(const IntHashMap&) = delete;
  IntHashMap& operator=(const IntHashMap&) = delete;
  V get(int key) {
    int hashValue = hash(key);
    Entry* entry = this->table[indexOf(hashValue, static_cast<int>(this->table.size()))];
    while(entry != nullptr) {
      if(entry->key == key) {
        return entry->value;
      }
      entry = entry->next;
    }
    return V();
  }
  bool containsKey(int key) {
    return this->getEntry(key) != nullptr;
  }
  Entry* getEntry(int key) {
    int hashValue = hash(key);
    Entry* entry = this->table[indexOf(hashValue, static_cast<int>(this->table.size()))];
    while(entry != nullptr) {
      if(entry->key == key) {
        return entry;
      }
      entry = entry->next;
    }
    return nullptr;
  }
  void put(int key, V value) {
    int hashValue = hash(key);
    int bucketIndex = indexOf(hashValue, static_cast<int>(this->table.size()));
    Entry* entry = this->table[bucketIndex];
    while(entry != nullptr) {
      if(entry->key == key) {
        entry->value = value;
      }
      entry = entry->next;
    }
    ++this->modCount;
    this->addEntry(hashValue, key, value, bucketIndex);
  }
  V remove(int key) {
    Entry* entry = this->removeEntry(key);
    if(entry == nullptr) {
      return V();
    }
    V value = entry->value;
    delete entry;
    return value;
  }
  Entry* removeEntry(int key) {
    int hashValue = hash(key);
    int bucketIndex = indexOf(hashValue, static_cast<int>(this->table.size()));
    Entry* prev = this->table[bucketIndex];
    Entry* cur = prev;
    while(cur != nullptr) {
      Entry* next = cur->next;
      if(cur->key == key) {
        ++this->modCount;
        --this->size;
        if(prev == cur) {
          this->table[bucketIndex] = next;
        } else {
          prev->next = next;
        }
        return cur;
      }
      prev = cur;
      cur = next;
    }
    return cur;
  }
  void clear() {
    ++this->modCount;
    for(std::size_t i = 0; i < this->table.size(); ++i) {
      Entry* entry = this->table[i];
      while(entry != nullptr) {
        Entry* next = entry->next;
        delete entry;
        entry = next;
      }
      this->table[i] = nullptr;
    }
    this->size = 0;
  }
  static int synthHash(int key) {
    return hash(key);
  }

private:
  std::vector<Entry*> table;
  int size = 0;
  int threshold = 12;
  float loadFactor;
  int modCount = 0;
  static int hash(int key) {
    std::uint32_t k = static_cast<std::uint32_t>(key);
    k ^= k >> 20 ^ k >> 12;
    return static_cast<int>(k ^ k >> 7 ^ k >> 4);
  }
  static int indexOf(int hashValue, int arrayLength) {
    return hashValue & (arrayLength - 1);
  }
  void resize(int newSize) {
    int oldCapacity = static_cast<int>(this->table.size());
    if(oldCapacity == 0x40000000) {
      this->threshold = 0x7fffffff;
      return;
    }
    std::vector<Entry*> newTable(static_cast<std::size_t>(newSize), nullptr);
    this->transfer(newTable);
    this->table = std::move(newTable);
    this->threshold = static_cast<int>(static_cast<float>(newSize) * this->loadFactor);
  }
  void transfer(std::vector<Entry*>& entryArray) {
    int newCapacity = static_cast<int>(entryArray.size());
    for(std::size_t i = 0; i < this->table.size(); ++i) {
      Entry* entry = this->table[i];
      if(entry == nullptr) {
        continue;
      }
      this->table[i] = nullptr;
      do {
        Entry* nextEntry = entry->next;
        int newBucketIndex = indexOf(entry->hash, newCapacity);
        entry->next = entryArray[newBucketIndex];
        entryArray[newBucketIndex] = entry;
        entry = nextEntry;
      } while(entry != nullptr);
    }
  }
  void addEntry(int hashValue, int key, V value, int index) {
    Entry* entry = this->table[index];
    this->table[index] = new Entry(hashValue, key, value, entry);
    if(this->size++ >= this->threshold) {
      this->resize(2 * static_cast<int>(this->table.size()));
    }
  }
};
template <typename V>
inline int IntHashMapEntry<V>::hashCode() const {
  return IntHashMap<V>::synthHash(this->key);
}
} // namespace net::minecraft::util
