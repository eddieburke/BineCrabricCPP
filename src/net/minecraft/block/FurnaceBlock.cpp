#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FurnaceBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

using net::minecraft::entity::ItemEntity;

namespace {

constexpr int kFurnaceId = 61;
constexpr int kLitFurnaceId = 62;
bool ignoreBlockRemoval = false;


static ::net::minecraft::registry::RegisterBlock<FurnaceBlock> autoReg(61);
} // namespace

std::unique_ptr<entity::BlockEntity> FurnaceBlock::createBlockEntity()
{
    return std::make_unique<entity::FurnaceBlockEntity>();
}

void FurnaceBlock::updateDirection(World* world, int x, int y, int z)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int northId = world->getBlockId(x, y, z - 1);
    const int southId = world->getBlockId(x, y, z + 1);
    const int westId = world->getBlockId(x - 1, y, z);
    const int eastId = world->getBlockId(x + 1, y, z);
    int facing = 3;
    if (northId > 0 && northId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)]
        && (southId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)])) {
        facing = 3;
    } else if (southId > 0 && southId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)]
               && (northId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)])) {
        facing = 2;
    } else if (westId > 0 && westId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)]
               && (eastId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)])) {
        facing = 5;
    } else if (eastId > 0 && eastId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)]
               && (westId == 0 || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)])) {
        facing = 4;
    }
    world->setBlockMeta(x, y, z, facing);
}

void FurnaceBlock::onPlaced(World* world, int x, int y, int z)
{
    if (ignoreBlockRemoval) {
        Block::onPlaced(world, x, y, z);
        return;
    }
    BlockWithEntity::onPlaced(world, x, y, z);
    updateDirection(world, x, y, z);
}

void FurnaceBlock::onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer)
{
    BlockWithEntity::onPlaced(world, x, y, z);
    if (world == nullptr || placer == nullptr) {
        return;
    }
    const int direction = MathHelper::floor(static_cast<double>(placer->yaw * 4.0f / 360.0f) + 0.5) & 3;
    if (direction == 0) {
        world->setBlockMeta(x, y, z, 2);
    } else if (direction == 1) {
        world->setBlockMeta(x, y, z, 5);
    } else if (direction == 2) {
        world->setBlockMeta(x, y, z, 3);
    } else if (direction == 3) {
        world->setBlockMeta(x, y, z, 4);
    }
}

void FurnaceBlock::updateLitState(bool lit, World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    const std::uint8_t meta = world->getBlockMeta(x, y, z);
    entity::BlockEntity* blockEntity = world->getBlockEntity(x, y, z);
    ignoreBlockRemoval = true;
    world->setBlock(x, y, z, lit ? kLitFurnaceId : kFurnaceId, meta);
    ignoreBlockRemoval = false;
    world->setBlockMeta(x, y, z, meta);
    if (blockEntity != nullptr) {
        blockEntity->cancelRemoval();
        world->updateBlockEntity(x, y, z, blockEntity);
    }
}

void FurnaceBlock::onBreak(World* world, int x, int y, int z)
{
    if (ignoreBlockRemoval) {
        Block::onBreak(world, x, y, z);
        return;
    }
    if (world == nullptr) {
        BlockWithEntity::onBreak(world, x, y, z);
        return;
    }
    auto* furnace = dynamic_cast<entity::FurnaceBlockEntity*>(world->getBlockEntity(x, y, z));
    if (furnace != nullptr) {
        for (std::size_t slot = 0; slot < furnace->size(); ++slot) {
            ItemStack stack = furnace->getStack(slot);
            if (stack.empty()) {
                continue;
            }
            const float offsetX = random_.nextFloat() * 0.8f + 0.1f;
            const float offsetY = random_.nextFloat() * 0.8f + 0.1f;
            const float offsetZ = random_.nextFloat() * 0.8f + 0.1f;
            while (stack.count > 0) {
                int dropCount = random_.nextInt(21) + 10;
                if (dropCount > stack.count) {
                    dropCount = stack.count;
                }
                stack.count -= dropCount;
                auto* itemEntity = new ItemEntity(
                    world,
                    static_cast<double>(x) + static_cast<double>(offsetX),
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

bool FurnaceBlock::onUse(World* world, int x, int y, int z, PlayerEntity* player)
{
    if (world == nullptr || player == nullptr) {
        return true;
    }
    auto* furnace = dynamic_cast<entity::FurnaceBlockEntity*>(world->getBlockEntity(x, y, z));
    if (furnace == nullptr) {
        return true;
    }
    if (world->isRemote()) {
        return true;
    }
    player->openFurnaceScreen(furnace);
    return true;
}

void FurnaceBlock::randomDisplayTick(
    World* world, int x, int y, int z, JavaRandom& random)
{
    if (!lit || world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const float px = static_cast<float>(x) + 0.5f;
    const float py = static_cast<float>(y) + random.nextFloat() * 6.0f / 16.0f;
    const float pz = static_cast<float>(z) + 0.5f;
    constexpr float offset = 0.52f;
    const float spread = random.nextFloat() * 0.6f - 0.3f;
    if (meta == 4) {
        world->addParticle("smoke", px - offset, py, pz + spread, 0.0, 0.0, 0.0);
        world->addParticle("flame", px - offset, py, pz + spread, 0.0, 0.0, 0.0);
    } else if (meta == 5) {
        world->addParticle("smoke", px + offset, py, pz + spread, 0.0, 0.0, 0.0);
        world->addParticle("flame", px + offset, py, pz + spread, 0.0, 0.0, 0.0);
    } else if (meta == 2) {
        world->addParticle("smoke", px + spread, py, pz - offset, 0.0, 0.0, 0.0);
        world->addParticle("flame", px + spread, py, pz - offset, 0.0, 0.0, 0.0);
    } else if (meta == 3) {
        world->addParticle("smoke", px + spread, py, pz + offset, 0.0, 0.0, 0.0);
        world->addParticle("flame", px + spread, py, pz + offset, 0.0, 0.0, 0.0);
    }
}
namespace {

void FurnaceBlock::registerClass()
{
    Block::FURNACE = (new FurnaceBlock(61, false))->setHardness(3.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("furnace")->ignoreMetaUpdates();
    Block::LIT_FURNACE = (new FurnaceBlock(62, true))->setHardness(3.5f)->setSoundGroup(&vanillaStoneSound())->setLuminance(0.875f)->setTranslationKey("furnace")->ignoreMetaUpdates();
}



} // namespace
} // namespace net::minecraft::block

