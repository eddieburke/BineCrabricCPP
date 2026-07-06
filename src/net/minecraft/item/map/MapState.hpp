#pragma once
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/world/PersistentState.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::map {
class MapState : public PersistentState {
public:
  struct MapIcon {
    std::uint8_t type = 0;
    std::uint8_t x = 0;
    std::uint8_t z = 0;
    std::uint8_t rotation = 0;
  };
  explicit MapState(std::string idIn) : PersistentState(std::move(idIn)) {}
  void readNbt(const NbtCompound& nbt) override;
  void writeNbt(NbtCompound& nbt) const override;
  void update(PlayerEntity* player, const ItemStack& stack);
  void markDirtyColumn(int column, int startZ, int endZ);
  [[nodiscard]] std::vector<std::uint8_t> getPlayerMarkerPacket(const ItemStack& stack, PlayerEntity* player);
  void readUpdateData(const std::vector<std::uint8_t>& updateData);
  int centerX = 0;
  int centerZ = 0;
  std::uint8_t dimension = 0;
  std::uint8_t scale = 0;
  std::array<std::uint8_t, 16384> colors{};
  int inventoryTicks = 0;
  std::vector<MapIcon> icons{};

private:
  struct PlayerUpdateTracker {
    explicit PlayerUpdateTracker(PlayerEntity* playerIn);
    PlayerEntity* player = nullptr;
    std::array<int, 128> startZ{};
    std::array<int, 128> endZ{};
    int nextDirtyPixel = 0;
    int colorsUpdateInterval = 0;
    std::vector<std::uint8_t> iconsData{};
    [[nodiscard]] std::vector<std::uint8_t> getUpdateData(MapState& mapState, const ItemStack& stack);
  };
  std::vector<std::unique_ptr<PlayerUpdateTracker>> updateTrackers_;
  std::unordered_map<PlayerEntity*, PlayerUpdateTracker*> updateTrackersByPlayer_;
};
} // namespace net::minecraft::map
