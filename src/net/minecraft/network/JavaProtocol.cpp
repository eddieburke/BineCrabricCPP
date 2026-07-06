#include "net/minecraft/network/JavaProtocol.hpp"
namespace net::minecraft::network::java {
HandshakePacket makeClientHandshake(const std::string& username) {
  return HandshakePacket(username);
}
LoginHelloPacket makeClientLoginHello(const std::string& username) {
  return LoginHelloPacket(username, kProtocolVersionBeta173);
}
ServerLogin decodeServerLogin(const LoginHelloPacket& packet) {
  return ServerLogin{packet.protocolVersion, packet.worldSeed, packet.dimensionId};
}
ClientboundPlayerMove decodeClientboundPlayerMove(const PlayerMovePacket& packet) {
  return ClientboundPlayerMove{packet.x, packet.y, packet.eyeHeight, packet.z, packet.yaw,
                               packet.pitch, packet.onGround, packet.changePosition, packet.changeLook};
}
ClientboundPlayerMove makeClientboundPlayerMove(double x, double stanceY, double feetY, double z, float yaw,
                                                float pitch, bool onGround, bool changePosition, bool changeLook) {
  return ClientboundPlayerMove{x, stanceY, feetY, z, yaw, pitch, onGround, changePosition, changeLook};
}
void encodeClientboundPlayerMove(PlayerMovePacket& packet, const ClientboundPlayerMove& move) {
  packet.x = move.x;
  packet.y = move.stanceY;
  packet.eyeHeight = move.feetY;
  packet.z = move.z;
  packet.yaw = move.yaw;
  packet.pitch = move.pitch;
  packet.onGround = move.onGround;
}
ServerboundPlayerMove decodeServerboundPlayerMove(const PlayerMovePacket& packet) {
  return ServerboundPlayerMove{packet.x, packet.y, packet.eyeHeight, packet.z, packet.yaw,
                               packet.pitch, packet.onGround, packet.changePosition, packet.changeLook};
}
ServerboundPlayerMove makeServerboundPlayerMove(double x, double feetY, double stanceY, double z, float yaw,
                                                float pitch, bool onGround, bool changePosition, bool changeLook) {
  return ServerboundPlayerMove{x, feetY, stanceY, z, yaw, pitch, onGround, changePosition, changeLook};
}
void encodeServerboundPlayerMove(PlayerMovePacket& packet, const ServerboundPlayerMove& move) {
  packet.x = move.x;
  packet.y = move.feetY;
  packet.eyeHeight = move.stanceY;
  packet.z = move.z;
  packet.yaw = move.yaw;
  packet.pitch = move.pitch;
  packet.onGround = move.onGround;
}
} // namespace net::minecraft::network::java
