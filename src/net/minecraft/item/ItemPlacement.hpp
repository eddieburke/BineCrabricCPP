#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/BlockSounds.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item::detail {
inline void playerLookRay(const PlayerEntity* user, float partialTicks, Vec3d& start, Vec3d& end, double reach = 5.0) {
  const float pitch = user->prevPitch + (user->pitch - user->prevPitch) * partialTicks;
  const float yaw = user->prevYaw + (user->yaw - user->prevYaw) * partialTicks;
  const double x = user->prevX + (user->x - user->prevX) * static_cast<double>(partialTicks);
  const double y = user->prevY + (user->y - user->prevY) * static_cast<double>(partialTicks) + 1.62 -
                   static_cast<double>(user->standingEyeHeight);
  const double z = user->prevZ + (user->z - user->prevZ) * static_cast<double>(partialTicks);
  start = Vec3d{x, y, z};
  const float cosYaw = MathHelper::cos(-yaw * (kPiF / 180.0f) - kPiF);
  const float sinYaw = MathHelper::sin(-yaw * (kPiF / 180.0f) - kPiF);
  const float negCosPitch = -MathHelper::cos(-pitch * (kPiF / 180.0f));
  const float sinPitch = MathHelper::sin(-pitch * (kPiF / 180.0f));
  end = Vec3d{start.x + static_cast<double>(sinYaw * negCosPitch) * reach,
              start.y + static_cast<double>(sinPitch) * reach,
              start.z + static_cast<double>(cosYaw * negCosPitch) * reach};
}
inline void offsetPlacementPos(World* world, int& x, int& y, int& z, int& side) {
  if(world != nullptr && Block::SNOW != nullptr && world->getBlockId(x, y, z) == Block::SNOW->id) {
    side = 0;
    return;
  }
  if(side == 0) {
    --y;
  } else if(side == 1) {
    ++y;
  } else if(side == 2) {
    --z;
  } else if(side == 3) {
    ++z;
  } else if(side == 4) {
    --x;
  } else if(side == 5) {
    ++x;
  }
}
inline void playPlaceSound(World* world, Block* block, int x, int y, int z) {
  block::sounds::playPlace(world, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
                           static_cast<double>(z) + 0.5, block);
}
inline bool placeBlockItem(ItemStack* stack, PlayerEntity* user, World* world, int blockId, int metadata, int x, int y,
                           int z, int side, bool alwaysReturnTrue) {
  if(world == nullptr || stack == nullptr || blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
    return false;
  }
  offsetPlacementPos(world, x, y, z, side);
  if(stack->count == 0) {
    return false;
  }
  Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block == nullptr) {
    return false;
  }
  if(world->canPlace(blockId, x, y, z, false, side)) {
    if(world->setBlock(x, y, z, blockId, static_cast<std::uint8_t>(metadata))) {
      block->onPlaced(world, x, y, z, side);
      block->onPlaced(world, x, y, z, user);
      playPlaceSound(world, block, x, y, z);
      --stack->count;
    }
    return true;
  }
  return alwaysReturnTrue;
}
} // namespace net::minecraft::item::detail
