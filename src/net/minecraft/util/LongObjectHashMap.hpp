#pragma once

#include "net/minecraft/util/LongObjectHashMapEntry.hpp"

#include <cstdint>
#include <vector>

namespace net::minecraft::util {

// Faithful port of net.minecraft.util.LongObjectHashMap (beta 1.7.3).
// Templated on the stored value type (Java's Object). Uses nullptr/V() to
// denote an absent value, matching Java's null return.
template <typename V>
class LongObjectHashMap {
public:
    using Entry = LongObjectHashMapEntry<V>;

    LongObjectHashMap() : entries(16, nullptr), loadFactor(0.75f) {}

    ~LongObjectHashMap() { clearAll(); }

    LongObjectHashMap(const LongObjectHashMap&) = delete;
    LongObjectHashMap& operator=(const LongObjectHashMap&) = delete;

    V get(std::int64_t key) {
        int n = hash(key);
        Entry* entry = this->entries[indexOf(n, static_cast<int>(this->entries.size()))];
        while (entry != nullptr) {
            if (entry->key == key) {
                return entry->value;
            }
            entry = entry->next;
        }
        return V();
    }

    void put(std::int64_t key, V value) {
        int n = hash(key);
        int n2 = indexOf(n, static_cast<int>(this->entries.size()));
        Entry* entry = this->entries[n2];
        while (entry != nullptr) {
            if (entry->key == key) {
                entry->value = value;
            }
            entry = entry->next;
        }
        ++this->modCount;
        this->addEntry(n, key, value, n2);
    }

    V remove(std::int64_t key) {
        Entry* entry = this->removeEntry(key);
        if (entry == nullptr) {
            return V();
        }
        V value = entry->value;
        delete entry;
        return value;
    }

    Entry* removeEntry(std::int64_t key) {
        int n = hash(key);
        int n2 = indexOf(n, static_cast<int>(this->entries.size()));
        Entry* prev = this->entries[n2];
        Entry* cur = prev;
        while (cur != nullptr) {
            Entry* next = cur->next;
            if (cur->key == key) {
                ++this->modCount;
                --this->size;
                if (prev == cur) {
                    this->entries[n2] = next;
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

    static int synthHash(std::int64_t l) { return hash(l); }

private:
    std::vector<Entry*> entries;
    int size = 0;
    int capacity = 12;
    float loadFactor;
    int modCount = 0;

    static int hash(std::int64_t key) {
        return hash(static_cast<int>(key ^ static_cast<std::int64_t>(static_cast<std::uint64_t>(key) >> 32)));
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
        int n = static_cast<int>(this->entries.size());
        if (n == 0x40000000) {
            this->capacity = 0x7fffffff;
            return;
        }
        std::vector<Entry*> newEntries(static_cast<std::size_t>(newSize), nullptr);
        this->transfer(newEntries);
        this->entries = std::move(newEntries);
        this->capacity = static_cast<int>(static_cast<float>(newSize) * this->loadFactor);
    }

    void transfer(std::vector<Entry*>& entryArray) {
        int n = static_cast<int>(entryArray.size());
        for (std::size_t i = 0; i < this->entries.size(); ++i) {
            Entry* entry = this->entries[i];
            if (entry == nullptr) {
                continue;
            }
            this->entries[i] = nullptr;
            do {
                Entry* nextEntry = entry->next;
                int n2 = indexOf(entry->hash, n);
                entry->next = entryArray[n2];
                entryArray[n2] = entry;
                entry = nextEntry;
            } while (entry != nullptr);
        }
    }

    void addEntry(int hashValue, std::int64_t key, V value, int index) {
        Entry* entry = this->entries[index];
        this->entries[index] = new Entry(hashValue, key, value, entry);
        if (this->size++ >= this->capacity) {
            this->resize(2 * static_cast<int>(this->entries.size()));
        }
    }

    void clearAll() {
        for (std::size_t i = 0; i < this->entries.size(); ++i) {
            Entry* entry = this->entries[i];
            while (entry != nullptr) {
                Entry* next = entry->next;
                delete entry;
                entry = next;
            }
            this->entries[i] = nullptr;
        }
        this->size = 0;
    }
};

template <typename V>
inline int LongObjectHashMapEntry<V>::hashCode() const {
    return LongObjectHashMap<V>::synthHash(this->key);
}

} // namespace net::minecraft::util
