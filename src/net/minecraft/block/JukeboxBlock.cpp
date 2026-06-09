#include "net/minecraft/block/JukeboxBlock.hpp"

#include "net/minecraft/block/entity/JukeboxBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

JukeboxBlock::JukeboxBlock(int blockId, int textureId) : BlockWithEntity(blockId, textureId, material::Material::WOOD)
{
}

int JukeboxBlock::getTexture(int side) const
{
    return textureId + (side == 1 ? 1 : 0);
}

bool JukeboxBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/)
{
    if (world == nullptr || world->getBlockMeta(x, y, z) == 0) {
        return false;
    }
    tryEjectRecord(world, x, y, z);
    return true;
}

void JukeboxBlock::insertRecord(World* world, int x, int y, int z, int recordId)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    auto* jukebox = dynamic_cast<entity::JukeboxBlockEntity*>(world->getBlockEntity(x, y, z));
    if (jukebox == nullptr) {
        return;
    }
    jukebox->recordId = recordId;
    jukebox->markDirty();
    world->setBlockMeta(x, y, z, 1);
}

void JukeboxBlock::tryEjectRecord(World* world, int x, int y, int z)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    auto* jukebox = dynamic_cast<entity::JukeboxBlockEntity*>(world->getBlockEntity(x, y, z));
    if (jukebox == nullptr) {
        return;
    }
    const int recordId = jukebox->recordId;
    if (recordId == 0) {
        return;
    }
    world->worldEvent(1005, x, y, z, 0);
    world->playStreaming("", x, y, z);
    jukebox->recordId = 0;
    jukebox->markDirty();
    world->setBlockMeta(x, y, z, 0);
    constexpr float spread = 0.7f;
    const double offsetX =
        static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
    const double offsetY =
        static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.2 + 0.6;
    const double offsetZ =
        static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
    auto* itemEntity = new ItemEntity(
        world, static_cast<double>(x) + offsetX, static_cast<double>(y) + offsetY, static_cast<double>(z) + offsetZ,
        ItemStack(recordId, 1, 0));
    itemEntity->pickupDelay = 10;
    world->spawnEntity(itemEntity);
}

void JukeboxBlock::onBreak(World* world, int x, int y, int z)
{
    tryEjectRecord(world, x, y, z);
    BlockWithEntity::onBreak(world, x, y, z);
}

void JukeboxBlock::dropStacks(World* world, int x, int y, int z, int meta, float luck)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    BlockWithEntity::dropStacks(world, x, y, z, meta, luck);
}

} // namespace net::minecraft::block
