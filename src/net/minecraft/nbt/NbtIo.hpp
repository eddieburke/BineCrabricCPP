#pragma once
// Root-tag file I/O (named compound, optional gzip).
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include <istream>
#include <ostream>
#include <stdexcept>
#include <vector>
namespace net::minecraft {
class NbtIo {
public:
  [[nodiscard]] static NbtCompound read(std::istream& stream) {
    const Nbt root = Nbt::read(stream);
    if(!root.isCompound()) {
      throw std::runtime_error("Root tag must be a named compound tag");
    }
    return NbtCompound(root);
  }
  [[nodiscard]] static NbtCompound read(const std::vector<std::uint8_t>& bytes) {
    const Nbt root = Nbt::read(bytes);
    if(!root.isCompound()) {
      throw std::runtime_error("Root tag must be a named compound tag");
    }
    return NbtCompound(root);
  }
  [[nodiscard]] static NbtCompound readCompressed(std::istream& stream) {
    const Nbt root = Nbt::readCompressed(stream);
    if(!root.isCompound()) {
      throw std::runtime_error("Root tag must be a named compound tag");
    }
    return NbtCompound(root);
  }
  [[nodiscard]] static NbtCompound readCompressed(const std::vector<std::uint8_t>& bytes) {
    const Nbt root = Nbt::readCompressed(bytes);
    if(!root.isCompound()) {
      throw std::runtime_error("Root tag must be a named compound tag");
    }
    return NbtCompound(root);
  }
  static void write(const NbtCompound& tag, std::ostream& stream) {
    tag.storage().write(stream);
  }
  static void writeCompressed(const NbtCompound& tag, std::ostream& stream) {
    tag.storage().writeCompressed(stream);
  }
  static void writeCompressed(const std::vector<std::uint8_t>& bytes, std::ostream& stream) {
    const std::vector<std::uint8_t> compressed = gzipCompress(bytes);
    binary::writeAllBytes(stream, compressed);
  }
};
} // namespace net::minecraft
