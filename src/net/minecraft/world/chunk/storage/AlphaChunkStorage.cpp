#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include <cassert>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/NbtBinaryCodec.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
namespace net::minecraft {
AlphaChunkStorage::AlphaChunkStorage(fs::path dir, bool make) : dir_(std::move(dir)), make_(make) {
 if(make_) {
  fs::create_directories(dir_);
 }
}
namespace {
std::mutex& chunkFileMutex() {
 static std::mutex mutex;
 return mutex;
}
[[nodiscard]] std::string toBase36(int value) {
 if(value == 0) {
  return "0";
 }
 bool negative = value < 0;
 if(negative) {
  value = -value;
 }
 std::string out;
 while(value > 0) {
  const int digit = value % 36;
  out.push_back(static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10));
  value /= 36;
 }
 if(negative) {
  out.push_back('-');
 }
 std::reverse(out.begin(), out.end());
 return out;
}
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
void writeLevelFieldsFromSnapshot(std::vector<std::uint8_t>& out, const AlphaChunkStorage::ChunkSnapshot& snap) {
 appendNamedInt(out, "xPos", snap.x);
 appendNamedInt(out, "zPos", snap.z);
 appendNamedLong(out, "LastUpdate", static_cast<std::int64_t>(snap.lastUpdate));
 appendNamedByteArray(out, "Blocks", snap.blocks);
 appendNamedByteArray(out, "Data", snap.meta.bytes);
 appendNamedByteArray(out, "SkyLight", snap.skyLight.bytes);
 appendNamedByteArray(out, "BlockLight", snap.blockLight.bytes);
 appendNamedByteArray(out, "HeightMap", snap.heightmap.data(), snap.heightmap.size());
 appendNamedByte(out, "TerrainPopulated", snap.terrainPopulated ? 1 : 0);
 appendNamedListHeader(out,
                       "Entities",
                       snap.entities.size() == 0 ? Nbt::Type::Byte : Nbt::Type::Compound,
                       static_cast<std::int32_t>(snap.entities.size()));
 appendCompoundListElements(out, snap.entities);
 appendNamedListHeader(out,
                       "TileEntities",
                       snap.tileEntities.size() == 0 ? Nbt::Type::Byte : Nbt::Type::Compound,
                       static_cast<std::int32_t>(snap.tileEntities.size()));
 appendCompoundListElements(out, snap.tileEntities);
}
} // namespace
AlphaChunkStorage::ChunkSnapshot AlphaChunkStorage::takeSnapshot(Chunk& chunk, World* world) {
 if(world != nullptr) {
  world->checkSessionLock();
 }
 if(!chunk.meta.hasExpectedSizeForBlockCount(chunk.blocks.size())) {
  chunk.meta.ensureSizeForBlockCount(chunk.blocks.size());
 }
 ChunkSnapshot snap;
 snap.x = chunk.x;
 snap.z = chunk.z;
 snap.lastUpdate = static_cast<std::uint64_t>(world != nullptr ? world->getTime() : 0);
 snap.blocks = chunk.blocks;
 snap.meta = chunk.meta;
 snap.skyLight = chunk.skyLight;
 snap.blockLight = chunk.blockLight;
 snap.heightmap.assign(chunk.heightmap.begin(), chunk.heightmap.end());
 snap.terrainPopulated = chunk.terrainPopulated;
 chunk.lastSaveHadEntities = false;
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
   snap.entities.storage().asList().push_back(std::move(entityNbt.storage()));
  }
 }
 for(const auto& entry : chunk.blockEntities) {
  if(entry.second == nullptr) {
   continue;
  }
  NbtCompound tileNbt;
  entry.second->writeNbt(tileNbt);
  snap.tileEntities.storage().asList().push_back(std::move(tileNbt.storage()));
 }
 return snap;
}
void AlphaChunkStorage::writeRootChunkFromSnapshot(std::vector<std::uint8_t>& out, const ChunkSnapshot& snapshot) {
 out.clear();
 out.reserve(98304U);
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Compound));
 binary::appendU16BE(out, 0);
 binary::appendU8(out, static_cast<std::uint8_t>(Nbt::Type::Compound));
 binary::writeModifiedUtf8(out, "Level");
 writeLevelFieldsFromSnapshot(out, snapshot);
 binary::appendU8(out, 0);
 binary::appendU8(out, 0);
}
void AlphaChunkStorage::writeRootChunk(std::vector<std::uint8_t>& out, Chunk& chunk, World* world) {
 const ChunkSnapshot snapshot = takeSnapshot(chunk, world);
 writeRootChunkFromSnapshot(out, snapshot);
}
fs::path AlphaChunkStorage::getChunkFile(int chunkX, int chunkZ) const {
 const std::string fileName = "c." + toBase36(chunkX) + "." + toBase36(chunkZ) + ".dat";
 const fs::path subX = dir_ / toBase36(chunkX & 0x3F);
 if(!fs::exists(subX)) {
  if(!make_) {
   return {};
  }
  fs::create_directories(subX);
 }
 const fs::path subZ = subX / toBase36(chunkZ & 0x3F);
 if(!fs::exists(subZ)) {
  if(!make_) {
   return {};
  }
  fs::create_directories(subZ);
 }
 const fs::path file = subZ / fileName;
 if(!fs::exists(file) && !make_) {
  return {};
 }
 return file;
}
Chunk AlphaChunkStorage::loadChunk(World* world, int chunkX, int chunkZ) {
 const std::lock_guard lock(chunkFileMutex());
 const fs::path file = getChunkFile(chunkX, chunkZ);
 if(file.empty() || !fs::exists(file)) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
 try {
  std::ifstream input(file, std::ios::binary);
  if(!input) {
   return EmptyChunk(world, chunkX, chunkZ);
  }
  NbtCompound root = NbtIo::readCompressed(input);
  return loadChunkFromRootNbt(world, root, chunkX, chunkZ);
 } catch(const std::exception&) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
}
Chunk AlphaChunkStorage::loadChunkFromRootNbt(World* world, NbtCompound& root, int chunkX, int chunkZ) {
 if(!root.contains("Level")) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
 NbtCompound levelCompound = root.getCompound("Level");
 if(!levelCompound.contains("Blocks")) {
  return EmptyChunk(world, chunkX, chunkZ);
 }
 if(levelCompound.getInt("xPos") != chunkX || levelCompound.getInt("zPos") != chunkZ) {
  levelCompound.putInt("xPos", chunkX);
  levelCompound.putInt("zPos", chunkZ);
 }
 Chunk chunk = loadChunkFromNbt(world, levelCompound);
 chunk.fill();
 return chunk;
}
void AlphaChunkStorage::saveChunk(World* world, Chunk& chunk) {
 if(world == nullptr) {
  return;
 }
 const std::lock_guard lock(chunkFileMutex());
 world->checkSessionLock();
 const fs::path file = getChunkFile(chunk.x, chunk.z);
 if(file.empty()) {
  return;
 }
 WorldProperties& properties = world->getProperties();
 if(fs::exists(file)) {
  properties.setSizeOnDisk(properties.getSizeOnDisk() - static_cast<std::uint64_t>(fs::file_size(file)));
 }
 try {
  const fs::path temp = dir_ / "tmp_chunk.dat";
  std::vector<std::uint8_t> raw;
  writeRootChunk(raw, chunk, world);
  {
   std::ofstream output(temp, std::ios::binary | std::ios::trunc);
   if(!output) {
    throw std::runtime_error("Failed to open temporary chunk file");
   }
   NbtIo::writeCompressed(raw, output);
  }
  std::error_code ec;
  fs::remove(file, ec);
  ec.clear();
  fs::rename(temp, file, ec);
  ec.clear();
  fs::remove(temp, ec);
  if(fs::exists(file)) {
   properties.setSizeOnDisk(properties.getSizeOnDisk() + static_cast<std::uint64_t>(fs::file_size(file)));
  }
 } catch(const std::exception&) {
 }
}
Chunk AlphaChunkStorage::loadChunkFromNbt(World* world, NbtCompound& nbt) {
 assert(registry::Registry::isBootstrapped() && "AlphaChunkStorage: call Registry::bootstrap() before load");
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
