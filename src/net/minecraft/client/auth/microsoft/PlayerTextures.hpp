#pragma once
// Applies authenticated profile textures to client player entities.
#include "net/minecraft/client/util/Session.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace msauth {
void applySessionTextures(net::minecraft::entity::player::PlayerEntity& player,
                          const net::minecraft::client::util::Session& session);
void refreshPlayerTextures(net::minecraft::client::Minecraft& client);
} // namespace msauth
