#pragma once
#include <cstdint>
#include <string>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
inline constexpr int kProtocolVersionBeta173 = 14;
class KeepAlivePacket : public Packet {
 public:
 void read(const std::uint8_t*&, const std::uint8_t*) override {
 }
 void write(std::uint8_t*&, std::uint8_t*) const override {
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onKeepAlive(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 0;
 }
};
class LoginHelloPacket : public Packet {
 public:
 int protocolVersion = 14;
 std::string username;
 std::uint64_t worldSeed = 0;
 std::int8_t dimensionId = 0;
 LoginHelloPacket() = default;
 LoginHelloPacket(std::string usernameIn, int protocolVersionIn)
     : protocolVersion(protocolVersionIn), username(std::move(usernameIn)) {
 }
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  protocolVersion = packetio::readI32BE(src, end);
  username = Packet::readString(src, end, 16);
  worldSeed = static_cast<std::uint64_t>(packetio::readI64BE(src, end));
  dimensionId = packetio::readI8(src, end);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  packetio::writeI32BE(dest, end, protocolVersion);
  Packet::writeString(username, dest, end);
  packetio::writeI64BE(dest, end, static_cast<std::int64_t>(worldSeed));
  packetio::writeI8(dest, end, dimensionId);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onHello(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4U + packetio::javaStringSize(username) + 8U + 1U;
 }
};
class HandshakePacket : public Packet {
 public:
 HandshakePacket() = default;
 explicit HandshakePacket(std::string name) : name(std::move(name)) {
 }
 std::string name;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  name = Packet::readString(src, end, 4096);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  Packet::writeString(name, dest, end);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onHandshake(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return packetio::javaStringSize(name);
 }
};
class DisconnectPacket : public Packet {
 public:
 std::string reason;
 void read(const std::uint8_t*& src, const std::uint8_t* end) override {
  reason = Packet::readString(src, end, 100);
 }
 void write(std::uint8_t*& dest, std::uint8_t* end) const override {
  Packet::writeString(reason, dest, end);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onDisconnect(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return packetio::javaStringSize(reason);
 }
};
} // namespace net::minecraft
