#include "net/minecraft/block/entity/BlockEntityInventoryHelpers.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block::entity::inventory_util {
bool canPlayerUseBlockEntity(const BlockEntity& self, PlayerEntity* player) {
  if(self.world == nullptr || player == nullptr) {
    return false;
  }
  if(self.world->getBlockEntity(self.x, self.y, self.z) != &self) {
    return false;
  }
  return player->getSquaredDistance(static_cast<double>(self.x) + 0.5,
                                    static_cast<double>(self.y) + 0.5,
                                    static_cast<double>(self.z) + 0.5) <= 64.0;
}
} // namespace net::minecraft::block::entity::inventory_util
