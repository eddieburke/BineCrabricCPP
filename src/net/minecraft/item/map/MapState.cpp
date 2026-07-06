#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/PersistentStateRegistry.hpp"
#include <algorithm>
#include <cstring>
namespace net::minecraft::map {
namespace {
constexpr int kMapSize = 128;
// MapItem loads existing map states through the type-erased getOrCreate path (never templating
// getOrCreateState), so register the factory eagerly rather than relying on first templated use.
[[maybe_unused]] const bool kMapStateRegistered = [] {
  PersistentStateRegistry::instance().registerType<MapState>();
  return true;
}();
} // namespace
MapState::PlayerUpdateTracker::PlayerUpdateTracker(PlayerEntity* playerIn) : player(playerIn) {
  startZ.fill(0);
  endZ.fill(127);
}
std::vector<std::uint8_t> MapState::PlayerUpdateTracker::getUpdateData(MapState& mapState, const ItemStack& stack) {
  (void)stack;
  if(--colorsUpdateInterval < 0) {
    colorsUpdateInterval = 4;
    std::vector<std::uint8_t> iconPacket(mapState.icons.size() * 3 + 1);
    iconPacket[0] = 1;
    for(std::size_t iconIndex = 0; iconIndex < mapState.icons.size(); ++iconIndex) {
      const MapIcon& icon = mapState.icons[iconIndex];
      iconPacket[iconIndex * 3 + 1] = static_cast<std::uint8_t>(icon.type + (icon.rotation & 0x0F) * 16);
      iconPacket[iconIndex * 3 + 2] = icon.x;
      iconPacket[iconIndex * 3 + 3] = icon.z;
    }
    bool unchanged = iconsData.size() == iconPacket.size();
    if(unchanged) {
      for(std::size_t i = 0; i < iconPacket.size(); ++i) {
        if(iconPacket[i] != iconsData[i]) {
          unchanged = false;
          break;
        }
      }
    }
    if(!unchanged) {
      iconsData = iconPacket;
      return iconPacket;
    }
  }
  for(int attempt = 0; attempt < 10; ++attempt) {
    const int column = nextDirtyPixel * 11 % kMapSize;
    ++nextDirtyPixel;
    if(startZ[static_cast<std::size_t>(column)] < 0) {
      continue;
    }
    const int rowCount = endZ[static_cast<std::size_t>(column)] - startZ[static_cast<std::size_t>(column)] + 1;
    const int rowStart = startZ[static_cast<std::size_t>(column)];
    std::vector<std::uint8_t> colorPacket(static_cast<std::size_t>(rowCount) + 3);
    colorPacket[0] = 0;
    colorPacket[1] = static_cast<std::uint8_t>(column);
    colorPacket[2] = static_cast<std::uint8_t>(rowStart);
    for(int row = 0; row < rowCount; ++row) {
      colorPacket[static_cast<std::size_t>(row) + 3] =
          mapState.colors[static_cast<std::size_t>((row + rowStart) * kMapSize + column)];
    }
    endZ[static_cast<std::size_t>(column)] = -1;
    startZ[static_cast<std::size_t>(column)] = -1;
    return colorPacket;
  }
  return {};
}
void MapState::readNbt(const NbtCompound& nbt) {
  dimension = static_cast<std::uint8_t>(nbt.getByte("dimension"));
  centerX = nbt.getInt("xCenter");
  centerZ = nbt.getInt("zCenter");
  const std::int8_t scaleByte = nbt.getByte("scale");
  if(scaleByte < 0) {
    scale = 0;
  } else if(scaleByte > 4) {
    scale = 4;
  } else {
    scale = static_cast<std::uint8_t>(scaleByte);
  }
  const int width = nbt.getShort("width");
  const int height = nbt.getShort("height");
  const std::vector<std::uint8_t> storedColors = nbt.getByteArray("colors");
  if(width == kMapSize && height == kMapSize) {
    if(storedColors.size() >= colors.size()) {
      std::copy_n(storedColors.begin(), colors.size(), colors.begin());
    }
  } else {
    colors.fill(0);
    const int offsetX = (kMapSize - width) / 2;
    const int offsetZ = (kMapSize - height) / 2;
    for(int z = 0; z < height; ++z) {
      const int targetZ = z + offsetZ;
      if(targetZ < 0 || targetZ >= kMapSize) {
        continue;
      }
      for(int x = 0; x < width; ++x) {
        const int targetX = x + offsetX;
        if(targetX < 0 || targetX >= kMapSize) {
          continue;
        }
        colors[static_cast<std::size_t>(targetX + targetZ * kMapSize)] =
            storedColors[static_cast<std::size_t>(x + z * width)];
      }
    }
  }
}
void MapState::writeNbt(NbtCompound& nbt) const {
  nbt.putByte("dimension", static_cast<std::int8_t>(dimension));
  nbt.putInt("xCenter", centerX);
  nbt.putInt("zCenter", centerZ);
  nbt.putByte("scale", static_cast<std::int8_t>(scale));
  nbt.putShort("width", static_cast<std::int16_t>(kMapSize));
  nbt.putShort("height", static_cast<std::int16_t>(kMapSize));
  nbt.putByteArray("colors", std::vector<std::uint8_t>(colors.begin(), colors.end()));
}
void MapState::update(PlayerEntity* player, const ItemStack& stack) {
  if(player == nullptr) {
    return;
  }
  if(updateTrackersByPlayer_.find(player) == updateTrackersByPlayer_.end()) {
    auto tracker = std::make_unique<PlayerUpdateTracker>(player);
    PlayerUpdateTracker* trackerPtr = tracker.get();
    updateTrackersByPlayer_[player] = trackerPtr;
    updateTrackers_.push_back(std::move(tracker));
  }
  icons.clear();
  for(auto it = updateTrackers_.begin(); it != updateTrackers_.end();) {
    PlayerUpdateTracker* tracker = it->get();
    if(tracker == nullptr || tracker->player == nullptr || tracker->player->dead ||
       !tracker->player->inventory.contains(stack)) {
      if(tracker != nullptr && tracker->player != nullptr) {
        updateTrackersByPlayer_.erase(tracker->player);
      }
      it = updateTrackers_.erase(it);
      continue;
    }
    const float relativeX =
        static_cast<float>(tracker->player->x - static_cast<double>(centerX)) / static_cast<float>(1 << scale);
    const float relativeZ =
        static_cast<float>(tracker->player->z - static_cast<double>(centerZ)) / static_cast<float>(1 << scale);
    constexpr int range = 64;
    if(relativeX < static_cast<float>(-range) || relativeZ < static_cast<float>(-range) ||
       relativeX > static_cast<float>(range) || relativeZ > static_cast<float>(range)) {
      ++it;
      continue;
    }
    std::uint8_t rotation = static_cast<std::uint8_t>(static_cast<double>(player->yaw) * 16.0 / 360.0 + 0.5);
    if(static_cast<std::int8_t>(dimension) < 0) {
      const int tickBucket = inventoryTicks / 10;
      rotation = static_cast<std::uint8_t>(((tickBucket * tickBucket * 34187121 + tickBucket * 121) >> 15) & 0xF);
    }
    if(tracker->player->dimensionId != static_cast<int>(dimension)) {
      ++it;
      continue;
    }
    icons.push_back(MapIcon{0, static_cast<std::uint8_t>(static_cast<double>(relativeX) * 2.0 + 0.5),
                            static_cast<std::uint8_t>(static_cast<double>(relativeZ) * 2.0 + 0.5), rotation});
    ++it;
  }
}
void MapState::markDirtyColumn(int column, int startZValue, int endZValue) {
  PersistentState::markDirty();
  for(const std::unique_ptr<PlayerUpdateTracker>& tracker : updateTrackers_) {
    if(tracker == nullptr) {
      continue;
    }
    if(tracker->startZ[static_cast<std::size_t>(column)] < 0 ||
       tracker->startZ[static_cast<std::size_t>(column)] > startZValue) {
      tracker->startZ[static_cast<std::size_t>(column)] = startZValue;
    }
    if(tracker->endZ[static_cast<std::size_t>(column)] < 0 ||
       tracker->endZ[static_cast<std::size_t>(column)] < endZValue) {
      tracker->endZ[static_cast<std::size_t>(column)] = endZValue;
    }
  }
}
std::vector<std::uint8_t> MapState::getPlayerMarkerPacket(const ItemStack& stack, PlayerEntity* player) {
  const auto it = updateTrackersByPlayer_.find(player);
  if(it == updateTrackersByPlayer_.end() || it->second == nullptr) {
    return {};
  }
  return it->second->getUpdateData(*this, stack);
}
void MapState::readUpdateData(const std::vector<std::uint8_t>& updateData) {
  if(updateData.empty()) {
    return;
  }
  if(updateData[0] == 0) {
    if(updateData.size() < 3) {
      return;
    }
    const int column = static_cast<int>(updateData[1] & 0xFF);
    const int startZ = static_cast<int>(updateData[2] & 0xFF);
    for(std::size_t i = 0; i + 3 < updateData.size(); ++i) {
      colors[(i + static_cast<std::size_t>(startZ)) * static_cast<std::size_t>(kMapSize) +
             static_cast<std::size_t>(column)] = updateData[i + 3];
    }
    markDirty();
  } else if(updateData[0] == 1) {
    icons.clear();
    for(std::size_t i = 0; i + 1 < updateData.size(); i += 3) {
      MapIcon icon;
      icon.type = static_cast<std::uint8_t>(updateData[i + 1] % 16);
      icon.x = updateData[i + 2];
      icon.z = updateData[i + 3];
      icon.rotation = static_cast<std::uint8_t>(updateData[i + 1] / 16);
      icons.push_back(icon);
    }
  }
}
} // namespace net::minecraft::map
