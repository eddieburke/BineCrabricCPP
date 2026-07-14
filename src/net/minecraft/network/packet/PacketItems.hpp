#pragma once
#include <istream>
#include <ostream>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft::packetitems {
inline ItemStack readOptionalItemStack(std::istream& input) {
  const std::int16_t itemId = packetio::readI16BE(input);
  if(itemId < 0) {
    return {};
  }
  return ItemStack(itemId, packetio::readI8(input), packetio::readI16BE(input));
}
inline void writeOptionalItemStack(std::ostream& output, const ItemStack& stack) {
  if(stack.itemId <= 0) {
    packetio::writeI16BE(output, -1);
    return;
  }
  packetio::writeI16BE(output, static_cast<std::int16_t>(stack.itemId));
  packetio::writeI8(output, static_cast<std::int8_t>(stack.count));
  packetio::writeI16BE(output, static_cast<std::int16_t>(stack.getDamage()));
}
} // namespace net::minecraft::packetitems
