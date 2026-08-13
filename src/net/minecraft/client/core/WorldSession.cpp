#include "net/minecraft/client/core/WorldSession.hpp"
#include <chrono>
#include <limits>
#include <thread>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/PlayerTextures.hpp"
#include "net/minecraft/client/sound/WorldSoundListener.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
namespace net::minecraft::client::core {
namespace {
void relightSkylightForPreparedArea(World& world, int centerX, int centerZ, int radiusBlocks) {
 for(int dx = -radiusBlocks; dx <= radiusBlocks; dx += 16) {
  for(int dz = -radiusBlocks; dz <= radiusBlocks; dz += 16) {
   if(Chunk* chunk = world.getChunkIfLoaded(centerX + dx, centerZ + dz);
      chunk != nullptr && !chunk->isEmpty()) {
    chunk->relightSkylightGaps();
   }
  }
 }
}
void saveLocalWorld(Minecraft& client, bool saveDedicatedPlayerData) {
 World* world = client.world;
 if(world == nullptr || world->isRemote()) {
  return;
 }
 WorldStorage* storage = world->getDimensionData();
 if(saveDedicatedPlayerData) {
  auto* alphaStorage = dynamic_cast<AlphaWorldStorage*>(storage);
  if(alphaStorage != nullptr && client.player != nullptr) {
   alphaStorage->savePlayerData(*client.player);
  }
 }
 world->save(true);
 if(ChunkSource* source = world->getChunkSource(); source != nullptr) {
  source->save(true);
 }
 if(storage != nullptr) {
  storage->forceSave();
 }
}
} // namespace
void WorldSession::clearWorld(Minecraft& client) {
 // Detach render listeners before ownedWorld_ is destroyed. WorldRenderer keeps
 // its own world pointer; without clearing it first, the next setWorld() call
 // dereferences freed memory in removeEventListener().
 if(client.worldSoundListener != nullptr && client.world != nullptr) {
  client.worldSoundListener->detach(client.world);
 }
 if(client.worldRenderer != nullptr) {
  client.worldRenderer->setWorld(nullptr);
 }
 ownedPlayer_.reset();
 client.player = nullptr;
 parkedDimensionWorld_.reset();
 ownedWorld_.reset();
 ownedWorldStorage_.reset();
}
void WorldSession::unloadWorld(Minecraft& client) {
 if(client.stats != nullptr) {
  client.stats->syncStats();
  client.stats->save();
 }
 saveLocalWorld(client, false);
 if(client.world != nullptr) {
  client.world->savingProgress(0);
  if(client.worldSoundListener != nullptr) {
   client.worldSoundListener->detach(client.world);
  }
 }
 client.camera = nullptr;
 client.world = nullptr;
 client.particleManager.setWorld(nullptr);
 if(client.interactionManager != nullptr) {
  client.interactionManager->setWorld(nullptr);
 }
 clearWorld(client);
 client.lastTickTime = 0;
}
void WorldSession::setWorld(Minecraft& client,
                            World* worldIn,
                            const std::string& message,
                            PlayerEntity* existingPlayer,
                            bool skipTerrainPrepare) {
 if(worldIn == nullptr) {
  unloadWorld(client);
  return;
 }
 if(client.stats != nullptr) {
  client.stats->syncStats();
  client.stats->save();
 }
 client.camera = nullptr;
 client.progressRenderer.progressStart(message);
 client.progressRenderer.progressStage("");
 client.audio.playRecord("", 0.0f, 0.0f, 0.0f, 0.0f);
 if(client.world != nullptr) {
  client.world->savingProgress(0);
 }
 if(client.worldSoundListener != nullptr && client.world != nullptr) {
  client.worldSoundListener->detach(client.world);
 }
 client.world = worldIn;
 client.particleManager.setWorld(worldIn);
 if(worldIn != nullptr) {
  if(client.interactionManager != nullptr) {
   client.interactionManager->setWorld(worldIn);
  }
  const bool hasSavedPlayer = !worldIn->isRemote() && worldIn->getProperties().getPlayerNbt() != nullptr;
  if(!worldIn->isRemote()) {
   if(existingPlayer == nullptr && client.player == nullptr && client.interactionManager != nullptr) {
    ownedPlayer_ = std::unique_ptr<entity::player::ClientPlayerEntity>(
        static_cast<entity::player::ClientPlayerEntity*>(client.interactionManager->createPlayer(worldIn)));
    client.player = ownedPlayer_.get();
   }
  } else if(client.player != nullptr) {
   client.player->teleportTop();
   worldIn->spawnEntity(client.player);
  }
  if(client.player == nullptr && client.interactionManager != nullptr) {
   ownedPlayer_ = std::unique_ptr<entity::player::ClientPlayerEntity>(
       static_cast<entity::player::ClientPlayerEntity*>(client.interactionManager->createPlayer(worldIn)));
   client.player = ownedPlayer_.get();
   if(!hasSavedPlayer) {
    client.player->teleportTop();
   }
   client.interactionManager->preparePlayer(client.player);
  } else if(!worldIn->isRemote() && existingPlayer == nullptr && client.player != nullptr && !hasSavedPlayer) {
   client.player->teleportTop();
   client.interactionManager->preparePlayer(client.player);
  }
  if(client.player != nullptr) {
   worldIn->setChunkCacheCenterFromBlockPos(MathHelper::floor(client.player->x),
                                            MathHelper::floor(client.player->z));
   client.camera = client.player;
  }
  // Sync ao from aoLevel before the first chunk remesh in setWorld/reload.
  client.options.applyDerivedSettings();
  if(client.worldRenderer != nullptr) {
   client.worldRenderer->setWorld(worldIn);
  }
  if(client.worldSoundListener != nullptr) {
   client.worldSoundListener->attach(worldIn);
  }
  if(client.interactionManager != nullptr && client.player != nullptr) {
   client.interactionManager->preparePlayerRespawn(client.player);
  }
  if(existingPlayer != nullptr) {
   worldIn->saveLevelProperties();
  }
  if(!worldIn->isRemote()) {
   if(!skipTerrainPrepare) {
    prepareWorld(client, message);
   }
   if(client.player != nullptr) {
    worldIn->addPlayer(client.player);
    client.player->nudgeOutOfCollision();
   }
  } else if(client.player != nullptr) {
   worldIn->addPlayer(client.player);
  }
  msauth::refreshPlayerTextures(client);
  client.options.applyToWorld(worldIn);
  if(worldIn->isNewWorld()) {
   worldIn->savingProgress(0);
  }
 }
 client.lastTickTime = 0;
}
void WorldSession::prepareWorld(Minecraft& client, const std::string& worldName) {
 if(client.world == nullptr || client.world->isRemote()) {
  return;
 }
 client.progressRenderer.progressStart(worldName);
 client.progressRenderer.progressStage("Building terrain");
 constexpr int radius = 48;
 Vec3i center = client.world->getSpawnPos();
 if(client.player != nullptr) {
  center.x = MathHelper::floor(client.player->x);
  center.z = MathHelper::floor(client.player->z);
 }
 client.world->setChunkCacheCenterFromBlockPos(center.x, center.z);
 constexpr int chunkRadius = radius / 16;
 int totalChunks = 0;
 for(int dx = -chunkRadius; dx <= chunkRadius; ++dx) {
  for(int dz = -chunkRadius; dz <= chunkRadius; ++dz) {
   if(dx * dx + dz * dz <= chunkRadius * chunkRadius) {
    ++totalChunks;
   }
  }
 }
 const int centerChunkX = center.x >> 4;
 const int centerChunkZ = center.z >> 4;
 if(ChunkSource* source = client.world->getChunkSource(); source != nullptr) {
  if(auto* cache = dynamic_cast<world::chunk::ChunkCache*>(source); cache != nullptr) {
   cache->setActiveRadius(chunkRadius);
   cache->prefetchChunksNear(centerChunkX, centerChunkZ);
   const auto start = std::chrono::steady_clock::now();
   while(!cache->isChunkDataReady(centerChunkX, centerChunkZ)) {
    cache->prefetchChunksNear(centerChunkX, centerChunkZ);
    cache->pumpChunkPublish();
    const std::size_t pending = cache->pendingAsyncLoadCount();
    const int done = totalChunks > static_cast<int>(pending) ? totalChunks - static_cast<int>(pending) : 0;
    client.progressRenderer.progressStagePercentage(done * 100 / totalChunks);
    if(pending == 0 || std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
     break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
   cache->pumpChunkPublish();
  } else {
   for(int dx = -chunkRadius; dx <= chunkRadius; ++dx) {
    for(int dz = -chunkRadius; dz <= chunkRadius; ++dz) {
     (void)client.world->getChunk(centerChunkX + dx, centerChunkZ + dz);
    }
   }
   source->pumpChunkPublish();
  }
 }
 relightSkylightForPreparedArea(*client.world, center.x, center.z, radius);
 client.world->finishLightingUpdates();
 client.progressRenderer.progressStage("Simulating world for a bit");
 client.world->tickChunks();
 if(client.world->isNewWorld()) {
  client.world->saveLevelProperties();
 }
}
void WorldSession::convertAndSaveWorld(Minecraft& client, const std::string& worldName, const std::string& name) {
 if(client.worldStorageSource != nullptr) {
  client.progressRenderer.progressStart("Converting World to " + client.worldStorageSource->getName());
  client.progressRenderer.progressStage("This may take a while :)");
  if(!client.worldStorageSource->convert(worldName, [&client](int pct) { client.progressRenderer.progressStagePercentage(pct); })) {
   client.progressRenderer.progressStage(
       "Alpha-format worlds cannot be converted yet; use a Region-format save or re-create the world.");
   return;
  }
 }
 client.startGame(worldName, name, 0, {}, true);
}
bool WorldSession::parkLocalWorldForRemoteHandoff(Minecraft& client) {
 if(client.world == nullptr || client.world->isRemote() || ownedWorld_ == nullptr) {
  return false;
 }
 if(client.stats != nullptr) {
  client.stats->syncStats();
  client.stats->save();
 }
 saveLocalWorld(client, true);
 if(client.worldSoundListener != nullptr) {
  client.worldSoundListener->detach(client.world);
 }
 if(client.worldRenderer != nullptr) {
  client.worldRenderer->setWorld(nullptr);
 }
 parkedHandoffWorldStorage_ = std::move(ownedWorldStorage_);
 parkedHandoffWorld_ = std::move(ownedWorld_);
 ownedPlayer_.reset();
 client.player = nullptr;
 client.camera = nullptr;
 client.world = nullptr;
 client.particleManager.setWorld(nullptr);
 if(client.interactionManager != nullptr) {
  client.interactionManager->setWorld(nullptr);
 }
 return true;
}
bool WorldSession::restoreParkedLocalWorld(Minecraft& client) {
 if(parkedHandoffWorld_ == nullptr) {
  return false;
 }
 ownedWorldStorage_ = std::move(parkedHandoffWorldStorage_);
 ownedWorld_ = std::move(parkedHandoffWorld_);
 if(WorldStorage* storage = ownedWorldStorage_.get()) {
  storage->refreshSessionLock();
 }
 setWorld(client, ownedWorld_.get(), "Loading level", nullptr, true);
 return true;
}
void WorldSession::commitRemoteHandoff() {
 parkedHandoffWorld_.reset();
 parkedHandoffWorldStorage_.reset();
}
void WorldSession::tickJoinPlayerCounter(Minecraft& client) {
 if(client.world == nullptr || client.player == nullptr) {
  return;
 }
 // Drive render-distance chunk residency every tick (budgeted inside
 // loadChunksNearEntity). The old 30-tick gate starved the loader so chunks
 // only generated when the player physically walked into them.
 client.world->loadChunksNearEntity(client.player);
}
} // namespace net::minecraft::client::core
