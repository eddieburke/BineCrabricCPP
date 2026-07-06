#pragma once
// Shared Beta 1.7.3 Java protocol conversions used by client and server.
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include <cstdint>
#include <string>
namespace net::minecraft::network::java {
inline constexpr int kProtocolVersionBeta173 = 14;
struct ServerLogin {
  int playerEntityId = 0;
  std::uint64_t worldSeed = 0;
  int dimensionId = 0;
};
struct ClientboundPlayerMove {
  double x = 0.0;
  double stanceY = 0.0;
  double feetY = 0.0;
  double z = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  bool onGround = false;
  bool changePosition = false;
  bool changeLook = false;
};
struct ServerboundPlayerMove {
  double x = 0.0;
  double feetY = 0.0;
  double stanceY = 0.0;
  double z = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  bool onGround = false;
  bool changePosition = false;
  bool changeLook = false;
};
[[nodiscard]] HandshakePacket makeClientHandshake(const std::string& username);
[[nodiscard]] LoginHelloPacket makeClientLoginHello(const std::string& username);
[[nodiscard]] ServerLogin decodeServerLogin(const LoginHelloPacket& packet);
[[nodiscard]] ClientboundPlayerMove decodeClientboundPlayerMove(const PlayerMovePacket& packet);
[[nodiscard]] ClientboundPlayerMove makeClientboundPlayerMove(double x, double stanceY, double feetY, double z,
                                                              float yaw, float pitch, bool onGround,
                                                              bool changePosition, bool changeLook);
void encodeClientboundPlayerMove(PlayerMovePacket& packet, const ClientboundPlayerMove& move);
[[nodiscard]] ServerboundPlayerMove decodeServerboundPlayerMove(const PlayerMovePacket& packet);
[[nodiscard]] ServerboundPlayerMove makeServerboundPlayerMove(double x, double feetY, double stanceY, double z,
                                                              float yaw, float pitch, bool onGround,
                                                              bool changePosition, bool changeLook);
void encodeServerboundPlayerMove(PlayerMovePacket& packet, const ServerboundPlayerMove& move);
} // namespace net::minecraft::network::java
