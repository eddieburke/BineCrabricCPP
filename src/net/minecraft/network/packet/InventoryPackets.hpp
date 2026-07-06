#pragma once
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/network/packet/PacketItems.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <vector>
namespace net::minecraft {
class OpenScreenS2CPacket : public Packet {
public:
  int syncId = 0;
  int screenHandlerId = 0;
  std::string name;
  int inventorySize = 0;
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    screenHandlerId = packetio::readU8(input);
    name = packetio::readUtfString(input);
    inventorySize = packetio::readU8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeU8(output, static_cast<std::uint8_t>(screenHandlerId));
    packetio::writeUtfString(output, name);
    packetio::writeU8(output, static_cast<std::uint8_t>(inventorySize));
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onOpenScreen(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 3U + name.size();
  }
};
class CloseScreenS2CPacket : public Packet {
public:
  int syncId = 0;
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
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
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    slot = packetio::readI16BE(input);
    button = packetio::readU8(input);
    actionType = packetio::readI16BE(input);
    holdingShift = packetio::readBoolean(input);
    stack = packetitems::readOptionalItemStack(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(slot));
    packetio::writeU8(output, static_cast<std::uint8_t>(button));
    packetio::writeI16BE(output, actionType);
    packetio::writeBoolean(output, holdingShift);
    packetitems::writeOptionalItemStack(output, stack);
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onClickSlot(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 11;
  }
};
class ScreenHandlerSlotUpdateS2CPacket : public Packet {
public:
  int syncId = 0;
  int slot = 0;
  ItemStack stack;
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    slot = packetio::readI16BE(input);
    stack = packetitems::readOptionalItemStack(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(slot));
    packetitems::writeOptionalItemStack(output, stack);
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
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    const int count = packetio::readI16BE(input);
    contents.clear();
    contents.resize(static_cast<std::size_t>(count));
    for(int i = 0; i < count; ++i) {
      const std::int16_t itemId = packetio::readI16BE(input);
      if(itemId < 0) {
        contents[static_cast<std::size_t>(i)] = {};
        continue;
      }
      contents[static_cast<std::size_t>(i)] =
          ItemStack(itemId, packetio::readI8(input), packetio::readI16BE(input));
    }
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(contents.size()));
    for(const ItemStack& s : contents) {
      packetitems::writeOptionalItemStack(output, s);
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
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    propertyId = packetio::readI16BE(input);
    value = packetio::readI16BE(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(propertyId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(value));
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
  void read(std::istream& input) override {
    syncId = packetio::readU8(input);
    actionType = packetio::readI16BE(input);
    accepted = packetio::readU8(input) != 0;
  }
  void write(std::ostream& output) const override {
    packetio::writeU8(output, static_cast<std::uint8_t>(syncId));
    packetio::writeI16BE(output, actionType);
    packetio::writeU8(output, accepted ? 1U : 0U);
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
  void read(std::istream& input) override {
    x = packetio::readI32BE(input);
    y = packetio::readI16BE(input);
    z = packetio::readI32BE(input);
    for(std::size_t i = 0; i < text.size(); ++i) {
      text[i] = Packet::readString(input, 15);
    }
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, x);
    packetio::writeI16BE(output, static_cast<std::int16_t>(y));
    packetio::writeI32BE(output, z);
    for(const std::string& line : text) {
      Packet::writeString(line, output);
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
  void read(std::istream& input) override {
    itemRawId = packetio::readI16BE(input);
    id = packetio::readI16BE(input);
    const std::uint8_t length = packetio::readU8(input);
    updateData = packetio::readBytes(input, length);
  }
  void write(std::ostream& output) const override {
    packetio::writeI16BE(output, static_cast<std::int16_t>(itemRawId));
    packetio::writeI16BE(output, static_cast<std::int16_t>(id));
    packetio::writeU8(output, static_cast<std::uint8_t>(updateData.size()));
    packetio::writeBytes(output, updateData);
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
  void read(std::istream& input) override {
    statId = packetio::readI32BE(input);
    amount = packetio::readI8(input);
  }
  void write(std::ostream& output) const override {
    packetio::writeI32BE(output, statId);
    packetio::writeI8(output, static_cast<std::int8_t>(amount));
  }
  void apply(NetworkHandler& networkHandler) const override {
    networkHandler.onIncreaseStat(*this);
  }
  [[nodiscard]] std::size_t size() const override {
    return 6;
  }
};
} // namespace net::minecraft
