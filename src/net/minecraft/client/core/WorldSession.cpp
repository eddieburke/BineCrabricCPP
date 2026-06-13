#include "net/minecraft/client/core/WorldSession.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/core/ClientNetworkBridge.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::core {

// Out-of-line: ClientNetworkBridge is incomplete in the header (forward-declared), so the
// destructor that tears down networkBridge_/retiredNetworkBridges_ must be emitted here.
WorldSession::~WorldSession() = default;

void WorldSession::adoptNetworkBridge(std::unique_ptr<ClientNetworkBridge> bridge)
{
    // Replacing an existing connection (reconnect / dimension reset): park the old bridge for
    // deferred teardown rather than freeing it inline.
    retireNetworkBridge();
    networkBridge_ = std::move(bridge);
}

void WorldSession::retireNetworkBridge()
{
    if (networkBridge_ != nullptr) {
        retiredNetworkBridges_.push_back(std::move(networkBridge_));
    }
}

void WorldSession::flushRetiredNetwork()
{
    // Safe point (run loop top): the handler->tick() that retired these has fully unwound.
    retiredNetworkBridges_.clear();
}

void WorldSession::setWorld(Minecraft& client, World* worldIn)
{
    setWorld(client, worldIn, "");
}

void WorldSession::setWorld(Minecraft& client, World* worldIn, const std::string& message)
{
    setWorld(client, worldIn, message, nullptr);
}

void WorldSession::clearWorld(Minecraft& client)
{
    // Detach render listeners before ownedWorld_ is destroyed. WorldRenderer keeps
    // its own world pointer; without clearing it first, the next setWorld() call
    // dereferences freed memory in removeEventListener().
    if (client.worldRenderer != nullptr) {
        client.worldRenderer->setWorld(nullptr);
    }
    ownedPlayer_.reset();
    client.player = nullptr;
    parkedDimensionWorld_.reset();
    ownedWorld_.reset();
    ownedWorldStorage_.reset();
}

void WorldSession::setWorld(Minecraft& client, World* worldIn, const std::string& message, PlayerEntity* existingPlayer)
{
    if (client.stats != nullptr) {
        client.stats->syncStats();
        client.stats->save();
    }
    client.camera = nullptr;
    client.progressRenderer.progressStart(message);
    client.progressRenderer.progressStage("");
    client.audio.playRecord("", 0.0f, 0.0f, 0.0f, 0.0f);
    if (client.world != nullptr) {
        if (!client.world->isRemote() && client.player != nullptr) {
            if (WorldStorage* storage = client.world->getDimensionData()) {
                if (auto* alphaStorage = dynamic_cast<AlphaWorldStorage*>(storage)) {
                    alphaStorage->savePlayerData(*client.player);
                }
            }
        }
        client.world->savingProgress(nullptr);
    }
    client.world = worldIn;
    client.particleManager.setWorld(worldIn);
    if (worldIn != nullptr) {
        if (client.interactionManager != nullptr) {
            client.interactionManager->setWorld(worldIn);
        }
        if (!worldIn->isRemote()) {
            if (existingPlayer == nullptr && client.player == nullptr && client.interactionManager != nullptr) {
                ownedPlayer_ = std::unique_ptr<entity::player::ClientPlayerEntity>(
                    static_cast<entity::player::ClientPlayerEntity*>(client.interactionManager->createPlayer(worldIn)));
                client.player = ownedPlayer_.get();
            }
        } else if (client.player != nullptr) {
            client.player->teleportTop();
            worldIn->spawnEntity(client.player);
        }
        if (client.player == nullptr && client.interactionManager != nullptr) {
            ownedPlayer_ = std::unique_ptr<entity::player::ClientPlayerEntity>(
                static_cast<entity::player::ClientPlayerEntity*>(client.interactionManager->createPlayer(worldIn)));
            client.player = ownedPlayer_.get();
            client.player->teleportTop();
            client.interactionManager->preparePlayer(client.player);
        }
        if (client.player != nullptr) {
            if (!worldIn->isRemote() && existingPlayer == nullptr) {
                if (WorldStorage* storage = worldIn->getDimensionData()) {
                    if (auto* alphaStorage = dynamic_cast<AlphaWorldStorage*>(storage)) {
                        alphaStorage->loadPlayerData(*client.player);
                    }
                }
            }
            worldIn->setChunkCacheCenterFromBlockPos(
                MathHelper::floor(client.player->x),
                MathHelper::floor(client.player->z));
            client.camera = client.player;
        }
        if (client.worldRenderer != nullptr) {
            client.worldRenderer->setWorld(worldIn);
        }
        if (client.interactionManager != nullptr && client.player != nullptr) {
            client.interactionManager->preparePlayerRespawn(client.player);
        }
        if (existingPlayer != nullptr) {
            worldIn->saveLevelProperties();
        }
        if (!worldIn->isRemote()) {
            prepareWorld(client, message);
            if (client.player != nullptr) {
                worldIn->addPlayer(client.player);
            }
        } else if (client.player != nullptr) {
            worldIn->addPlayer(client.player);
        }
        client.options.applyToWorld(worldIn);
        if (worldIn->isNewWorld()) {
            worldIn->savingProgress(nullptr);
        }
    } else {
        clearWorld(client);
    }
    client.lastTickTime = 0;
}

void WorldSession::prepareWorld(Minecraft& client, const std::string& worldName)
{
    client.progressRenderer.progressStart(worldName);
    client.progressRenderer.progressStage("Building terrain");
    if (client.world == nullptr) {
        return;
    }
    constexpr int radius = 128;
    int progress = 0;
    const int progressTotal = ((radius * 2 / 16) + 1) * ((radius * 2 / 16) + 1);
    Vec3i center = client.world->getSpawnPos();
    if (client.player != nullptr) {
        center.x = MathHelper::floor(client.player->x);
        center.z = MathHelper::floor(client.player->z);
    }
    client.world->setChunkCacheCenterFromBlockPos(center.x, center.z);
    for (int dx = -radius; dx <= radius; dx += 16) {
        for (int dz = -radius; dz <= radius; dz += 16) {
            client.progressRenderer.progressStagePercentage(progress++ * 100 / progressTotal);
            (void)client.world->getBlockId(center.x + dx, 64, center.z + dz);
        }
    }
    client.world->populateChunkCacheReadyChunks();
    // Lighting runs on its own thread; wait for the initial flood to settle.
    client.world->finishLightingUpdates();
    client.progressRenderer.progressStage("Simulating world for a bit");
    client.world->tickChunks();
    if (client.world->isNewWorld()) {
        client.world->saveLevelProperties();
    }
}

void WorldSession::convertAndSaveWorld(Minecraft& client, const std::string& worldName, const std::string& name)
{
    if (client.worldStorageSource != nullptr) {
        client.progressRenderer.progressStart("Converting World to " + client.worldStorageSource->getName());
        client.progressRenderer.progressStage("This may take a while :)");
        client.worldStorageSource->convert(worldName, &client.progressRenderer);
    }
    client.startGame(worldName, name, 0);
}

void WorldSession::tickJoinPlayerCounter(Minecraft& client)
{
    if (client.world == nullptr || client.player == nullptr) {
        return;
    }
    // Drive render-distance chunk residency every tick (budgeted inside
    // loadChunksNearEntity). The old 30-tick gate starved the loader so chunks
    // only generated when the player physically walked into them.
    client.world->loadChunksNearEntity(client.player);
    ++joinPlayerCounter_;
}

} // namespace net::minecraft::client::core
