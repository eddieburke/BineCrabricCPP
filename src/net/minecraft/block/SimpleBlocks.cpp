// Blocks simple enough to not warrant their own translation unit: plain
// constructor + setter chain, no recipes, no block items. Behavior subclasses
// (GravelBlock, OreBlock, etc.) still live in their own headers; only
// registration is centralized here. Each addBlock keeps the original
// registering class's priority id so bootstrap order matches the old
// one-TU-per-block scheme exactly (multi-block groups register together at
// their group's id, as before).
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/block/CactusBlock.hpp"
#include "net/minecraft/block/CakeBlock.hpp"
#include "net/minecraft/block/CobwebBlock.hpp"
#include "net/minecraft/block/CropBlock.hpp"
#include "net/minecraft/block/DeadBushBlock.hpp"
#include "net/minecraft/block/DirtBlock.hpp"
#include "net/minecraft/block/FarmlandBlock.hpp"
#include "net/minecraft/block/GravelBlock.hpp"
#include "net/minecraft/block/MushroomPlantBlock.hpp"
#include "net/minecraft/block/ObsidianBlock.hpp"
#include "net/minecraft/block/OreBlock.hpp"
#include "net/minecraft/block/OreStorageBlock.hpp"
#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/block/SandBlock.hpp"
#include "net/minecraft/block/SoulSandBlock.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"
#include "net/minecraft/block/SpongeBlock.hpp"
#include "net/minecraft/block/SugarCaneBlock.hpp"
#include "net/minecraft/block/TallPlantBlock.hpp"

namespace net::minecraft::block {
namespace {

namespace mat = material;

struct SimpleBlocksRegistrar {
    SimpleBlocksRegistrar()
    {
        using registry::Registry;
        Registry::addBlock(3, [] {
            Block::DIRT = (new DirtBlock(3, 2))
                ->setHardness(0.5f)
                ->setSoundGroup(&vanillaGravelSound())
                ->setTranslationKey("dirt");
        });
        Registry::addBlock(4, [] {
            Block::COBBLESTONE = (new Block(4, 16, mat::Material::STONE))
                ->setHardness(2.0f)
                ->setResistance(10.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("stonebrick");
        });
        Registry::addBlock(7, [] {
            Block::BEDROCK = (new Block(7, 17, mat::Material::STONE))
                ->setUnbreakable()
                ->setResistance(6000000.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("bedrock")
                ->disableTrackingStatistics();
        });
        Registry::addBlock(12, [] {
            Block::SAND = (new SandBlock(12, 18))
                ->setHardness(0.5f)
                ->setSoundGroup(&vanillaSandSound())
                ->setTranslationKey("sand");
        });
        Registry::addBlock(13, [] {
            Block::GRAVEL = (new GravelBlock(13, 19))
                ->setHardness(0.6f)
                ->setSoundGroup(&vanillaGravelSound())
                ->setTranslationKey("gravel");
        });
        Registry::addBlock(14, [] {
            Block::GOLD_ORE = (new OreBlock(14, 32))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("oreGold");
            Block::IRON_ORE = (new OreBlock(15, 33))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("oreIron");
            Block::COAL_ORE = (new OreBlock(16, 34))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("oreCoal");
            Block::LAPIS_ORE = (new OreBlock(21, 160))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("oreLapis");
            Block::DIAMOND_ORE = (new OreBlock(56, 50))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("oreDiamond");
        });
        Registry::addBlock(19, [] {
            Block::SPONGE = (new SpongeBlock(19))
                ->setHardness(0.6f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("sponge");
        });
        Registry::addBlock(22, [] {
            Block::LAPIS_BLOCK = (new Block(22, 144, mat::Material::STONE))
                ->setHardness(3.0f)
                ->setResistance(5.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("blockLapis");
        });
        Registry::addBlock(30, [] {
            Block::COBWEB = (new CobwebBlock(30, 11))
                ->setOpacity(1)
                ->setHardness(4.0f)
                ->setTranslationKey("web");
        });
        Registry::addBlock(31, [] {
            Block::GRASS = (new TallPlantBlock(31, 39))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("tallgrass");
        });
        Registry::addBlock(32, [] {
            Block::DEAD_BUSH = (new DeadBushBlock(32, 55))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("deadbush");
        });
        Registry::addBlock(37, [] {
            Block::DANDELION = (new PlantBlock(37, 13))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("flower");
            Block::ROSE = (new PlantBlock(38, 12))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("rose");
        });
        Registry::addBlock(39, [] {
            Block::BROWN_MUSHROOM = (new MushroomPlantBlock(39, 29))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setLuminance(0.125f)
                ->setTranslationKey("mushroom");
            Block::RED_MUSHROOM = (new MushroomPlantBlock(40, 28))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("mushroom");
        });
        Registry::addBlock(41, [] {
            Block::GOLD_BLOCK = (new OreStorageBlock(41, 23))
                ->setHardness(3.0f)
                ->setResistance(10.0f)
                ->setSoundGroup(&vanillaMetalSound())
                ->setTranslationKey("blockGold");
            Block::IRON_BLOCK = (new OreStorageBlock(42, 22))
                ->setHardness(5.0f)
                ->setResistance(10.0f)
                ->setSoundGroup(&vanillaMetalSound())
                ->setTranslationKey("blockIron");
            Block::DIAMOND_BLOCK = (new OreStorageBlock(57, 24))
                ->setHardness(5.0f)
                ->setResistance(10.0f)
                ->setSoundGroup(&vanillaMetalSound())
                ->setTranslationKey("blockDiamond");
        });
        Registry::addBlock(48, [] {
            Block::MOSSY_COBBLESTONE = (new Block(48, 36, mat::Material::STONE))
                ->setHardness(2.0f)
                ->setResistance(10.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("stoneMoss");
        });
        Registry::addBlock(49, [] {
            Block::OBSIDIAN = (new ObsidianBlock(49, 37))
                ->setHardness(10.0f)
                ->setResistance(2000.0f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("obsidian");
        });
        Registry::addBlock(52, [] {
            Block::SPAWNER = (new SpawnerBlock(52, 65))
                ->setHardness(5.0f)
                ->setSoundGroup(&vanillaMetalSound())
                ->setTranslationKey("mobSpawner")
                ->disableTrackingStatistics();
        });
        Registry::addBlock(59, [] {
            Block::WHEAT = (new CropBlock(59, 88))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("crops")
                ->disableTrackingStatistics()
                ->ignoreMetaUpdates();
        });
        Registry::addBlock(60, [] {
            Block::FARMLAND = (new FarmlandBlock(60))
                ->setHardness(0.6f)
                ->setSoundGroup(&vanillaGravelSound())
                ->setTranslationKey("farmland");
        });
        Registry::addBlock(81, [] {
            Block::CACTUS = (new CactusBlock(81, 70))
                ->setHardness(0.4f)
                ->setSoundGroup(&vanillaWoolSound())
                ->setTranslationKey("cactus");
        });
        Registry::addBlock(83, [] {
            Block::SUGAR_CANE = (new SugarCaneBlock(83, 73))
                ->setHardness(0.0f)
                ->setSoundGroup(&vanillaDirtSound())
                ->setTranslationKey("reeds")
                ->disableTrackingStatistics();
        });
        Registry::addBlock(87, [] {
            Block::NETHERRACK = (new Block(87, 103, mat::Material::STONE))
                ->setHardness(0.4f)
                ->setSoundGroup(&vanillaStoneSound())
                ->setTranslationKey("hellrock");
        });
        Registry::addBlock(88, [] {
            Block::SOUL_SAND = (new SoulSandBlock(88, 104))
                ->setHardness(0.5f)
                ->setSoundGroup(&vanillaSandSound())
                ->setTranslationKey("hellsand");
        });
        Registry::addBlock(92, [] {
            Block::CAKE = (new CakeBlock(92, 121))
                ->setHardness(0.5f)
                ->setSoundGroup(&vanillaWoolSound())
                ->setTranslationKey("cake")
                ->disableTrackingStatistics()
                ->ignoreMetaUpdates();
        });
    }
};

const SimpleBlocksRegistrar s_simpleBlocks;

} // namespace
} // namespace net::minecraft::block
