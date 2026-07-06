#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/DispenserBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/arrow.hpp"
#include "net/minecraft/item/EggItem.hpp"
#include "net/minecraft/item/SnowballItem.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
namespace net::minecraft::block {
namespace {
void spawnDispenserSmoke(World* world, int x, int y, int z, int offsetX, int offsetZ) {
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
}
} // namespace
using net::minecraft::entity::ItemEntity;
using net::minecraft::entity::projectile::ArrowEntity;
using net::minecraft::entity::projectile::setProjectileVelocity;
using net::minecraft::entity::projectile::thrown::EggEntity;
using net::minecraft::entity::projectile::thrown::SnowballEntity;
void DispenserBlock::onPlaced(World* world, int x, int y, int z) {
  BlockWithEntity::onPlaced(world, x, y, z);
  updateDirection(world, x, y, z);
}
void DispenserBlock::onPlaced(World* world, int x, int y, int z, PlayerEntity* placer) {
  BlockWithEntity::onPlaced(world, x, y, z);
  if(world == nullptr || placer == nullptr) {
    return;
  }
  const int facing = MathHelper::floor(static_cast<double>(placer->yaw * 4.0f / 360.0f) + 0.5) & 3;
  if(facing == 0) {
    world->setBlockMeta(x, y, z, 2);
  } else if(facing == 1) {
    world->setBlockMeta(x, y, z, 5);
  } else if(facing == 2) {
    world->setBlockMeta(x, y, z, 3);
  } else if(facing == 3) {
    world->setBlockMeta(x, y, z, 4);
  }
}
void DispenserBlock::updateDirection(World* world, int x, int y, int z) {
  if(world == nullptr || world->isRemote()) {
    return;
  }
  const int northId = world->getBlockId(x, y, z - 1);
  const int southId = world->getBlockId(x, y, z + 1);
  const int westId = world->getBlockId(x - 1, y, z);
  const int eastId = world->getBlockId(x + 1, y, z);
  int facing = 3;
  if(northId > 0 && northId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)] &&
     (southId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)])) {
    facing = 3;
  } else if(southId > 0 && southId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)] &&
            (northId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)])) {
    facing = 2;
  } else if(westId > 0 && westId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)] &&
            (eastId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)])) {
    facing = 5;
  } else if(eastId > 0 && eastId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)] &&
            (westId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)])) {
    facing = 4;
  }
  world->setBlockMeta(x, y, z, facing);
}
void DispenserBlock::dispense(World* world, int x, int y, int z, JavaRandom& randomIn) {
  if(world == nullptr) {
    return;
  }
  const int facing = world->getBlockMeta(x, y, z);
  int offsetX = 0;
  int offsetZ = 0;
  if(facing == 3) {
    offsetZ = 1;
  } else if(facing == 2) {
    offsetZ = -1;
  } else {
    offsetX = facing == 5 ? 1 : -1;
  }
  auto* dispenser = dynamic_cast<entity::DispenserBlockEntity*>(world->getBlockEntity(x, y, z));
  if(dispenser == nullptr) {
    return;
  }
  const double spawnX = static_cast<double>(x) + static_cast<double>(offsetX) * 0.6 + 0.5;
  const double spawnY = static_cast<double>(y) + 0.5;
  const double spawnZ = static_cast<double>(z) + static_cast<double>(offsetZ) * 0.6 + 0.5;
  ItemStack stack = dispenser->getItemToDispense();
  if(stack.empty()) {
    world->playSound(x, y, z, "random.click", 1.0f, 1.2f);
    return;
  }
  const int arrowId = Item::byRawId(6) != nullptr ? Item::byRawId(6)->id : 262;
  const int eggId = Item::byRawId(88) != nullptr ? Item::byRawId(88)->id : 344;
  const int snowballId = Item::byRawId(76) != nullptr ? Item::byRawId(76)->id : 332;
  if(stack.itemId == arrowId) {
    auto* arrow = new ArrowEntity(world, spawnX, spawnY, spawnZ);
    setProjectileVelocity(*arrow, offsetX, 0.1, offsetZ, 1.1f, 6.0f);
    world->spawnEntity(arrow);
    world->playSound(x, y, z, "random.bow", 1.0f, 1.2f);
  } else if(stack.itemId == eggId) {
    auto* egg = new EggEntity(world);
    egg->setPosition(spawnX, spawnY, spawnZ);
    setProjectileVelocity(*egg, offsetX, 0.1, offsetZ, 1.1f, 6.0f);
    world->spawnEntity(egg);
    world->playSound(x, y, z, "random.bow", 1.0f, 1.2f);
  } else if(stack.itemId == snowballId) {
    auto* snowball = new SnowballEntity(world);
    snowball->setPosition(spawnX, spawnY, spawnZ);
    setProjectileVelocity(*snowball, offsetX, 0.1, offsetZ, 1.1f, 6.0f);
    world->spawnEntity(snowball);
    world->playSound(x, y, z, "random.bow", 1.0f, 1.2f);
  } else {
    auto* itemEntity = new ItemEntity(world, spawnX, spawnY - 0.3, spawnZ, stack);
    const double impulse = randomIn.nextDouble() * 0.1 + 0.2;
    itemEntity->velocityX = static_cast<double>(offsetX) * impulse;
    itemEntity->velocityY = 0.2;
    itemEntity->velocityZ = static_cast<double>(offsetZ) * impulse;
    itemEntity->velocityX += randomIn.nextGaussian() * 0.0075 * 6.0;
    itemEntity->velocityY += randomIn.nextGaussian() * 0.0075 * 6.0;
    itemEntity->velocityZ += randomIn.nextGaussian() * 0.0075 * 6.0;
    world->spawnEntity(itemEntity);
    world->playSound(x, y, z, "random.click", 1.0f, 1.0f);
  }
  spawnDispenserSmoke(world, x, y, z, offsetX, offsetZ);
}
void DispenserBlock::neighborUpdate(World* world, int x, int y, int z, int id) {
  if(world == nullptr || id <= 0 || id >= Block::BLOCK_COUNT) {
    return;
  }
  Block* neighbor = Block::BLOCKS[static_cast<std::size_t>(id)];
  if(neighbor == nullptr || !neighbor->canEmitRedstonePower()) {
    return;
  }
  if(world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z)) {
    world->scheduleBlockUpdate(x, y, z, this->id, getTickRate());
  }
}
void DispenserBlock::onTick(World* world, int x, int y, int z, JavaRandom& randomIn) {
  if(world == nullptr) {
    return;
  }
  if(world->isEmittingRedstonePower(x, y, z) || world->isEmittingRedstonePower(x, y + 1, z)) {
    dispense(world, x, y, z, randomIn);
  }
}
bool DispenserBlock::onUse(World* world, int x, int y, int z, PlayerEntity* player) {
  if(world == nullptr || player == nullptr) {
    return true;
  }
  if(world->isRemote()) {
    return true;
  }
  auto* dispenser = dynamic_cast<entity::DispenserBlockEntity*>(world->getBlockEntity(x, y, z));
  if(dispenser != nullptr) {
    player->openChestScreen(dispenser);
  }
  return true;
}
void DispenserBlock::onBreak(World* world, int x, int y, int z) {
  if(world == nullptr) {
    BlockWithEntity::onBreak(world, x, y, z);
    return;
  }
  auto* dispenser = dynamic_cast<entity::DispenserBlockEntity*>(world->getBlockEntity(x, y, z));
  if(dispenser != nullptr) {
    for(std::size_t slot = 0; slot < dispenser->size(); ++slot) {
      ItemStack stack = dispenser->getStack(slot);
      if(stack.empty()) {
        continue;
      }
      const float offsetX = random_.nextFloat() * 0.8f + 0.1f;
      const float offsetY = random_.nextFloat() * 0.8f + 0.1f;
      const float offsetZ = random_.nextFloat() * 0.8f + 0.1f;
      while(stack.count > 0) {
        int dropCount = random_.nextInt(21) + 10;
        if(dropCount > stack.count) {
          dropCount = stack.count;
        }
        stack.count -= dropCount;
        auto* itemEntity = new ItemEntity(world, static_cast<double>(x) + static_cast<double>(offsetX),
                                          static_cast<double>(y) + static_cast<double>(offsetY),
                                          static_cast<double>(z) + static_cast<double>(offsetZ),
                                          ItemStack(stack.itemId, dropCount, stack.damage));
        constexpr float spread = 0.05f;
        itemEntity->velocityX = random_.nextGaussian() * spread;
        itemEntity->velocityY = random_.nextGaussian() * spread + 0.2f;
        itemEntity->velocityZ = random_.nextGaussian() * spread;
        world->spawnEntity(itemEntity);
      }
    }
  }
  BlockWithEntity::onBreak(world, x, y, z);
}
void DispenserBlock::registerClass() {
  Block::DISPENSER = (new DispenserBlock(kBlockId))
                         ->setHardness(3.5f)
                         ->setTranslationKey("dispenser")
                         ->ignoreMetaUpdates();
}
void DispenserBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::DISPENSER),
                                {std::string("###"), std::string("#X#"), std::string("#R#"), '#', Block::COBBLESTONE,
                                 'X', Item::byRawId(5), 'R', Item::byRawId(75)});
}
MC_REGISTER_BLOCK(DispenserBlock)
} // namespace net::minecraft::block
