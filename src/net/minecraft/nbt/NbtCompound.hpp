#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"
#include "net/minecraft/nbt/NbtList.hpp"

#include <string>
#include <vector>

namespace net::minecraft {

class NbtList;

// Faithful port of net.minecraft.nbt.NbtCompound.
class NbtCompound : public NbtElement {
public:
    NbtCompound() : tag_(Nbt::compound()) {}

    explicit NbtCompound(Nbt tag)
        : tag_(std::move(tag))
    {
        if (!tag_.isCompound()) {
            tag_ = Nbt::compound();
        }
    }

    [[nodiscard]] std::uint8_t getType() const override { return 10; }

    void write(std::ostream& output) const override;
    void read(std::istream& input) override;

    [[nodiscard]] Nbt toStorage() const override { return tag_; }

    [[nodiscard]] Nbt& storage() { return tag_; }
    [[nodiscard]] const Nbt& storage() const { return tag_; }

    void put(const std::string& key, Nbt value) { tag_.put(key, std::move(value)); }
    void put(const std::string& key, const NbtCompound& value) { tag_.put(key, value.tag_); }
    void put(const std::string& key, const NbtList& value) { tag_.put(key, value.storage()); }
    void put(const std::string& key, std::unique_ptr<NbtElement> value);

    void putByte(const std::string& key, std::int8_t value) { tag_.putByte(key, value); }
    void putShort(const std::string& key, std::int16_t value) { tag_.putShort(key, value); }
    void putInt(const std::string& key, std::int32_t value) { tag_.putInt(key, value); }
    void putLong(const std::string& key, std::int64_t value) { tag_.putLong(key, value); }
    void putFloat(const std::string& key, float value) { tag_.putFloat(key, value); }
    void putDouble(const std::string& key, double value) { tag_.putDouble(key, value); }
    void putString(const std::string& key, std::string value) { tag_.putString(key, std::move(value)); }
    void putByteArray(const std::string& key, std::vector<std::uint8_t> value)
    {
        tag_.putByteArray(key, std::move(value));
    }
    void putBoolean(const std::string& key, bool value) { tag_.putBoolean(key, value); }

    [[nodiscard]] bool contains(const std::string& key) const { return tag_.contains(key); }

    [[nodiscard]] std::int8_t getByte(const std::string& key) const { return tag_.getByte(key); }
    [[nodiscard]] std::int16_t getShort(const std::string& key) const { return tag_.getShort(key); }
    [[nodiscard]] std::int32_t getInt(const std::string& key) const { return tag_.getInt(key); }
    [[nodiscard]] std::int64_t getLong(const std::string& key) const { return tag_.getLong(key); }
    [[nodiscard]] float getFloat(const std::string& key) const { return tag_.getFloat(key); }
    [[nodiscard]] double getDouble(const std::string& key) const { return tag_.getDouble(key); }
    [[nodiscard]] std::string getString(const std::string& key) const { return tag_.getString(key); }
    [[nodiscard]] std::vector<std::uint8_t> getByteArray(const std::string& key) const;
    [[nodiscard]] bool getBoolean(const std::string& key) const { return tag_.getBoolean(key); }

    [[nodiscard]] NbtCompound getCompound(const std::string& key) const;
    [[nodiscard]] NbtList getList(const std::string& key) const;

private:
    Nbt tag_;
};

} // namespace net::minecraft
