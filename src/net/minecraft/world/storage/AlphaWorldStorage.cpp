#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include <fstream>
#include <ostream>
#include <stdexcept>
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtFileIo.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/storage/exception/SessionLockException.hpp"
namespace net::minecraft {
namespace {
[[nodiscard]] std::uint64_t nowMillis() {
 using namespace std::chrono;
 return static_cast<std::uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}
} // namespace
AlphaWorldStorage::AlphaWorldStorage(fs::path savesDir, std::string name, bool createPlayerDataDir)
    : dir_(std::move(savesDir) / name),
      playerDataDir_(dir_ / "players"),
      dataDir_(dir_ / "data"),
      worldName_(std::move(name)),
      startTime_(nowMillis()) {
 fs::create_directories(dir_);
 fs::create_directories(dataDir_);
 if(createPlayerDataDir) {
  fs::create_directories(playerDataDir_);
 }
 writeSessionLock();
}
void AlphaWorldStorage::writeSessionLock() {
 const fs::path lockFile = dir_ / "session.lock";
 std::ofstream output(lockFile, std::ios::binary | std::ios::trunc);
 if(!output) {
  throw std::runtime_error("Failed to check session lock, aborting");
 }
 std::vector<std::uint8_t> bytes;
 binary::appendI64BE(bytes, static_cast<std::int64_t>(startTime_));
 binary::writeAllBytes(output, bytes);
}
void AlphaWorldStorage::refreshSessionLock() {
 startTime_ = nowMillis();
 writeSessionLock();
}
void AlphaWorldStorage::checkSessionLock() {
 const fs::path lockFile = dir_ / "session.lock";
 std::ifstream input(lockFile, std::ios::binary);
 if(!input) {
  throw SessionLockException("Failed to check session lock, aborting");
 }
 const std::vector<std::uint8_t> bytes = binary::readAllBytes(input);
 std::size_t pos = 0;
 const std::uint64_t stored = static_cast<std::uint64_t>(binary::readI64BE(bytes, pos));
 if(stored != startTime_) {
  throw SessionLockException("The save is being accessed from another location, aborting");
 }
}
std::unique_ptr<ChunkStorage> AlphaWorldStorage::getChunkStorage(const Dimension* dimension) {
 const std::string sub = dimension != nullptr ? dimension->saveFolder() : std::string();
 const fs::path chunkDir = sub.empty() ? dir_ : dir_ / sub;
 fs::create_directories(chunkDir);
 return std::make_unique<AlphaChunkStorage>(chunkDir, true);
}
std::optional<WorldProperties> AlphaWorldStorage::loadProperties() {
 if(const std::optional<WorldProperties> loaded = loadPropertiesFrom(dir_ / "level.dat"); loaded.has_value()) {
  return loaded;
 }
 return loadPropertiesFrom(dir_ / "level.dat_old");
}
std::optional<WorldProperties> AlphaWorldStorage::loadPropertiesFrom(const fs::path& file) {
 if(!fs::exists(file)) {
  return std::nullopt;
 }
 std::ifstream input(file, std::ios::binary);
 if(!input) {
  return std::nullopt;
 }
 try {
  const NbtCompound root = NbtIo::readCompressed(input);
  if(!root.contains("Data")) {
   return std::nullopt;
  }
  return WorldProperties(root.getCompound("Data"));
 } catch(...) {
  return std::nullopt;
 }
}
void AlphaWorldStorage::save(const WorldProperties& properties) {
 // level.dat is small and written at most once per autosave interval, so it always goes to
 // stable storage before the swap. The old saveUnload() overload turned fsync OFF for the
 // shutdown write -- the one write that most needs to land -- while leaving it on for the
 // periodic one.
 NbtCompound root;
 root.put("Data", properties.asNbt());
 AtomicWriteOptions options;
 options.keepBackup = true;
 try {
  writeFileAtomic(
      dir_ / "level.dat", [&root](std::ostream& output) { NbtIo::writeCompressed(root, output); }, options);
 } catch(const std::exception&) {
 }
}
void AlphaWorldStorage::savePlayerData(entity::player::PlayerEntity& player) {
 fs::create_directories(playerDataDir_);
 const fs::path file = playerDataDir_ / (player.name + ".dat");
 try {
  // The player is written as-is. This used to re-read the file it was about to overwrite and
  // splice the old inventory back in whenever the live one looked empty -- which resurrected
  // a stack every time a player legitimately emptied their inventory.
  NbtCompound nbt;
  player.writeNbt(nbt);
  AtomicWriteOptions options;
  options.keepBackup = true;
  writeFileAtomic(file, [&nbt](std::ostream& output) { NbtIo::writeCompressed(nbt, output); }, options);
 } catch(...) {
 }
}
void AlphaWorldStorage::loadPlayerData(entity::player::PlayerEntity& player) {
 if(const Nbt nbt = loadPlayerData(player.name); nbt.type() == Nbt::Type::Compound) {
  player.readNbt(NbtCompound(nbt));
 }
}
Nbt AlphaWorldStorage::loadPlayerData(const std::string& playerName) {
 // savePlayerData writes with keepBackup, so "<name>.dat_old" holds the previous copy across
 // the swap. Read it when the live file is missing or unreadable — same fallback level.dat
 // has always had. Without it a kill mid-swap left the loader with nothing, the player
 // spawned empty, and the next save overwrote the good backup with that empty inventory.
 for(const fs::path& file : {playerDataDir_ / (playerName + ".dat"), playerDataDir_ / (playerName + ".dat_old")}) {
  if(!fs::exists(file)) {
   continue;
  }
  try {
   std::ifstream input(file, std::ios::binary);
   if(!input) {
    continue;
   }
   return NbtIo::readCompressed(input).storage();
  } catch(...) {
  }
 }
 return {};
}
fs::path AlphaWorldStorage::getWorldPropertiesFile(const std::string& name) const {
 return dataDir_ / (name + ".dat");
}
} // namespace net::minecraft
