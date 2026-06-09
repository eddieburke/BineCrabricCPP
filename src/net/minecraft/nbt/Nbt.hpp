#pragma once

// Unified NBT value storage and binary I/O. Application code should prefer NbtCompound / NbtElement
// (net.minecraft.nbt.*) for Java parity; Nbt remains the backing type for compounds and lists.

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace net::minecraft {

class Nbt {
public:
    enum class Type : std::uint8_t {
        End = 0,
        Byte = 1,
        Short = 2,
        Int = 3,
        Long = 4,
        Float = 5,
        Double = 6,
        ByteArray = 7,
        String = 8,
        List = 9,
        Compound = 10,
        IntArray = 11,
        LongArray = 12
    };

    using List = std::vector<Nbt>;
    using Compound = std::unordered_map<std::string, Nbt>;
    using ByteArray = std::vector<std::uint8_t>;
    using IntArray = std::vector<std::int32_t>;
    using LongArray = std::vector<std::int64_t>;

    Nbt() = default;
    explicit Nbt(std::int8_t value) : type_(Type::Byte), value_(value) {}
    explicit Nbt(std::int16_t value) : type_(Type::Short), value_(value) {}
    explicit Nbt(std::int32_t value) : type_(Type::Int), value_(value) {}
    explicit Nbt(std::int64_t value) : type_(Type::Long), value_(value) {}
    explicit Nbt(float value) : type_(Type::Float), value_(value) {}
    explicit Nbt(double value) : type_(Type::Double), value_(value) {}
    explicit Nbt(std::string value) : type_(Type::String), value_(std::move(value)) {}
    explicit Nbt(const char* value) : Nbt(std::string(value)) {}
    explicit Nbt(std::vector<std::uint8_t> value) : type_(Type::ByteArray), value_(std::move(value)) {}
    explicit Nbt(std::vector<std::int32_t> value) : type_(Type::IntArray), value_(std::move(value)) {}
    explicit Nbt(std::vector<std::int64_t> value) : type_(Type::LongArray), value_(std::move(value)) {}

    static Nbt list()
    {
        Nbt tag;
        tag.type_ = Type::List;
        tag.value_ = std::make_shared<List>();
        return tag;
    }

    static Nbt compound()
    {
        Nbt tag;
        tag.type_ = Type::Compound;
        tag.value_ = std::make_shared<Compound>();
        return tag;
    }

    [[nodiscard]] Type type() const noexcept
    {
        return type_;
    }

    [[nodiscard]] bool isCompound() const noexcept
    {
        return type_ == Type::Compound;
    }

    [[nodiscard]] bool isList() const noexcept
    {
        return type_ == Type::List;
    }

    [[nodiscard]] Compound& asCompound()
    {
        ensureType(Type::Compound);
        return *std::get<std::shared_ptr<Compound>>(value_);
    }

    [[nodiscard]] const Compound& asCompound() const
    {
        ensureType(Type::Compound);
        return *std::get<std::shared_ptr<Compound>>(value_);
    }

    [[nodiscard]] std::int8_t asByte() const
    {
        ensureType(Type::Byte);
        return std::get<std::int8_t>(value_);
    }

    [[nodiscard]] std::int16_t asShort() const
    {
        ensureType(Type::Short);
        return std::get<std::int16_t>(value_);
    }

    [[nodiscard]] std::int32_t asInt() const
    {
        ensureType(Type::Int);
        return std::get<std::int32_t>(value_);
    }

    [[nodiscard]] std::int64_t asLong() const
    {
        ensureType(Type::Long);
        return std::get<std::int64_t>(value_);
    }

    [[nodiscard]] float asFloat() const
    {
        ensureType(Type::Float);
        return std::get<float>(value_);
    }

    [[nodiscard]] double asDouble() const
    {
        ensureType(Type::Double);
        return std::get<double>(value_);
    }

    [[nodiscard]] const std::string& asString() const
    {
        ensureType(Type::String);
        return std::get<std::string>(value_);
    }

    [[nodiscard]] ByteArray& asByteArray()
    {
        ensureType(Type::ByteArray);
        return std::get<ByteArray>(value_);
    }

    [[nodiscard]] const ByteArray& asByteArray() const
    {
        ensureType(Type::ByteArray);
        return std::get<ByteArray>(value_);
    }

    [[nodiscard]] IntArray& asIntArray()
    {
        ensureType(Type::IntArray);
        return std::get<IntArray>(value_);
    }

    [[nodiscard]] const IntArray& asIntArray() const
    {
        ensureType(Type::IntArray);
        return std::get<IntArray>(value_);
    }

    [[nodiscard]] LongArray& asLongArray()
    {
        ensureType(Type::LongArray);
        return std::get<LongArray>(value_);
    }

    [[nodiscard]] const LongArray& asLongArray() const
    {
        ensureType(Type::LongArray);
        return std::get<LongArray>(value_);
    }

    [[nodiscard]] List& asList()
    {
        ensureType(Type::List);
        return *std::get<std::shared_ptr<List>>(value_);
    }

    [[nodiscard]] const List& asList() const
    {
        ensureType(Type::List);
        return *std::get<std::shared_ptr<List>>(value_);
    }

    void put(const std::string& key, Nbt value)
    {
        asCompound()[key] = std::move(value);
    }

    void putByte(const std::string& key, std::int8_t value)
    {
        put(key, Nbt(value));
    }

    void putShort(const std::string& key, std::int16_t value)
    {
        put(key, Nbt(value));
    }

    void putInt(const std::string& key, std::int32_t value)
    {
        put(key, Nbt(value));
    }

    void putLong(const std::string& key, std::int64_t value)
    {
        put(key, Nbt(value));
    }

    void putFloat(const std::string& key, float value)
    {
        put(key, Nbt(value));
    }

    void putDouble(const std::string& key, double value)
    {
        put(key, Nbt(value));
    }

    void putString(const std::string& key, std::string value)
    {
        put(key, Nbt(std::move(value)));
    }

    void putByteArray(const std::string& key, ByteArray value)
    {
        put(key, Nbt(std::move(value)));
    }

    void putIntArray(const std::string& key, IntArray value)
    {
        put(key, Nbt(std::move(value)));
    }

    void putLongArray(const std::string& key, LongArray value)
    {
        put(key, Nbt(std::move(value)));
    }

    void putBoolean(const std::string& key, bool value)
    {
        putByte(key, value ? 1 : 0);
    }

    [[nodiscard]] const Nbt* get(const std::string& key) const
    {
        if (!isCompound()) {
            return nullptr;
        }

        const auto& compound = asCompound();
        const auto it = compound.find(key);
        if (it == compound.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] Nbt* get(const std::string& key)
    {
        if (!isCompound()) {
            return nullptr;
        }

        auto& compound = asCompound();
        const auto it = compound.find(key);
        if (it == compound.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] bool contains(const std::string& key) const
    {
        return isCompound() && asCompound().find(key) != asCompound().end();
    }

    [[nodiscard]] std::int8_t getByte(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Byte) {
            return std::get<std::int8_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int16_t getShort(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Short) {
            return std::get<std::int16_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int32_t getInt(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Int) {
            return std::get<std::int32_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] std::int64_t getLong(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Long) {
            return std::get<std::int64_t>(tag->value_);
        }
        return 0;
    }

    [[nodiscard]] float getFloat(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Float) {
            return std::get<float>(tag->value_);
        }
        return 0.0F;
    }

    [[nodiscard]] double getDouble(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::Double) {
            return std::get<double>(tag->value_);
        }
        return 0.0;
    }

    [[nodiscard]] std::string getString(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::String) {
            return std::get<std::string>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] ByteArray getByteArray(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::ByteArray) {
            return std::get<ByteArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] IntArray getIntArray(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::IntArray) {
            return std::get<IntArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] LongArray getLongArray(const std::string& key) const
    {
        if (const Nbt* tag = get(key); tag != nullptr && tag->type_ == Type::LongArray) {
            return std::get<LongArray>(tag->value_);
        }
        return {};
    }

    [[nodiscard]] bool getBoolean(const std::string& key) const
    {
        return getByte(key) != 0;
    }

    [[nodiscard]] std::string dump(int indent = 0) const
    {
        std::ostringstream out;
        dumpImpl(out, indent);
        return out.str();
    }

    void write(std::ostream& out) const
    {
        std::vector<std::uint8_t> bytes;
        writeNamedTag(bytes, "", *this);
        binary::writeAllBytes(out, bytes);
    }

    void writeCompressed(std::ostream& out) const
    {
        const std::vector<std::uint8_t> bytes = toBytes();
        const std::vector<std::uint8_t> compressed = gzipCompress(bytes);
        binary::writeAllBytes(out, compressed);
    }

    [[nodiscard]] std::vector<std::uint8_t> toBytes() const
    {
        std::vector<std::uint8_t> bytes;
        writeNamedTag(bytes, "", *this);
        return bytes;
    }

    static Nbt read(std::istream& in)
    {
        const std::vector<std::uint8_t> bytes = binary::readAllBytes(in);
        std::size_t pos = 0;
        return readNamedTag(bytes, pos);
    }

    static Nbt read(const std::vector<std::uint8_t>& bytes)
    {
        std::size_t pos = 0;
        return readNamedTag(bytes, pos);
    }

    static Nbt readCompressed(std::istream& in)
    {
        const std::vector<std::uint8_t> bytes = binary::readAllBytes(in);
        const std::vector<std::uint8_t> decompressed = gzipDecompress(bytes);
        std::size_t pos = 0;
        return readNamedTag(decompressed, pos);
    }

    static Nbt readCompressed(const std::vector<std::uint8_t>& bytes)
    {
        const std::vector<std::uint8_t> decompressed = gzipDecompress(bytes);
        std::size_t pos = 0;
        return readNamedTag(decompressed, pos);
    }

private:
    using Storage = std::variant<
        std::monostate,
        std::int8_t,
        std::int16_t,
        std::int32_t,
        std::int64_t,
        float,
        double,
        std::vector<std::uint8_t>,
        std::string,
        std::shared_ptr<List>,
        std::shared_ptr<Compound>,
        std::vector<std::int32_t>,
        std::vector<std::int64_t>>;

    void ensureType(Type expected) const
    {
        if (type_ != expected) {
            throw std::logic_error("NBT type mismatch");
        }
    }

    void dumpImpl(std::ostream& out, int indent) const
    {
        const auto pad = std::string(static_cast<std::size_t>(indent), ' ');
        switch (type_) {
        case Type::End:
        out << "END";
        break;
        case Type::Byte:
            out << static_cast<int>(std::get<std::int8_t>(value_));
            break;
        case Type::Short:
            out << std::get<std::int16_t>(value_);
            break;
        case Type::Int:
            out << std::get<std::int32_t>(value_);
            break;
        case Type::Long:
            out << std::get<std::int64_t>(value_);
            break;
        case Type::Float:
            out << std::get<float>(value_);
            break;
        case Type::Double:
            out << std::get<double>(value_);
            break;
        case Type::ByteArray: {
            const auto& bytes = std::get<ByteArray>(value_);
            out << "byte[" << bytes.size() << "]";
            break;
        }
        case Type::String:
            out << '"' << std::get<std::string>(value_) << '"';
            break;
        case Type::List: {
            out << "[";
            const auto& list = *std::get<std::shared_ptr<List>>(value_);
            for (std::size_t i = 0; i < list.size(); ++i) {
                if (i != 0) {
                    out << ", ";
                }
                list[i].dumpImpl(out, indent + 2);
            }
            out << "]";
            break;
        }
        case Type::Compound: {
            out << "{";
            const auto& compound = *std::get<std::shared_ptr<Compound>>(value_);
            bool first = true;
            for (const auto& [key, value] : compound) {
                if (!first) {
                    out << ", ";
                }
                first = false;
                out << key << ": ";
                value.dumpImpl(out, indent + 2);
            }
            out << "}";
            break;
        }
        case Type::IntArray: {
            const auto& values = std::get<IntArray>(value_);
            out << "int[" << values.size() << "]";
            break;
        }
        case Type::LongArray: {
            const auto& values = std::get<LongArray>(value_);
            out << "long[" << values.size() << "]";
            break;
        }
        }
        (void)pad;
    }

    static void writeNamedTag(std::vector<std::uint8_t>& out, const std::string& name, const Nbt& tag)
    {
        binary::appendU8(out, static_cast<std::uint8_t>(tag.type_));
        if (tag.type_ == Type::End) {
            return;
        }

        binary::writeModifiedUtf8(out, name);
        tag.writePayload(out);
    }

    void writePayload(std::vector<std::uint8_t>& out) const
    {
        switch (type_) {
        case Type::End:
            return;
        case Type::Byte:
            binary::appendU8(out, static_cast<std::uint8_t>(std::get<std::int8_t>(value_)));
            return;
        case Type::Short:
            binary::appendI16BE(out, std::get<std::int16_t>(value_));
            return;
        case Type::Int:
            binary::appendI32BE(out, std::get<std::int32_t>(value_));
            return;
        case Type::Long:
            binary::appendI64BE(out, std::get<std::int64_t>(value_));
            return;
        case Type::Float: {
            const float value = std::get<float>(value_);
            static_assert(sizeof(float) == sizeof(std::uint32_t));
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(float));
            binary::appendU32BE(out, bits);
            return;
        }
        case Type::Double: {
            const double value = std::get<double>(value_);
            static_assert(sizeof(double) == sizeof(std::uint64_t));
            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(double));
            binary::appendU64BE(out, bits);
            return;
        }
        case Type::ByteArray: {
            const ByteArray& bytes = std::get<ByteArray>(value_);
            binary::appendI32BE(out, static_cast<std::int32_t>(bytes.size()));
            out.insert(out.end(), bytes.begin(), bytes.end());
            return;
        }
        case Type::String:
            binary::writeModifiedUtf8(out, std::get<std::string>(value_));
            return;
        case Type::List: {
            const auto& list = *std::get<std::shared_ptr<List>>(value_);
            // Beta 1.7.3 writes TAG_Byte (1) for empty lists, not TAG_End (0).
            const Type itemType = list.empty() ? Type::Byte : list.front().type();
            binary::appendU8(out, static_cast<std::uint8_t>(itemType));
            binary::appendI32BE(out, static_cast<std::int32_t>(list.size()));
            for (const Nbt& item : list) {
                item.writePayload(out);
            }
            return;
        }
        case Type::Compound: {
            const auto& compound = *std::get<std::shared_ptr<Compound>>(value_);
            for (const auto& [key, child] : compound) {
                writeNamedTag(out, key, child);
            }
            binary::appendU8(out, 0);
            return;
        }
        case Type::IntArray: {
            const IntArray& values = std::get<IntArray>(value_);
            binary::appendI32BE(out, static_cast<std::int32_t>(values.size()));
            for (std::int32_t value : values) {
                binary::appendI32BE(out, value);
            }
            return;
        }
        case Type::LongArray: {
            const LongArray& values = std::get<LongArray>(value_);
            binary::appendI32BE(out, static_cast<std::int32_t>(values.size()));
            for (std::int64_t value : values) {
                binary::appendI64BE(out, value);
            }
            return;
        }
        }
    }

    static Nbt readNamedTag(const std::vector<std::uint8_t>& data, std::size_t& pos)
    {
        const auto type = static_cast<Type>(binary::readU8(data, pos));
        if (type == Type::End) {
            return Nbt();
        }

        (void)binary::readModifiedUtf8(data, pos);
        return readPayload(type, data, pos);
    }

    static Nbt readPayload(Type type, const std::vector<std::uint8_t>& data, std::size_t& pos)
    {
        switch (type) {
        case Type::End:
            return Nbt();
        case Type::Byte:
            return Nbt(static_cast<std::int8_t>(binary::readU8(data, pos)));
        case Type::Short:
            return Nbt(binary::readI16BE(data, pos));
        case Type::Int:
            return Nbt(binary::readI32BE(data, pos));
        case Type::Long:
            return Nbt(binary::readI64BE(data, pos));
        case Type::Float: {
            const std::uint32_t bits = binary::readU32BE(data, pos);
            float value = 0.0F;
            std::memcpy(&value, &bits, sizeof(float));
            return Nbt(value);
        }
        case Type::Double: {
            const std::uint64_t bits = binary::readU64BE(data, pos);
            double value = 0.0;
            std::memcpy(&value, &bits, sizeof(double));
            return Nbt(value);
        }
        case Type::ByteArray: {
            const std::int32_t size = binary::readI32BE(data, pos);
            if (size < 0) {
                throw std::runtime_error("Negative byte array size");
            }
            ByteArray bytes(static_cast<std::size_t>(size));
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                bytes[i] = binary::readU8(data, pos);
            }
            return Nbt(std::move(bytes));
        }
        case Type::String:
            return Nbt(binary::readModifiedUtf8(data, pos));
        case Type::List: {
            const auto itemType = static_cast<Type>(binary::readU8(data, pos));
            const std::int32_t size = binary::readI32BE(data, pos);
            if (size < 0) {
                throw std::runtime_error("Negative NBT list size");
            }
            Nbt tag = list();
            auto& items = tag.asList();
            items.reserve(static_cast<std::size_t>(size));
            for (std::int32_t i = 0; i < size; ++i) {
                items.push_back(readPayload(itemType, data, pos));
            }
            return tag;
        }
        case Type::Compound: {
            Nbt tag = compound();
            auto& items = tag.asCompound();
            while (true) {
                const auto childType = static_cast<Type>(binary::readU8(data, pos));
                if (childType == Type::End) {
                    break;
                }
                const std::string key = binary::readModifiedUtf8(data, pos);
                items[key] = readPayload(childType, data, pos);
            }
            return tag;
        }
        case Type::IntArray: {
            const std::int32_t size = binary::readI32BE(data, pos);
            if (size < 0) {
                throw std::runtime_error("Negative int array size");
            }
            IntArray values(static_cast<std::size_t>(size));
            for (std::size_t i = 0; i < values.size(); ++i) {
                values[i] = binary::readI32BE(data, pos);
            }
            return Nbt(std::move(values));
        }
        case Type::LongArray: {
            const std::int32_t size = binary::readI32BE(data, pos);
            if (size < 0) {
                throw std::runtime_error("Negative long array size");
            }
            LongArray values(static_cast<std::size_t>(size));
            for (std::size_t i = 0; i < values.size(); ++i) {
                values[i] = binary::readI64BE(data, pos);
            }
            return Nbt(std::move(values));
        }
        }

        throw std::runtime_error("Unknown NBT tag type");
    }

    Type type_ = Type::End;
    Storage value_ {};
};

} // namespace net::minecraft
