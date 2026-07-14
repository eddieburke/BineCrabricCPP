#pragma once
#include <algorithm>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/entity/data/DataTrackerEntry.hpp"
#include "net/minecraft/network/Packet.hpp"
namespace net::minecraft::entity::data {
class DataTracker {
public:
  static constexpr std::size_t kMaxByteArrayLength = 1024U * 1024U;
  DataTracker() = default;
  template <typename T>
  void startTracking(int key, T value) {
    const int dataTypeId = dataTypeIdFor<T>();
    if(key > 31) {
      throw std::invalid_argument("Data value id is too big");
    }
    if(entries_.contains(key)) {
      throw std::invalid_argument("Duplicate data tracker id");
    }
    entries_.emplace(key, DataTrackerEntry(dataTypeId, key, DataTrackerValue(normalizeValue(std::move(value)))));
  }
  [[nodiscard]] std::int8_t getByte(int id) const {
    return std::get<std::int8_t>(entries_.at(id).get());
  }
  [[nodiscard]] std::int16_t getShort(int id) const {
    return std::get<std::int16_t>(entries_.at(id).get());
  }
  [[nodiscard]] std::int32_t getInt(int id) const {
    return std::get<std::int32_t>(entries_.at(id).get());
  }
  [[nodiscard]] float getFloat(int id) const {
    return std::get<float>(entries_.at(id).get());
  }
  [[nodiscard]] std::string getString(int id) const {
    return std::get<std::string>(entries_.at(id).get());
  }
  [[nodiscard]] const ItemStack& getItemStack(int id) const {
    return std::get<ItemStack>(entries_.at(id).get());
  }
  [[nodiscard]] const Vec3i& getVec3i(int id) const {
    return std::get<Vec3i>(entries_.at(id).get());
  }
  [[nodiscard]] const DataTrackerByteArray& getByteArray(int id) const {
    return std::get<DataTrackerByteArray>(entries_.at(id).get());
  }
  template <typename T>
  void set(int id, T value) {
    DataTrackerEntry& entry = entries_.at(id);
    const DataTrackerValue normalized = DataTrackerValue(normalizeValue(std::move(value)));
    if(entry.get() != normalized) {
      entry.set(normalized);
      entry.setDirty(true);
      dirty_ = true;
    }
  }
  [[nodiscard]] bool isDirty() const noexcept {
    return dirty_;
  }
  [[nodiscard]] std::vector<DataTrackerEntry> getDirtyEntries() {
    std::vector<DataTrackerEntry> dirtyEntries;
    if(dirty_) {
      for(auto& [id, entry] : entries_) {
        if(!entry.isDirty()) {
          continue;
        }
        entry.setDirty(false);
        dirtyEntries.push_back(entry);
      }
    }
    dirty_ = false;
    return dirtyEntries;
  }
  void writeAllEntries(std::ostream& output) const {
    std::vector<DataTrackerEntry> entries;
    entries.reserve(entries_.size());
    for(const auto& [id, entry] : entries_) {
      entries.push_back(entry);
    }
    writeEntries(entries, output);
  }
  [[nodiscard]] std::vector<DataTrackerEntry> snapshotEntries() const {
    std::vector<DataTrackerEntry> entries;
    entries.reserve(entries_.size());
    for(const auto& [id, entry] : entries_) {
      entries.push_back(entry);
    }
    return entries;
  }
  static void writeEntries(const std::vector<DataTrackerEntry>& entries, std::ostream& output) {
    for(const DataTrackerEntry& entry : entries) {
      writeEntry(output, entry);
    }
    packetio::writeU8(output, 127U);
  }
  static std::vector<DataTrackerEntry> readEntries(std::istream& input) {
    std::vector<DataTrackerEntry> entries;
    for(;;) {
      const std::uint8_t header = packetio::readU8(input);
      if(header == 127U) {
        break;
      }
      const int dataTypeId = static_cast<int>((header & 0xE0U) >> 5U);
      const int id = static_cast<int>(header & 0x1FU);
      switch(dataTypeId) {
      case 0:
        entries.emplace_back(dataTypeId, id, packetio::readI8(input));
        break;
      case 1:
        entries.emplace_back(dataTypeId, id, packetio::readI16BE(input));
        break;
      case 2:
        entries.emplace_back(dataTypeId, id, packetio::readI32BE(input));
        break;
      case 3:
        entries.emplace_back(dataTypeId, id, packetio::readFloatBE(input));
        break;
      case 4:
        entries.emplace_back(dataTypeId, id, Packet::readString(input, 64));
        break;
      case 5: {
        const std::int16_t itemId = packetio::readI16BE(input);
        const std::int8_t count = packetio::readI8(input);
        const std::int16_t damage = packetio::readI16BE(input);
        entries.emplace_back(dataTypeId, id, ItemStack(itemId, count, damage));
        break;
      }
      case 6: {
        const std::int32_t x = packetio::readI32BE(input);
        const std::int32_t y = packetio::readI32BE(input);
        const std::int32_t z = packetio::readI32BE(input);
        entries.emplace_back(dataTypeId, id, Vec3i(x, y, z));
        break;
      }
      case 7: {
        const std::int32_t length = packetio::readI32BE(input);
        if(length < 0 || static_cast<std::size_t>(length) > kMaxByteArrayLength) {
          throw std::runtime_error("Data tracker byte array is too large");
        }
        entries.emplace_back(dataTypeId, id, packetio::readBytes(input, static_cast<std::size_t>(length)));
        break;
      }
      default:
        throw std::runtime_error("Unknown data tracker entry type");
      }
    }
    return entries;
  }
  [[nodiscard]] std::vector<int> writeUpdatedEntries(const std::vector<DataTrackerEntry>& entries) {
    std::vector<int> updatedIds;
    updatedIds.reserve(entries.size());
    for(const DataTrackerEntry& entry : entries) {
      auto it = entries_.find(entry.getId());
      if(it == entries_.end() || it->second.getDataTypeId() != entry.getDataTypeId()) {
        continue;
      }
      it->second.set(entry.get());
      updatedIds.push_back(entry.getId());
    }
    return updatedIds;
  }

private:
  template <typename T>
  static constexpr int dataTypeIdFor() {
    using CleanT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr(std::is_same_v<CleanT, std::int8_t>) {
      return 0;
    } else if constexpr(std::is_same_v<CleanT, std::int16_t>) {
      return 1;
    } else if constexpr(std::is_same_v<CleanT, std::int32_t> || std::is_same_v<CleanT, int>) {
      return 2;
    } else if constexpr(std::is_same_v<CleanT, float>) {
      return 3;
    } else if constexpr(std::is_same_v<CleanT, std::string>) {
      return 4;
    } else if constexpr(std::is_same_v<CleanT, ItemStack>) {
      return 5;
    } else if constexpr(std::is_same_v<CleanT, Vec3i>) {
      return 6;
    } else if constexpr(std::is_same_v<CleanT, DataTrackerByteArray>) {
      return 7;
    } else {
      static_assert(!sizeof(T), "Unsupported DataTracker type");
    }
  }
  static std::int8_t normalizeValue(std::int8_t value) {
    return value;
  }
  static std::int16_t normalizeValue(std::int16_t value) {
    return value;
  }
  static std::int32_t normalizeValue(std::int32_t value) {
    return value;
  }
  static float normalizeValue(float value) {
    return value;
  }
  static std::string normalizeValue(const std::string& value) {
    return value;
  }
  static std::string normalizeValue(std::string&& value) {
    return std::move(value);
  }
  static ItemStack normalizeValue(const ItemStack& value) {
    return value;
  }
  static ItemStack normalizeValue(ItemStack&& value) {
    return std::move(value);
  }
  static Vec3i normalizeValue(const Vec3i& value) {
    return value;
  }
  static Vec3i normalizeValue(Vec3i&& value) {
    return std::move(value);
  }
  static DataTrackerByteArray normalizeValue(const DataTrackerByteArray& value) {
    if(value.size() > kMaxByteArrayLength) {
      throw std::invalid_argument("Data tracker byte array is too large");
    }
    return value;
  }
  static DataTrackerByteArray normalizeValue(DataTrackerByteArray&& value) {
    if(value.size() > kMaxByteArrayLength) {
      throw std::invalid_argument("Data tracker byte array is too large");
    }
    return std::move(value);
  }
  static void writeEntry(std::ostream& output, const DataTrackerEntry& entry) {
    const std::uint8_t header =
        static_cast<std::uint8_t>(((entry.getDataTypeId() << 5) | (entry.getId() & 0x1F)) & 0xFF);
    packetio::writeU8(output, header);
    switch(entry.getDataTypeId()) {
    case 0:
      packetio::writeI8(output, std::get<std::int8_t>(entry.get()));
      break;
    case 1:
      packetio::writeI16BE(output, std::get<std::int16_t>(entry.get()));
      break;
    case 2:
      packetio::writeI32BE(output, std::get<std::int32_t>(entry.get()));
      break;
    case 3:
      packetio::writeFloatBE(output, std::get<float>(entry.get()));
      break;
    case 4:
      Packet::writeString(std::get<std::string>(entry.get()), output);
      break;
    case 5: {
      const ItemStack& stack = std::get<ItemStack>(entry.get());
      packetio::writeI16BE(output, static_cast<std::int16_t>(stack.itemId));
      packetio::writeI8(output, static_cast<std::int8_t>(stack.count));
      packetio::writeI16BE(output, static_cast<std::int16_t>(stack.getDamage()));
      break;
    }
    case 6: {
      const Vec3i& vec = std::get<Vec3i>(entry.get());
      packetio::writeI32BE(output, vec.x);
      packetio::writeI32BE(output, vec.y);
      packetio::writeI32BE(output, vec.z);
      break;
    }
    case 7: {
      const DataTrackerByteArray& bytes = std::get<DataTrackerByteArray>(entry.get());
      if(bytes.size() > kMaxByteArrayLength) {
        throw std::runtime_error("Data tracker byte array is too large");
      }
      packetio::writeI32BE(output, static_cast<std::int32_t>(bytes.size()));
      packetio::writeBytes(output, bytes);
      break;
    }
    default:
      throw std::runtime_error("Unknown data tracker entry type");
    }
  }
  std::unordered_map<int, DataTrackerEntry> entries_;
  bool dirty_ = false;
};
} // namespace net::minecraft::entity::data
