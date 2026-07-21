#pragma once
#include <cstdint>
#include <vector>
#include "net/minecraft/util/LongObjectHashMapEntry.hpp"
namespace net::minecraft::util {
// Faithful port of net.minecraft.util.LongObjectHashMap (beta 1.7.3).
// Templated on the stored value type (Java's Object). Uses default-constructed V
// to denote an absent value, matching Java's null return.
template <typename V>
class LongObjectHashMap {
 public:
 using Entry = LongObjectHashMapEntry<V>;
 LongObjectHashMap() : table(16, nullptr), loadFactor(0.75f) {
 }
 ~LongObjectHashMap() {
  clear();
 }
 LongObjectHashMap(const LongObjectHashMap&) = delete;
 LongObjectHashMap& operator=(const LongObjectHashMap&) = delete;
 V get(std::int64_t key) {
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
 void put(std::int64_t key, V value) {
  int hashValue = hash(key);
  int bucketIndex = indexOf(hashValue, static_cast<int>(this->table.size()));
  Entry* entry = this->table[bucketIndex];
  while(entry != nullptr) {
   if(entry->key == key) {
    entry->value = value;
    return;
   }
   entry = entry->next;
  }
  ++this->modCount;
  this->addEntry(hashValue, key, value, bucketIndex);
 }
 V remove(std::int64_t key) {
  Entry* entry = this->removeEntry(key);
  if(entry == nullptr) {
   return V();
  }
  V value = entry->value;
  delete entry;
  return value;
 }
 Entry* removeEntry(std::int64_t key) {
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
 static int synthHash(std::int64_t key) {
  return hash(key);
 }

 private:
 std::vector<Entry*> table;
 int size = 0;
 int capacity = 12;
 float loadFactor;
 int modCount = 0;
 static int hash(std::int64_t key) {
  const std::uint64_t u = static_cast<std::uint64_t>(key);
  const int mixed = static_cast<int>(u ^ (u >> 32));
  return hash(mixed);
 }
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
   this->capacity = 0x7fffffff;
   return;
  }
  std::vector<Entry*> newTable(static_cast<std::size_t>(newSize), nullptr);
  this->transfer(newTable);
  this->table = std::move(newTable);
  this->capacity = static_cast<int>(static_cast<float>(newSize) * this->loadFactor);
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
 void addEntry(int hashValue, std::int64_t key, V value, int index) {
  Entry* entry = this->table[index];
  this->table[index] = new Entry(hashValue, key, value, entry);
  if(this->size++ >= this->capacity) {
   this->resize(2 * static_cast<int>(this->table.size()));
  }
 }
};
template <typename V>
inline int LongObjectHashMapEntry<V>::hashCode() const {
 return LongObjectHashMap<V>::synthHash(this->key);
}
} // namespace net::minecraft::util
