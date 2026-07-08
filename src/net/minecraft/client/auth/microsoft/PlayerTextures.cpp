#include "net/minecraft/client/auth/microsoft/PlayerTextures.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"

namespace msauth {
namespace {
void queueTextureDownloads(net::minecraft::client::texture::TextureManager& textureManager,
                           const net::minecraft::entity::player::PlayerEntity& player) {
    if (!player.skinUrl.empty()) {
        textureManager.downloadSkinImage(player.skinUrl);
    }
    if (!player.capeUrl.empty()) {
        textureManager.downloadCapeImage(player.capeUrl);
    }
}
}  // namespace

void applySessionTextures(net::minecraft::entity::player::PlayerEntity& player,
                          const net::minecraft::client::util::Session& session) {
    const std::string resolvedSkinUrl = !session.skinUrl.empty() ? session.skinUrl : legacySkinUrl(session.username);
    if (!resolvedSkinUrl.empty()) {
        player.skinUrl = resolvedSkinUrl;
    }
    player.profileCapeUrl = session.capeUrl;
    if (!session.username.empty()) {
        player.name = session.username;
    }
    player.updateCapeUrl();
}

void refreshPlayerTextures(net::minecraft::client::Minecraft& client, bool slimArms) {
    if (client.player == nullptr) {
        return;
    }
    net::minecraft::entity::player::PlayerEntity& player = *client.player;
    const std::string previousSkinUrl = player.skinUrl;
    const std::string previousCapeUrl = player.capeUrl;
    applySessionTextures(player, client.session);
    player.slimArms = slimArms;
    if (!previousSkinUrl.empty() && previousSkinUrl != player.skinUrl) {
        client.textureManager.releaseImage(previousSkinUrl);
    }
    if (!previousCapeUrl.empty() && previousCapeUrl != player.capeUrl) {
        client.textureManager.releaseImage(previousCapeUrl);
    }
    queueTextureDownloads(client.textureManager, player);
}
}  // namespace msauth
