#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
namespace net::minecraft {
inline constexpr int kProtocolVersionBeta173 = 14;
class KeepAlivePacket : public Packet {
 public:
 void read(std::istream&) override {
 }
 void write(std::ostream&) const override {
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
 void read(std::istream& input) override {
  protocolVersion = packetio::readI32BE(input);
  username = Packet::readString(input, 16);
  worldSeed = static_cast<std::uint64_t>(packetio::readI64BE(input));
  dimensionId = packetio::readI8(input);
 }
 void write(std::ostream& output) const override {
  packetio::writeI32BE(output, protocolVersion);
  Packet::writeString(username, output);
  packetio::writeI64BE(output, static_cast<std::int64_t>(worldSeed));
  packetio::writeI8(output, dimensionId);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onHello(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4U + username.size() + 4U + 5U;
 }
};
class HandshakePacket : public Packet {
 public:
 HandshakePacket() = default;
 explicit HandshakePacket(std::string name) : name(std::move(name)) {
 }
 std::string name;
 void read(std::istream& input) override {
  name = Packet::readString(input, 4096);
 }
 void write(std::ostream& output) const override {
  Packet::writeString(name, output);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onHandshake(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return 4U + name.size() + 4U;
 }
};
class DisconnectPacket : public Packet {
 public:
 std::string reason;
 void read(std::istream& input) override {
  reason = Packet::readString(input, 100);
 }
 void write(std::ostream& output) const override {
  Packet::writeString(reason, output);
 }
 void apply(NetworkHandler& networkHandler) const override {
  networkHandler.onDisconnect(*this);
 }
 [[nodiscard]] std::size_t size() const override {
  return reason.size();
 }
};
} // namespace net::minecraft
