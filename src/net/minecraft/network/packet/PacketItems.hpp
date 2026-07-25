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
 // Read into locals first: argument evaluation order is unspecified, so passing the
 // reads directly as arguments lets the compiler consume the count and damage fields
 // out of wire order.
 const std::int8_t count = packetio::readI8(input);
 const std::int16_t damage = packetio::readI16BE(input);
 return ItemStack(itemId, count, damage);
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
