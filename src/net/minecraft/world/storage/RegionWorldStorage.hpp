#pragma once
#include <memory>
#include "net/minecraft/world/chunk/storage/RegionChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp" // inline getChunkStorage calls dimension->saveFolder()
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
namespace net::minecraft {
class RegionWorldStorage : public AlphaWorldStorage {
 public:
 RegionWorldStorage(fs::path savesDir, std::string worldName, bool createPlayerDataDir = false)
     : AlphaWorldStorage(std::move(savesDir), std::move(worldName), createPlayerDataDir) {
 }
 [[nodiscard]] const fs::path& directory() const noexcept {
  return dir_;
 }
 [[nodiscard]] std::unique_ptr<ChunkStorage> getChunkStorage(const Dimension* dimension) override {
  const std::string sub = dimension != nullptr ? dimension->saveFolder() : std::string();
  const fs::path chunkDir = sub.empty() ? dir_ : dir_ / sub;
  fs::create_directories(chunkDir);
  return std::make_unique<RegionChunkStorage>(chunkDir);
 }
 void save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) override {
  WorldProperties copy = properties;
  copy.setVersion(19132);
  AlphaWorldStorage::save(copy, players);
 }
 void saveUnload(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) {
  WorldProperties copy = properties;
  copy.setVersion(19132);
  AlphaWorldStorage::saveUnload(copy, players);
 }
 void save(const WorldProperties& properties) override {
  WorldProperties copy = properties;
  copy.setVersion(19132);
  AlphaWorldStorage::save(copy);
 }
 void forceSave() override {
  RegionIo::flush();
 }
};
} // namespace net::minecraft
