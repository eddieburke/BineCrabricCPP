#include "net/minecraft/world/chunk/storage/AlphaChunkNbtCodec.hpp"
#include <cassert>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/NbtBinaryCodec.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft {
namespace {
void appendNamedByte(std::vector<std::uint8_t>& out, const std::string& key, std::int8_t value) {
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Byte));
 binary::writeModifiedUtf8(out, key);
 binary::appendU8(out, static_cast<std::uint8_t>(value));
}
void appendNamedInt(std::vector<std::uint8_t>& out, const std::string& key, std::int32_t value) {
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Int));
 binary::writeModifiedUtf8(out, key);
 binary::appendI32BE(out, value);
}
void appendNamedLong(std::vector<std::uint8_t>& out, const std::string& key, std::int64_t value) {
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Long));
 binary::writeModifiedUtf8(out, key);
 binary::appendI64BE(out, value);
}
void appendNamedByteArray(std::vector<std::uint8_t>& out,
                          const std::string& key,
                          const std::uint8_t* data,
                          std::size_t size) {
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::ByteArray));
 binary::writeModifiedUtf8(out, key);
 binary::appendI32BE(out, static_cast<std::int32_t>(size));
 binary::appendBytes(out, data, size);
}
void appendNamedByteArray(std::vector<std::uint8_t>& out,
                          const std::string& key,
                          const std::vector<std::uint8_t>& bytes) {
 appendNamedByteArray(out, key, bytes.data(), bytes.size());
}
void appendNamedListHeader(std::vector<std::uint8_t>& out,
                           const std::string& key,
                           Nbt::Type itemType,
                           std::int32_t size) {
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::List));
 binary::writeModifiedUtf8(out, key);
 binary::appendU8(out, static_cast<std::uint8_t>(itemType));
 binary::appendI32BE(out, size);
}
void appendCompoundListElements(std::vector<std::uint8_t>& out, const NbtList& list) {
 for(const Nbt& entry : list.entries()) {
  nbt::detail::writePayload(out, entry);
 }
}
[[nodiscard]] std::vector<std::uint8_t>* takeByteArray(NbtCompound& nbt, const std::string& key) {
 Nbt* tag = nbt.storage().get(key);
 if(tag == nullptr || tag->type() != Nbt::Type::ByteArray) {
  return nullptr;
 }
 return &tag->asByteArray();
}
void writeLevelFields(std::vector<std::uint8_t>& out, Chunk& chunk, World* world) {
 if(world != nullptr) {
  world->checkSessionLock();
 }
 if(!chunk.meta.hasExpectedSizeForBlockCount(chunk.blocks.size())) {
  chunk.meta.ensureSizeForBlockCount(chunk.blocks.size());
 }
 appendNamedInt(out, "xPos", chunk.x);
 appendNamedInt(out, "zPos", chunk.z);
 appendNamedLong(out, "LastUpdate", static_cast<std::int64_t>(world != nullptr ? world->getTime() : 0));
 appendNamedByteArray(out, "Blocks", chunk.blocks);
 appendNamedByteArray(out, "Data", chunk.meta.bytes);
 appendNamedByteArray(out, "SkyLight", chunk.skyLight.bytes);
 appendNamedByteArray(out, "BlockLight", chunk.blockLight.bytes);
 appendNamedByteArray(out, "HeightMap", chunk.heightmap.data(), chunk.heightmap.size());
 appendNamedByte(out, "TerrainPopulated", chunk.terrainPopulated ? 1 : 0);
 chunk.lastSaveHadEntities = false;
 NbtList entities;
 for(const std::vector<Entity*>& slice : chunk.entities) {
  for(Entity* entity : slice) {
   if(entity == nullptr) {
    continue;
   }
   if(*reinterpret_cast<const void* const*>(entity) == nullptr) {
    continue;
   }
   NbtCompound entityNbt;
   if(!entity->saveSelfNbt(entityNbt)) {
    continue;
   }
   chunk.lastSaveHadEntities = true;
   entities.storage().asList().push_back(std::move(entityNbt.storage()));
  }
 }
 appendNamedListHeader(out,
                       "Entities",
                       entities.size() == 0 ? Nbt::Type::Byte : Nbt::Type::Compound,
                       static_cast<std::int32_t>(entities.size()));
 appendCompoundListElements(out, entities);
 NbtList tileEntities;
 for(const auto& entry : chunk.blockEntities) {
  if(entry.second == nullptr) {
   continue;
  }
  NbtCompound tileNbt;
  entry.second->writeNbt(tileNbt);
  tileEntities.storage().asList().push_back(std::move(tileNbt.storage()));
 }
 appendNamedListHeader(out,
                       "TileEntities",
                       tileEntities.size() == 0 ? Nbt::Type::Byte : Nbt::Type::Compound,
                       static_cast<std::int32_t>(tileEntities.size()));
 appendCompoundListElements(out, tileEntities);
}
} // namespace
void AlphaChunkNbtCodec::writeRootChunk(std::vector<std::uint8_t>& out, Chunk& chunk, World* world) {
 out.clear();
 out.reserve(98304U);
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Compound));
 binary::appendU16BE(out, 0);
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Compound));
 binary::writeModifiedUtf8(out, "Level");
 writeLevelFields(out, chunk, world);
 binary::appendU8(out, 0);
 binary::appendU8(out, 0);
}
Chunk AlphaChunkNbtCodec::loadChunkFromNbt(World* world, NbtCompound& nbt) {
 assert(registry::Registry::isBootstrapped() && "AlphaChunkNbtCodec: call initializeBlocks() before load");
 Chunk chunk(world, nbt.getInt("xPos"), nbt.getInt("zPos"));
 if(std::vector<std::uint8_t>* blockBytes = takeByteArray(nbt, "Blocks")) {
  if(blockBytes->size() >= chunk.blocks.size()) {
   chunk.blocks = std::move(*blockBytes);
  } else {
   std::copy_n(blockBytes->begin(), blockBytes->size(), chunk.blocks.begin());
  }
 }
 if(std::vector<std::uint8_t>* metaBytes = takeByteArray(nbt, "Data")) {
  chunk.meta = ChunkNibbleArray(std::move(*metaBytes));
 }
 const std::size_t expectedLightBytes = chunk.blocks.size() / 2U;
 bool skyLightInitialized = false;
 if(std::vector<std::uint8_t>* skyBytes = takeByteArray(nbt, "SkyLight")) {
  chunk.skyLight = ChunkNibbleArray(std::move(*skyBytes));
  skyLightInitialized = chunk.skyLight.isArrayInitialized() && chunk.skyLight.bytes.size() == expectedLightBytes;
 }
 if(std::vector<std::uint8_t>* blockLightBytes = takeByteArray(nbt, "BlockLight")) {
  chunk.blockLight = ChunkNibbleArray(std::move(*blockLightBytes));
 }
 const std::vector<std::uint8_t>* heightBytes = nullptr;
 if(std::vector<std::uint8_t>* heightMapBytes = takeByteArray(nbt, "HeightMap")) {
  heightBytes = heightMapBytes;
  if(!heightMapBytes->empty()) {
   std::copy_n(heightMapBytes->begin(),
               std::min(heightMapBytes->size(), chunk.heightmap.size()),
               chunk.heightmap.begin());
  }
 }
 chunk.terrainPopulated = nbt.getBoolean("TerrainPopulated");
 if(!chunk.meta.isArrayInitialized()) {
  chunk.meta = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
 } else if(!chunk.meta.hasExpectedSizeForBlockCount(chunk.blocks.size())) {
  chunk.meta.ensureSizeForBlockCount(chunk.blocks.size());
 }
 if(heightBytes == nullptr || heightBytes->empty() || !skyLightInitialized || chunk.skyLight.isAllZero()) {
  chunk.heightmap.fill(0);
  chunk.skyLight = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
  chunk.populateHeightMap();
 } else {
  chunk.populateHeightMapOnly();
 }
 if(!chunk.blockLight.isArrayInitialized() || chunk.blockLight.bytes.size() != expectedLightBytes) {
  chunk.blockLight = ChunkNibbleArray(static_cast<int>(chunk.blocks.size()));
  chunk.onLoad();
 }
 if(nbt.contains("Entities")) {
  const NbtList entities = nbt.getList("Entities");
  for(const Nbt& entry : entities.entries()) {
   if(!entry.isCompound()) {
    continue;
   }
   NbtCompound entityNbt = NbtCompound::bind(const_cast<Nbt&>(entry));
   if(std::unique_ptr<Entity> entity = EntityRegistry::getEntityFromNbt(entityNbt, world)) {
    chunk.lastSaveHadEntities = true;
    chunk.addEntity(entity.release());
   }
  }
 }
 if(nbt.contains("TileEntities")) {
  const NbtList tileEntities = nbt.getList("TileEntities");
  for(const Nbt& entry : tileEntities.entries()) {
   if(!entry.isCompound()) {
    continue;
   }
   NbtCompound tileNbt = NbtCompound::bind(const_cast<Nbt&>(entry));
   if(std::unique_ptr<block::entity::BlockEntity> blockEntity =
          block::entity::BlockEntity::createFromNbt(tileNbt)) {
    chunk.addBlockEntity(std::move(blockEntity));
   }
  }
 }
 chunk.dirty = false;
 return chunk;
}
} // namespace net::minecraft
