#pragma once
#include <cstdint>
#include <string>
#include <variant>

#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::entity::data {
using DataTrackerValue = std::variant<std::int8_t, std::int16_t, std::int32_t, float, std::string, ItemStack, Vec3i>;

class DataTrackerEntry {
   public:
    DataTrackerEntry() = default;

    DataTrackerEntry(int dataTypeId, int id, DataTrackerValue value)
        : dataTypeId_(dataTypeId), id_(id), value_(std::move(value)) {
    }

    [[nodiscard]] int getId() const noexcept {
        return id_;
    }

    [[nodiscard]] int getDataTypeId() const noexcept {
        return dataTypeId_;
    }

    [[nodiscard]] const DataTrackerValue& get() const noexcept {
        return value_;
    }

    [[nodiscard]] DataTrackerValue& get() noexcept {
        return value_;
    }

    void set(DataTrackerValue value) {
        value_ = std::move(value);
    }

    [[nodiscard]] bool isDirty() const noexcept {
        return dirty_;
    }

    void setDirty(bool dirty) noexcept {
        dirty_ = dirty;
    }

   private:
    int dataTypeId_ = 0;
    int id_ = 0;
    DataTrackerValue value_{};
    bool dirty_ = true;
};
}  // namespace net::minecraft::entity::data
