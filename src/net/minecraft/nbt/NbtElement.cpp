#include "net/minecraft/nbt/NbtElement.hpp"

#include "net/minecraft/nbt/NbtByte.hpp"
#include "net/minecraft/nbt/NbtByteArray.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtDouble.hpp"
#include "net/minecraft/nbt/NbtEnd.hpp"
#include "net/minecraft/nbt/NbtFloat.hpp"
#include "net/minecraft/nbt/NbtInt.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/nbt/NbtLong.hpp"
#include "net/minecraft/nbt/NbtShort.hpp"
#include "net/minecraft/nbt/NbtString.hpp"

#include <cstring>
#include <sstream>

namespace net::minecraft {
namespace {

Nbt readPayload(Nbt::Type type, const std::vector<std::uint8_t>& data, std::size_t& pos)
{
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
    case Nbt::Type::Float: {
        const std::uint32_t bits = binary::readU32BE(data, pos);
        float value = 0.0F;
        std::memcpy(&value, &bits, sizeof(float));
        return Nbt(value);
    }
    case Nbt::Type::Double: {
        const std::uint64_t bits = binary::readU64BE(data, pos);
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(double));
        return Nbt(value);
    }
    case Nbt::Type::ByteArray: {
        const std::int32_t size = binary::readI32BE(data, pos);
        if (size < 0) {
            throw std::runtime_error("Negative byte array size");
        }
        Nbt::ByteArray bytes(static_cast<std::size_t>(size));
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] = binary::readU8(data, pos);
        }
        return Nbt(std::move(bytes));
    }
    case Nbt::Type::String:
        return Nbt(binary::readModifiedUtf8(data, pos));
    case Nbt::Type::List: {
        const auto itemType = static_cast<Nbt::Type>(binary::readU8(data, pos));
        const std::int32_t size = binary::readI32BE(data, pos);
        if (size < 0) {
            throw std::runtime_error("Negative NBT list size");
        }
        Nbt tag = Nbt::list();
        auto& items = tag.asList();
        items.reserve(static_cast<std::size_t>(size));
        for (std::int32_t i = 0; i < size; ++i) {
            items.push_back(readPayload(itemType, data, pos));
        }
        return tag;
    }
    case Nbt::Type::Compound: {
        Nbt tag = Nbt::compound();
        auto& items = tag.asCompound();
        while (true) {
            const auto childType = static_cast<Nbt::Type>(binary::readU8(data, pos));
            if (childType == Nbt::Type::End) {
                break;
            }
            const std::string key = binary::readModifiedUtf8(data, pos);
            items[key] = readPayload(childType, data, pos);
        }
        return tag;
    }
    default:
        throw std::runtime_error("Unknown NBT tag type");
    }
}

void writePayload(std::vector<std::uint8_t>& out, const Nbt& tag)
{
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
    case Nbt::Type::Float: {
        const float value = tag.asFloat();
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(float));
        binary::appendU32BE(out, bits);
        return;
    }
    case Nbt::Type::Double: {
        const double value = tag.asDouble();
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(double));
        binary::appendU64BE(out, bits);
        return;
    }
    case Nbt::Type::ByteArray: {
        const Nbt::ByteArray& bytes = tag.asByteArray();
        binary::appendI32BE(out, static_cast<std::int32_t>(bytes.size()));
        out.insert(out.end(), bytes.begin(), bytes.end());
        return;
    }
    case Nbt::Type::String:
        binary::writeModifiedUtf8(out, tag.asString());
        return;
    case Nbt::Type::List: {
        const auto& list = tag.asList();
        // Beta 1.7.3 writes TAG_Byte (1) for empty lists, not TAG_End (0).
        const Nbt::Type itemType = list.empty() ? Nbt::Type::Byte : list.front().type();
        binary::appendU8(out, static_cast<std::uint8_t>(itemType));
        binary::appendI32BE(out, static_cast<std::int32_t>(list.size()));
        for (const Nbt& item : list) {
            writePayload(out, item);
        }
        return;
    }
    case Nbt::Type::Compound: {
        for (const auto& [key, child] : tag.asCompound()) {
            binary::appendU8(out, static_cast<std::uint8_t>(child.type()));
            binary::writeModifiedUtf8(out, key);
            writePayload(out, child);
        }
        binary::appendU8(out, 0);
        return;
    }
    default:
        return;
    }
}

} // namespace

void NbtByte::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::appendU8(bytes, static_cast<std::uint8_t>(value));
    binary::writeAllBytes(output, bytes);
}

void NbtByte::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    value = static_cast<std::int8_t>(binary::readU8(bytes, pos));
}

void NbtShort::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::appendI16BE(bytes, value);
    binary::writeAllBytes(output, bytes);
}

void NbtShort::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    value = binary::readI16BE(bytes, pos);
}

void NbtInt::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::appendI32BE(bytes, value);
    binary::writeAllBytes(output, bytes);
}

void NbtInt::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    value = binary::readI32BE(bytes, pos);
}

void NbtLong::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::appendI64BE(bytes, value);
    binary::writeAllBytes(output, bytes);
}

void NbtLong::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    value = binary::readI64BE(bytes, pos);
}

void NbtFloat::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    binary::appendU32BE(bytes, bits);
    binary::writeAllBytes(output, bytes);
}

void NbtFloat::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    const std::uint32_t bits = binary::readU32BE(bytes, pos);
    std::memcpy(&value, &bits, sizeof(float));
}

void NbtDouble::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(double));
    binary::appendU64BE(bytes, bits);
    binary::writeAllBytes(output, bytes);
}

void NbtDouble::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    const std::uint64_t bits = binary::readU64BE(bytes, pos);
    std::memcpy(&value, &bits, sizeof(double));
}

void NbtString::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::writeModifiedUtf8(bytes, value);
    binary::writeAllBytes(output, bytes);
}

void NbtString::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    value = binary::readModifiedUtf8(bytes, pos);
}

void NbtByteArray::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    binary::appendI32BE(bytes, static_cast<std::int32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
    binary::writeAllBytes(output, bytes);
}

void NbtByteArray::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    const std::int32_t size = binary::readI32BE(bytes, pos);
    if (size < 0) {
        throw std::runtime_error("Negative byte array size");
    }
    value.resize(static_cast<std::size_t>(size));
    for (std::size_t i = 0; i < value.size(); ++i) {
        value[i] = binary::readU8(bytes, pos);
    }
}

void NbtCompound::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    writePayload(bytes, tag_);
    binary::writeAllBytes(output, bytes);
}

void NbtCompound::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    tag_ = readPayload(Nbt::Type::Compound, bytes, pos);
}

void NbtCompound::put(const std::string& key, std::unique_ptr<NbtElement> value)
{
    if (value == nullptr) {
        return;
    }
    value->setKey(key);
    tag_.put(key, value->toStorage());
}

std::vector<std::uint8_t> NbtCompound::getByteArray(const std::string& key) const
{
    return tag_.getByteArray(key);
}

NbtCompound NbtCompound::getCompound(const std::string& key) const
{
    if (const Nbt* child = tag_.get(key); child != nullptr && child->isCompound()) {
        return NbtCompound(*child);
    }
    return NbtCompound();
}

NbtList NbtCompound::getList(const std::string& key) const
{
    if (const Nbt* child = tag_.get(key); child != nullptr && child->isList()) {
        return NbtList(*child);
    }
    return NbtList();
}

void NbtList::write(std::ostream& output) const
{
    std::vector<std::uint8_t> bytes;
    writePayload(bytes, tag_);
    binary::writeAllBytes(output, bytes);
}

void NbtList::read(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    tag_ = readPayload(Nbt::Type::List, bytes, pos);
}

void NbtList::add(std::unique_ptr<NbtElement> element)
{
    if (element == nullptr) {
        return;
    }
    tag_.asList().push_back(element->toStorage());
}

std::unique_ptr<NbtElement> NbtElement::createTypeFromId(const std::uint8_t id)
{
    switch (id) {
    case 0:
        return std::make_unique<NbtEnd>();
    case 1:
        return std::make_unique<NbtByte>();
    case 2:
        return std::make_unique<NbtShort>();
    case 3:
        return std::make_unique<NbtInt>();
    case 4:
        return std::make_unique<NbtLong>();
    case 5:
        return std::make_unique<NbtFloat>();
    case 6:
        return std::make_unique<NbtDouble>();
    case 7:
        return std::make_unique<NbtByteArray>();
    case 8:
        return std::make_unique<NbtString>();
    case 9:
        return std::make_unique<NbtList>();
    case 10:
        return std::make_unique<NbtCompound>();
    default:
        return nullptr;
    }
}

std::string NbtElement::getTypeNameFromId(const std::uint8_t id)
{
    switch (id) {
    case 0:
        return "TAG_End";
    case 1:
        return "TAG_Byte";
    case 2:
        return "TAG_Short";
    case 3:
        return "TAG_Int";
    case 4:
        return "TAG_Long";
    case 5:
        return "TAG_Float";
    case 6:
        return "TAG_Double";
    case 7:
        return "TAG_Byte_Array";
    case 8:
        return "TAG_String";
    case 9:
        return "TAG_List";
    case 10:
        return "TAG_Compound";
    default:
        return "UNKNOWN";
    }
}

std::unique_ptr<NbtElement> NbtElement::fromStorage(const Nbt& tag)
{
    switch (tag.type()) {
    case Nbt::Type::End:
        return std::make_unique<NbtEnd>();
    case Nbt::Type::Byte: {
        auto out = std::make_unique<NbtByte>();
        out->value = tag.asByte();
        return out;
    }
    case Nbt::Type::Short: {
        auto out = std::make_unique<NbtShort>();
        out->value = tag.asShort();
        return out;
    }
    case Nbt::Type::Int: {
        auto out = std::make_unique<NbtInt>();
        out->value = tag.asInt();
        return out;
    }
    case Nbt::Type::Long: {
        auto out = std::make_unique<NbtLong>();
        out->value = tag.asLong();
        return out;
    }
    case Nbt::Type::Float: {
        auto out = std::make_unique<NbtFloat>();
        out->value = tag.asFloat();
        return out;
    }
    case Nbt::Type::Double: {
        auto out = std::make_unique<NbtDouble>();
        out->value = tag.asDouble();
        return out;
    }
    case Nbt::Type::ByteArray: {
        auto out = std::make_unique<NbtByteArray>();
        out->value = tag.asByteArray();
        return out;
    }
    case Nbt::Type::String: {
        auto out = std::make_unique<NbtString>();
        out->value = tag.asString();
        return out;
    }
    case Nbt::Type::List:
        return std::make_unique<NbtList>(tag);
    case Nbt::Type::Compound:
        return std::make_unique<NbtCompound>(tag);
    default:
        return nullptr;
    }
}

std::unique_ptr<NbtElement> NbtElement::readTag(std::istream& input)
{
    const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
    std::size_t pos = 0;
    const auto type = static_cast<Nbt::Type>(binary::readU8(bytes, pos));
    if (type == Nbt::Type::End) {
        return std::make_unique<NbtEnd>();
    }

    const std::string key = binary::readModifiedUtf8(bytes, pos);
    std::unique_ptr<NbtElement> element = fromStorage(readPayload(type, bytes, pos));
    if (element != nullptr) {
        element->setKey(key);
    }
    return element;
}

void NbtElement::writeTag(const NbtElement& element, std::ostream& output)
{
    std::vector<std::uint8_t> bytes;
    binary::appendU8(bytes, element.getType());
    if (element.getType() == 0) {
        binary::writeAllBytes(output, bytes);
        return;
    }

    binary::writeModifiedUtf8(bytes, element.getKey());
    writePayload(bytes, element.toStorage());
    binary::writeAllBytes(output, bytes);
}

} // namespace net::minecraft
