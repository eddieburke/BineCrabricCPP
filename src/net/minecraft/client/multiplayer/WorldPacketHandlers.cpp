// ClientNetworkHandler packet handlers for world state: time, chunk load/data/delta,
// block updates, explosions, signs, note blocks, weather/game-state, maps and world
// events (sounds/particles). Split out of ClientNetworkHandler.cpp for separation of
// concerns; see ClientNetworkHandlerInternal.hpp.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/BlockSounds.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/MusicDiscItem.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
namespace net::minecraft::client::multiplayer {
using namespace detail;
void ClientNetworkHandler::onWorldTimeUpdate(const WorldTimeUpdateS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  ClientWorld* clientWorld = dynamic_cast<ClientWorld*>(minecraft->world);
  if(clientWorld == nullptr) {
    minecraft->world->setTime(static_cast<std::uint64_t>(packet.time));
    return;
  }
  clientWorld->setTime(static_cast<std::uint64_t>(packet.time));
  clientWorld->updateSkyBrightness();
}
void ClientNetworkHandler::onChunkStatusUpdate(const ChunkStatusUpdateS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  clientWorld->updateChunk(packet.x, packet.z, packet.load);
}
void ClientNetworkHandler::onChunkDeltaUpdate(const ChunkDeltaUpdateS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr || world == nullptr) {
    return;
  }
  Chunk& chunk = world->getChunk(packet.x, packet.z);
  const int baseX = packet.x * 16;
  const int baseZ = packet.z * 16;
  for(int i = 0; i < packet.entryCount; ++i) {
    const std::int16_t position = packet.positions[static_cast<std::size_t>(i)];
    const int blockId = static_cast<int>(packet.blockRawIds[static_cast<std::size_t>(i)]) & 0xFF;
    const int meta = static_cast<int>(packet.blockMetadata[static_cast<std::size_t>(i)]);
    const int localX = (position >> 12) & 0xF;
    const int localZ = (position >> 8) & 0xF;
    const int localY = position & 0xFF;
    chunk.setBlock(localX, localY, localZ, blockId, meta);
    const int worldX = localX + baseX;
    const int worldZ = localZ + baseZ;
    clientWorld->clearBlockResets(worldX, localY, worldZ, worldX, localY, worldZ);
    clientWorld->setBlocksDirty(worldX, localY, worldZ, worldX, localY, worldZ);
  }
}
void ClientNetworkHandler::handleChunkData(const ChunkDataS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr || world == nullptr) {
    return;
  }
  clientWorld->clearBlockResets(packet.x, packet.y, packet.z, packet.x + packet.sizeX - 1,
                                packet.y + packet.sizeY - 1, packet.z + packet.sizeZ - 1);
  world->handleChunkDataUpdate(packet.x, packet.y, packet.z, packet.sizeX, packet.sizeY, packet.sizeZ,
                               packet.chunkData);
}
void ClientNetworkHandler::onBlockUpdate(const BlockUpdateS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  clientWorld->setBlockWithMetaFromPacket(packet.x, packet.y, packet.z, packet.blockRawId, packet.blockMetadata);
}
void ClientNetworkHandler::onExplosion(const ExplosionS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  Explosion explosion(minecraft->world, nullptr, packet.x, packet.y, packet.z, packet.radius);
  explosion.damagedBlocks.insert(packet.affectedBlocks.begin(), packet.affectedBlocks.end());
  explosion.playExplosionSound(true);
}
void ClientNetworkHandler::handleUpdateSign(const UpdateSignPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  if(!minecraft->world->isPosLoaded(packet.x, packet.y, packet.z)) {
    return;
  }
  auto* blockEntity = minecraft->world->getBlockEntity(packet.x, packet.y, packet.z);
  auto* sign = dynamic_cast<block::entity::SignBlockEntity*>(blockEntity);
  if(sign == nullptr) {
    return;
  }
  for(int i = 0; i < 4; ++i) {
    sign->texts[static_cast<std::size_t>(i)] = packet.text[static_cast<std::size_t>(i)];
  }
  sign->markDirty();
}
void ClientNetworkHandler::onPlayNoteSound(const PlayNoteSoundS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  minecraft->world->playNoteBlockActionAt(packet.x, packet.y, packet.z, packet.instrument, packet.pitch);
}
void ClientNetworkHandler::onGameStateChange(const GameStateChangeS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
    return;
  }
  const int reason = packet.reason;
  static const char* const kReasonMessages[] = {nullptr, nullptr, nullptr};
  if(reason >= 0 && reason < 3 && kReasonMessages[reason] != nullptr) {
    minecraft->player->sendMessage(kReasonMessages[reason]);
  }
  if(reason == 1) {
    world->getProperties().setRaining(true);
    world->setRainGradient(1.0f);
  } else if(reason == 2) {
    world->getProperties().setRaining(false);
    world->setRainGradient(0.0f);
  }
}
void ClientNetworkHandler::onMapUpdate(const MapUpdateS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr || Item::byRawId(102) == nullptr) {
    return;
  }
  if(packet.itemRawId == Item::byRawId(102)->id) {
    if(map::MapState* mapState =
           item::MapItem::getMapState(static_cast<std::int16_t>(packet.id), minecraft->world)) {
      mapState->readUpdateData(packet.updateData);
    }
  }
}
void ClientNetworkHandler::onWorldEvent(const WorldEventS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  World* world = minecraft->world;
  const int x = packet.x;
  const int y = packet.y;
  const int z = packet.z;
  const int data = packet.data;
  switch(packet.eventId) {
  case 1001:
    world->playSound(x, y, z, "random.click", 1.0f, 1.2f);
    break;
  case 1000:
    world->playSound(x, y, z, "random.click", 1.0f, 1.0f);
    break;
  case 1002:
    world->playSound(x, y, z, "random.bow", 1.0f, 1.2f);
    break;
  case 2001: {
    const int blockId = data & 0xFF;
    const int blockMeta = (data >> 8) & 0xFF;
    if(blockId > 0 && blockId < Block::BLOCK_COUNT &&
       Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
      Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
      block::sounds::playBreak(world, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
                               static_cast<double>(z) + 0.5, block);
    }
    world->spawnBlockBreakParticles(x, y, z, blockId, blockMeta);
    break;
  }
  case 1003: {
    JavaRandom& random = world->random();
    const char* sound = random.nextDouble() < 0.5 ? "random.door_open" : "random.door_close";
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
                     sound, 1.0f, random.nextFloat() * 0.1f + 0.9f);
    break;
  }
  case 1004: {
    JavaRandom& random = world->random();
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
                     "random.fizz", 0.5f, 2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f);
    break;
  }
  case 1005:
    if(data >= 0 && data < static_cast<int>(Item::ITEM_COUNT)) {
      Item* item = Item::ITEMS[static_cast<std::size_t>(data)];
      if(auto* disc = dynamic_cast<item::MusicDiscItem*>(item)) {
        world->playStreaming(disc->sound, x, y, z);
        break;
      }
    }
    world->playStreaming("", x, y, z);
    break;
  case 2000: {
    const int offsetX = data % 3 - 1;
    const int offsetZ = data / 3 % 3 - 1;
    JavaRandom& random = world->random();
    const double baseX = static_cast<double>(x) + static_cast<double>(offsetX) * 0.6 + 0.5;
    const double baseY = static_cast<double>(y) + 0.5;
    const double baseZ = static_cast<double>(z) + static_cast<double>(offsetZ) * 0.6 + 0.5;
    for(int i = 0; i < 10; ++i) {
      const double speed = random.nextDouble() * 0.2 + 0.01;
      const double px = baseX + static_cast<double>(offsetX) * 0.01 + (random.nextDouble() - 0.5) * offsetZ * 0.5;
      const double py = baseY + (random.nextDouble() - 0.5) * 0.5;
      const double pz = baseZ + static_cast<double>(offsetZ) * 0.01 + (random.nextDouble() - 0.5) * offsetX * 0.5;
      const double vx = static_cast<double>(offsetX) * speed + random.nextGaussian() * 0.01;
      const double vy = -0.03 + random.nextGaussian() * 0.01;
      const double vz = static_cast<double>(offsetZ) * speed + random.nextGaussian() * 0.01;
      world->addParticle("smoke", px, py, pz, vx, vy, vz);
    }
    break;
  }
  default:
    break;
  }
}
} // namespace net::minecraft::client::multiplayer
