#pragma once
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include <cstdint>
// Shared decode helpers used by the ClientNetworkHandler packet-handler translation
// units. The handler bodies are split by concern across ConnectionPacketHandlers
// (this file's owner, ClientNetworkHandler.cpp), WorldPacketHandlers,
// EntityPacketHandlers, PlayerPacketHandlers and ScreenPacketHandlers. They all
// implement methods of the single ClientNetworkHandler class (the NetworkHandler
// vtable), so the split is purely organisational -- no new indirection.
namespace net::minecraft::client::multiplayer::detail {
[[nodiscard]] inline ClientWorld* asClientWorld(World* worldPtr) {
  return dynamic_cast<ClientWorld*>(worldPtr);
}
[[nodiscard]] inline float decodePacketYaw(std::int8_t yaw) {
  return static_cast<float>(yaw * 360) / 256.0f;
}
[[nodiscard]] inline float decodePacketPitch(std::int8_t pitch) {
  return static_cast<float>(pitch * 360) / 256.0f;
}
} // namespace net::minecraft::client::multiplayer::detail
