#pragma once
#include <cstddef>
#include <string>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
class ChatMessagePacket : public Packet {
 public:
 std::string chatMessage;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  chatMessage = Packet::readString(src, end, 119);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  Packet::writeString(chatMessage, dest, end);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onChatMessage(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return packetio::javaStringSize(chatMessage);
 }
};
} // namespace net::minecraft
