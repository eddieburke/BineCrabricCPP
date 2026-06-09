#include "net/minecraft/item/DyeItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

DyeItem::DyeItem(int rawId) : Item(rawId)
{
    setHasSubtypes(true);
    setMaxDamage(0);
}

int DyeItem::getTextureId(int damage) const
{
    return textureId_ + damage % 8 * 16 + damage / 8;
}

std::string DyeItem::getTranslationKey(const ItemStack* stack) const
{
    const int damage = stack != nullptr ? (stack->getDamage() & 0xF) : 0;
    return Item::getTranslationKey() + "." + names[static_cast<std::size_t>(damage)];
}

bool DyeItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int /*side*/)
{
    if (stack == nullptr || world == nullptr || stack->getDamage() != 15) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    if (Block::SAPLING != nullptr && blockId == Block::SAPLING->id) {
        if (!world->isRemote()) {
            if (auto* sapling = dynamic_cast<block::SaplingBlock*>(Block::SAPLING)) {
                sapling->generate(world, x, y, z, world->random());
            }
            --stack->count;
        }
        return true;
    }
    if (Block::WHEAT != nullptr && blockId == Block::WHEAT->id) {
        if (!world->isRemote()) {
            world->setBlockMeta(x, y, z, 7);
            --stack->count;
        }
        return true;
    }
    if (Block::GRASS_BLOCK != nullptr && blockId == Block::GRASS_BLOCK->id) {
        if (!world->isRemote()) {
            --stack->count;
            for (int i = 0; i < 128; ++i) {
                int px = x;
                int py = y + 1;
                int pz = z;
                bool blocked = false;
                for (int j = 0; j < i / 16; ++j) {
                    px += random.nextInt(3) - 1;
                    py += (random.nextInt(3) - 1) * random.nextInt(3) / 2;
                    pz += random.nextInt(3) - 1;
                    if (world->getBlockId(px, py - 1, pz) != Block::GRASS_BLOCK->id || world->shouldSuffocate(px, py, pz)) {
                        blocked = true;
                        break;
                    }
                }
                if (blocked || world->getBlockId(px, py, pz) != 0) {
                    continue;
                }
                if (random.nextInt(10) != 0 && Block::GRASS != nullptr) {
                    world->setBlock(px, py, pz, Block::GRASS->id, 1);
                } else if (random.nextInt(3) != 0 && Block::DANDELION != nullptr) {
                    world->setBlock(px, py, pz, Block::DANDELION->id);
                } else if (Block::ROSE != nullptr) {
                    world->setBlock(px, py, pz, Block::ROSE->id);
                }
            }
        }
        return true;
    }
    return false;
}

void DyeItem::useOnEntity(ItemStack* stack, LivingEntity* entity)
{
    auto* sheep = dynamic_cast<entity::passive::SheepEntity*>(entity);
    if (stack == nullptr || sheep == nullptr || sheep->isSheared()) {
        return;
    }
    const int color = block::WoolBlock::getItemMeta(stack->getDamage());
    if (sheep->getColor() != color) {
        sheep->setColor(color);
        --stack->count;
    }
}

namespace {

void registerDyeItem()
{
    static DyeItem DYE(95);
    DYE.setTexturePosition(14, 4)->setTranslationKey("dyePowder");
    Item::DYE = &DYE;
}

MINECRAFT_REGISTER_ITEM(registerDyeItem, 95);

} // namespace

} // namespace net::minecraft::item
