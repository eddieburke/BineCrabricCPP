#include "net/minecraft/nbt/NbtBinaryCodec.hpp"

#include <cstring>

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Nbt.hpp"

namespace net::minecraft::nbt::detail {
namespace {
constexpr std::size_t kMaxNbtDepth = 512;
constexpr std::size_t kMaxContainerElements = 16U * 1024U * 1024U;

[[nodiscard]] bool validType(std::uint8_t type) noexcept {
    return type <= static_cast<std::uint8_t>(Nbt::Type::LongArray);
}

void validateCount(std::int32_t count, std::size_t maximum, const char* description) {
    if (count < 0) {
        throw std::runtime_error(std::string("Negative ") + description + " size");
    }
    if (static_cast<std::size_t>(count) > maximum) {
        throw std::runtime_error(std::string("Excessive ") + description + " size");
    }
}

Nbt readPayloadAtDepth(const std::uint8_t typeId,
                       const std::vector<std::uint8_t>& data,
                       std::size_t& pos,
                       std::size_t depth) {
    if (!validType(typeId)) {
        throw std::runtime_error("Unknown NBT tag type");
    }
    if (depth > kMaxNbtDepth) {
        throw std::runtime_error("NBT nesting depth exceeds safety limit");
    }
    const auto type = static_cast<Nbt::Type>(typeId);
    switch (type) {
        case Nbt::Type::End:
            return Nbt();
        case Nbt::Type::Byte:
            return Nbt(static_cast<std::int8_t>(binary::readU8(data, pos)));
        case Nbt::Type::Short:
            return Nbt(binary::readI16BE(data, pos));
        case Nbt::Type::Int:
            return Nbt(binary::readI32BE(data, pos));
        case Nbt::Type::Long:
            return Nbt(binary::readI64BE(data, pos));
        case Nbt::Type::Float:
            {
                const std::uint32_t bits = binary::readU32BE(data, pos);
                float value = 0.0F;
                std::memcpy(&value, &bits, sizeof(float));
                return Nbt(value);
            }
        case Nbt::Type::Double:
            {
                const std::uint64_t bits = binary::readU64BE(data, pos);
                double value = 0.0;
                std::memcpy(&value, &bits, sizeof(double));
                return Nbt(value);
            }
        case Nbt::Type::ByteArray:
            {
                const std::int32_t size = binary::readI32BE(data, pos);
                validateCount(size, data.size() - std::min(pos, data.size()), "byte array");
                Nbt::ByteArray bytes(static_cast<std::size_t>(size));
                binary::readBytes(data, pos, bytes.data(), bytes.size());
                return Nbt(std::move(bytes));
            }
        case Nbt::Type::String:
            return Nbt(binary::readModifiedUtf8(data, pos));
        case Nbt::Type::List:
            {
                const auto itemType = static_cast<Nbt::Type>(binary::readU8(data, pos));
                if (!validType(static_cast<std::uint8_t>(itemType))) {
                    throw std::runtime_error("Unknown NBT list element type");
                }
                const std::int32_t size = binary::readI32BE(data, pos);
                validateCount(size, kMaxContainerElements, "NBT list");
                Nbt tag = Nbt::list();
                auto& items = tag.asList();
                items.reserve(static_cast<std::size_t>(size));
                for (std::int32_t i = 0; i < size; ++i) {
                    items.push_back(readPayloadAtDepth(static_cast<std::uint8_t>(itemType), data, pos, depth + 1));
                }
                return tag;
            }
        case Nbt::Type::Compound:
            {
                Nbt tag = Nbt::compound();
                auto& items = tag.asCompound();
                while (true) {
                    const auto childType = static_cast<Nbt::Type>(binary::readU8(data, pos));
                    if (childType == Nbt::Type::End) {
                        break;
                    }
                    if (!validType(static_cast<std::uint8_t>(childType))) {
                        throw std::runtime_error("Unknown NBT compound child type");
                    }
                    const std::string key = binary::readModifiedUtf8(data, pos);
                    items.insert_or_assign(
                        key, readPayloadAtDepth(static_cast<std::uint8_t>(childType), data, pos, depth + 1));
                    if (items.size() > kMaxContainerElements) {
                        throw std::runtime_error("NBT compound exceeds safety limit");
                    }
                }
                return tag;
            }
        case Nbt::Type::IntArray:
            {
                const std::int32_t size = binary::readI32BE(data, pos);
                validateCount(size, (data.size() - std::min(pos, data.size())) / 4U, "int array");
                Nbt::IntArray values(static_cast<std::size_t>(size));
                binary::readI32BEArray(data, pos, values.data(), values.size());
                return Nbt(std::move(values));
            }
        case Nbt::Type::LongArray:
            {
                const std::int32_t size = binary::readI32BE(data, pos);
                validateCount(size, (data.size() - std::min(pos, data.size())) / 8U, "long array");
                Nbt::LongArray values(static_cast<std::size_t>(size));
                binary::readI64BEArray(data, pos, values.data(), values.size());
                return Nbt(std::move(values));
            }
    }
    throw std::runtime_error("Unknown NBT tag type");
}
}  // namespace

Nbt readPayload(const std::uint8_t typeId, const std::vector<std::uint8_t>& data, std::size_t& pos) {
    return readPayloadAtDepth(typeId, data, pos, 0);
}

Nbt readNamedTag(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    const std::uint8_t typeId = binary::readU8(data, pos);
    if (!validType(typeId)) {
        throw std::runtime_error("Unknown root NBT tag type");
    }
    const auto type = static_cast<Nbt::Type>(typeId);
    if (type == Nbt::Type::End) {
        return Nbt();
    }
    (void) binary::readModifiedUtf8(data, pos);
    return readPayloadAtDepth(typeId, data, pos, 0);
}

void writePayload(std::vector<std::uint8_t>& out, const Nbt& tag) {
    switch (tag.type()) {
        case Nbt::Type::End:
            return;
        case Nbt::Type::Byte:
            binary::appendU8(out, static_cast<std::uint8_t>(tag.asByte()));
            return;
        case Nbt::Type::Short:
            binary::appendI16BE(out, tag.asShort());
            return;
        case Nbt::Type::Int:
            binary::appendI32BE(out, tag.asInt());
            return;
        case Nbt::Type::Long:
            binary::appendI64BE(out, tag.asLong());
            return;
        case Nbt::Type::Float:
            {
                const float value = tag.asFloat();
                std::uint32_t bits = 0;
                std::memcpy(&bits, &value, sizeof(float));
                binary::appendU32BE(out, bits);
                return;
            }
        case Nbt::Type::Double:
            {
                const double value = tag.asDouble();
                std::uint64_t bits = 0;
                std::memcpy(&bits, &value, sizeof(double));
                binary::appendU64BE(out, bits);
                return;
            }
        case Nbt::Type::ByteArray:
            {
                const Nbt::ByteArray& bytes = tag.asByteArray();
                binary::appendI32BE(out, static_cast<std::int32_t>(bytes.size()));
                binary::appendBytes(out, bytes.data(), bytes.size());
                return;
            }
        case Nbt::Type::String:
            binary::writeModifiedUtf8(out, tag.asString());
            return;
        case Nbt::Type::List:
            {
                const auto& list = tag.asList();
                // Beta 1.7.3 writes TAG_Byte (1) for empty lists, not TAG_End (0).
                const Nbt::Type itemType = list.empty() ? Nbt::Type::Byte : list.front().type();
                binary::appendU8(out, static_cast<std::uint8_t>(itemType));
                binary::appendI32BE(out, static_cast<std::int32_t>(list.size()));
                for (const Nbt& item : list) {
                    if (item.type() != itemType) {
                        throw std::runtime_error("NBT lists must contain a single tag type");
                    }
                    writePayload(out, item);
                }
                return;
            }
        case Nbt::Type::Compound:
            {
                for (const auto& [key, child] : tag.asCompound()) {
                    writeNamedTag(out, key, child);
                }
                binary::appendU8(out, 0);
                return;
            }
        case Nbt::Type::IntArray:
            {
                const Nbt::IntArray& values = tag.asIntArray();
                binary::appendI32BE(out, static_cast<std::int32_t>(values.size()));
                binary::appendI32BEArray(out, values.data(), values.size());
                return;
            }
        case Nbt::Type::LongArray:
            {
                const Nbt::LongArray& values = tag.asLongArray();
                binary::appendI32BE(out, static_cast<std::int32_t>(values.size()));
                binary::appendI64BEArray(out, values.data(), values.size());
                return;
            }
    }
}

void writeNamedTag(std::vector<std::uint8_t>& out, const std::string& name, const Nbt& tag) {
    binary::appendU8(out, static_cast<std::uint8_t>(tag.type()));
    if (tag.type() == Nbt::Type::End) {
        return;
    }
    binary::writeModifiedUtf8(out, name);
    writePayload(out, tag);
}
}  // namespace net::minecraft::nbt::detail
