#pragma once
#include <cstdint>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft::packetitems {
inline ItemStack readOptionalItemStack(const std::uint8_t*& src, const std::uint8_t* end) {
 const std::int16_t itemId = packetio::readI16BE(src, end);
 if(itemId < 0) {
  return {};
 }
 const std::int8_t count = packetio::readI8(src, end);
 const std::int16_t damage = packetio::readI16BE(src, end);
 return ItemStack(itemId, count, damage);
}
inline void writeOptionalItemStack(std::uint8_t*& dest, std::uint8_t* end, const ItemStack& stack) {
 if(stack.itemId <= 0) {
  packetio::writeI16BE(dest, end, -1);
  return;
 }
 packetio::writeI16BE(dest, end, static_cast<std::int16_t>(stack.itemId));
 packetio::writeI8(dest, end, static_cast<std::int8_t>(stack.count));
 packetio::writeI16BE(dest, end, static_cast<std::int16_t>(stack.getDamage()));
}
} // namespace net::minecraft::packetitems
