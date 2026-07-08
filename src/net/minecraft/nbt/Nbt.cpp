#include "net/minecraft/nbt/Nbt.hpp"

#include <ostream>
#include <stdexcept>

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/nbt/NbtBinaryCodec.hpp"

namespace net::minecraft {
void Nbt::ensureType(Type expected) const {
    if (type_ != expected) {
        throw std::logic_error("NBT type mismatch");
    }
}

void Nbt::write(std::ostream& out) const {
    binary::writeAllBytes(out, toBytes());
}

void Nbt::writeCompressed(std::ostream& out) const {
    binary::writeAllBytes(out, gzipCompress(toBytes()));
}

std::vector<std::uint8_t> Nbt::toBytes() const {
    std::vector<std::uint8_t> bytes;
    nbt::detail::writeNamedTag(bytes, "", *this);
    return bytes;
}

Nbt Nbt::read(std::istream& in) {
    return read(binary::readAllBytes(in));
}

Nbt Nbt::read(const std::vector<std::uint8_t>& bytes) {
    std::size_t pos = 0;
    return nbt::detail::readNamedTag(bytes, pos);
}

Nbt Nbt::readCompressed(std::istream& in) {
    return readCompressed(binary::readAllBytes(in));
}

Nbt Nbt::readCompressed(const std::vector<std::uint8_t>& bytes) {
    return read(gzipDecompress(bytes));
}
}  // namespace net::minecraft
