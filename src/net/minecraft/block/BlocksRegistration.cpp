#include "net/minecraft/world/biome/Biomes.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/LeavesBlockItem.hpp"
#include "net/minecraft/item/LogBlockItem.hpp"
#include "net/minecraft/item/PistonBlockItem.hpp"
#include "net/minecraft/item/SaplingBlockItem.hpp"
#include "net/minecraft/item/SlabBlockItem.hpp"
#include "net/minecraft/item/WoolBlockItem.hpp"
#include "net/minecraft/registry/VanillaRegistry.hpp"

namespace net::minecraft::block {

namespace {

void registerBiomes()
{
    Biomes::init();
}

MINECRAFT_REGISTER(registerBiomes, registry::kBiomeRegistrarPriority);

void finalizeBlocks()
{
    finalizeBlockRegistryProperties();
}

MINECRAFT_REGISTER(finalizeBlocks, registry::kFinalizeBlockRegistryPriority);

void registerSpecialBlockItems()
{
    static item::SaplingBlockItem SAPLING_ITEM(6 - 256);
    SAPLING_ITEM.setTranslationKey("sapling");

    static item::LogBlockItem LOG_ITEM(17 - 256);
    LOG_ITEM.setTranslationKey("log");

    static item::LeavesBlockItem LEAVES_ITEM(18 - 256);
    LEAVES_ITEM.setTranslationKey("leaves");

    static item::PistonBlockItem STICKY_PISTON_ITEM(29 - 256);

    static item::WoolBlockItem WOOL_ITEM(35 - 256);
    WOOL_ITEM.setTranslationKey("cloth");

    static item::PistonBlockItem PISTON_ITEM(33 - 256);

    static item::SlabBlockItem SLAB_ITEM(44 - 256);
    SLAB_ITEM.setTranslationKey("stoneSlab");
}

MINECRAFT_REGISTER(registerSpecialBlockItems, registry::blockItemRegistrarPriority(0));

void registerGenericBlockItems()
{
    for (int blockId = 0; blockId < 256; ++blockId) {
        if (Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr
            || Item::ITEMS[static_cast<std::size_t>(blockId)] != nullptr) {
            continue;
        }
        Item::ITEMS[static_cast<std::size_t>(blockId)] = new item::BlockItem(blockId - 256);
        Block::BLOCKS[static_cast<std::size_t>(blockId)]->init();
    }
}

MINECRAFT_REGISTER(registerGenericBlockItems, registry::kGenericBlockItemRegistrarPriority);

void bootstrapEntities()
{
    net::minecraft::entity::EntityRegistry::bootstrap();
}

MINECRAFT_REGISTER(bootstrapEntities, registry::kEntityBootstrapPriority);

} // namespace

} // namespace net::minecraft::block
