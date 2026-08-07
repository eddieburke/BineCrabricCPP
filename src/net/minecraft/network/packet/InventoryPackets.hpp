#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/PacketItems.hpp"
namespace net::minecraft {
class OpenScreenS2CPacket : public Packet {
 public:
 int syncId = 0;
 int screenHandlerId = 0;
 std::string name;
 int inventorySize = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  screenHandlerId = packetio::readU8(src, end);
  name = packetio::readUtfString(src, end);
  inventorySize = packetio::readU8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(screenHandlerId));
  packetio::writeUtfString(dest, end, name);
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(inventorySize));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onOpenScreen(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 3U + packetio::utfStringSize(name);
 }
};
class CloseScreenS2CPacket : public Packet {
 public:
 int syncId = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onCloseScreen(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 1;
 }
};
class ClickSlotC2SPacket : public Packet {
 public:
 int syncId = 0;
 int slot = 0;
 int button = 0;
 std::int16_t actionType = 0;
 ItemStack stack;
 bool holdingShift = false;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  slot = packetio::readI16BE(src, end);
  button = packetio::readU8(src, end);
  actionType = packetio::readI16BE(src, end);
  holdingShift = packetio::readBoolean(src, end);
  stack = packetitems::readOptionalItemStack(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(slot));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(button));
  packetio::writeI16BE(dest, end, actionType);
  packetio::writeBoolean(dest, end, holdingShift);
  packetitems::writeOptionalItemStack(dest, end, stack);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onClickSlot(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 7U + (stack.itemId <= 0 ? 2U : 5U);
 }
};
class ScreenHandlerSlotUpdateS2CPacket : public Packet {
 public:
 int syncId = 0;
 int slot = 0;
 ItemStack stack;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  slot = packetio::readI16BE(src, end);
  stack = packetitems::readOptionalItemStack(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(slot));
  packetitems::writeOptionalItemStack(dest, end, stack);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onScreenHandlerSlotUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 8;
 }
};
class InventoryS2CPacket : public Packet {
 public:
 int syncId = 0;
 std::vector<ItemStack> contents;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  const int count = packetio::readI16BE(src, end);
  contents.clear();
  contents.resize(static_cast<std::size_t>(count));
  for(int i = 0; i < count; ++i) {
   contents[static_cast<std::size_t>(i)] = packetitems::readOptionalItemStack(src, end);
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(contents.size()));
  for(const ItemStack& s : contents) {
   packetitems::writeOptionalItemStack(dest, end, s);
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onInventory(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 3U + contents.size() * 5U;
 }
};
class ScreenHandlerPropertyUpdateS2CPacket : public Packet {
 public:
 int syncId = 0;
 int propertyId = 0;
 int value = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  propertyId = packetio::readI16BE(src, end);
  value = packetio::readI16BE(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(propertyId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(value));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onScreenHandlerPropertyUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 5;
 }
};
class ScreenHandlerAcknowledgementPacket : public Packet {
 public:
 int syncId = 0;
 std::int16_t actionType = 0;
 bool accepted = false;
 ScreenHandlerAcknowledgementPacket() = default;
 ScreenHandlerAcknowledgementPacket(int syncIdIn, std::int16_t actionTypeIn, bool acceptedIn)
     : syncId(syncIdIn), actionType(actionTypeIn), accepted(acceptedIn) {
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  syncId = packetio::readI8(src, end);
  actionType = packetio::readI16BE(src, end);
  accepted = packetio::readU8(src, end) != 0;
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(syncId));
  packetio::writeI16BE(dest, end, actionType);
  packetio::writeU8(dest, end, accepted ? 1U : 0U);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onScreenHandlerAcknowledgement(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4;
 }
};
class UpdateSignPacket : public Packet {
 public:
 int x = 0;
 int y = 0;
 int z = 0;
 std::array<std::string, 4> text{};
 UpdateSignPacket() {
  worldPacket = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  x = packetio::readI32BE(src, end);
  y = packetio::readI16BE(src, end);
  z = packetio::readI32BE(src, end);
  for(std::size_t i = 0; i < text.size(); ++i) {
   text[i] = Packet::readString(src, end, 15);
  }
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, x);
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(y));
  packetio::writeI32BE(dest, end, z);
  for(const std::string& line : text) {
   Packet::writeString(line, dest, end);
  }
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.handleUpdateSign(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  std::size_t total = 0;
  for(const std::string& line : text) {
   total += line.size();
  }
  return total;
 }
};
class MapUpdateS2CPacket : public Packet {
 public:
 int itemRawId = 0;
 int id = 0;
 std::vector<std::uint8_t> updateData;
 MapUpdateS2CPacket() {
  worldPacket = true;
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  itemRawId = packetio::readI16BE(src, end);
  id = packetio::readI16BE(src, end);
  const std::uint8_t length = packetio::readU8(src, end);
  updateData = packetio::readBytes(src, end, length);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(itemRawId));
  packetio::writeI16BE(dest, end, static_cast<std::int16_t>(id));
  packetio::writeU8(dest, end, static_cast<std::uint8_t>(updateData.size()));
  packetio::writeBytes(dest, end, updateData);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onMapUpdate(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4U + updateData.size();
 }
};
class IncreaseStatS2CPacket : public Packet {
 public:
 int statId = 0;
 int amount = 0;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  statId = packetio::readI32BE(src, end);
  amount = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, statId);
  packetio::writeI8(dest, end, static_cast<std::int8_t>(amount));
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onIncreaseStat(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 6;
 }
};
} // namespace net::minecraft
