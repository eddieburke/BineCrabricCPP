#include "net/minecraft/world/biome/Biomes.hpp"

#include "net/minecraft/block/GlassSoundGroup.hpp"
#include "net/minecraft/block/PressurePlateActivationRule.hpp"
#include "net/minecraft/block/SandSoundGroup.hpp"
#include "net/minecraft/block/material/Material.hpp"

// All vanilla block subclasses (beta 1.7.3 MCP).
#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/BookshelfBlock.hpp"
#include "net/minecraft/block/ButtonBlock.hpp"
#include "net/minecraft/block/CactusBlock.hpp"
#include "net/minecraft/block/CakeBlock.hpp"
#include "net/minecraft/block/ChestBlock.hpp"
#include "net/minecraft/block/ClayBlock.hpp"
#include "net/minecraft/block/CobwebBlock.hpp"
#include "net/minecraft/block/CropBlock.hpp"
#include "net/minecraft/block/DeadBushBlock.hpp"
#include "net/minecraft/block/DetectorRailBlock.hpp"
#include "net/minecraft/block/DirtBlock.hpp"
#include "net/minecraft/block/DispenserBlock.hpp"
#include "net/minecraft/block/DoorBlock.hpp"
#include "net/minecraft/block/FarmlandBlock.hpp"
#include "net/minecraft/block/FenceBlock.hpp"
#include "net/minecraft/block/FireBlock.hpp"
#include "net/minecraft/block/FlowingLiquidBlock.hpp"
#include "net/minecraft/block/FurnaceBlock.hpp"
#include "net/minecraft/block/GlassBlock.hpp"
#include "net/minecraft/block/GlowstoneBlock.hpp"
#include "net/minecraft/block/GrassBlock.hpp"
#include "net/minecraft/block/GravelBlock.hpp"
#include "net/minecraft/block/IceBlock.hpp"
#include "net/minecraft/block/JukeboxBlock.hpp"
#include "net/minecraft/block/LadderBlock.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/block/LeverBlock.hpp"
#include "net/minecraft/block/LockedChestBlock.hpp"
#include "net/minecraft/block/LogBlock.hpp"
#include "net/minecraft/block/MushroomPlantBlock.hpp"
#include "net/minecraft/block/NetherPortalBlock.hpp"
#include "net/minecraft/block/NetherrackBlock.hpp"
#include "net/minecraft/block/NoteBlock.hpp"
#include "net/minecraft/block/ObsidianBlock.hpp"
#include "net/minecraft/block/OreBlock.hpp"
#include "net/minecraft/block/OreStorageBlock.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/PistonExtensionBlock.hpp"
#include "net/minecraft/block/PistonHeadBlock.hpp"
#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/block/PressurePlateBlock.hpp"
#include "net/minecraft/block/PumpkinBlock.hpp"
#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/block/RedstoneOreBlock.hpp"
#include "net/minecraft/block/RedstoneTorchBlock.hpp"
#include "net/minecraft/block/RedstoneWireBlock.hpp"
#include "net/minecraft/block/RepeaterBlock.hpp"
#include "net/minecraft/block/SandBlock.hpp"
#include "net/minecraft/block/SandstoneBlock.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"
#include "net/minecraft/block/SignBlock.hpp"
#include "net/minecraft/block/SlabBlock.hpp"
#include "net/minecraft/block/SnowBlock.hpp"
#include "net/minecraft/block/SnowyBlock.hpp"
#include "net/minecraft/block/SoulSandBlock.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"
#include "net/minecraft/block/SpongeBlock.hpp"
#include "net/minecraft/block/StairsBlock.hpp"
#include "net/minecraft/block/StillLiquidBlock.hpp"
#include "net/minecraft/block/StoneBlock.hpp"
#include "net/minecraft/block/SugarCaneBlock.hpp"
#include "net/minecraft/block/TallPlantBlock.hpp"
#include "net/minecraft/block/TntBlock.hpp"
#include "net/minecraft/block/TorchBlock.hpp"
#include "net/minecraft/block/TrapdoorBlock.hpp"
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/block/WorkbenchBlock.hpp"

#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

namespace {

using net::minecraft::BlockSoundGroup;
namespace mat = material;

BlockSoundGroup& defaultSound() { static BlockSoundGroup g("stone", 1.0f, 1.0f); return g; }
BlockSoundGroup& stoneSound()  { static BlockSoundGroup g("stone", 1.0f, 1.0f); return g; }
BlockSoundGroup& woodSound()   { static BlockSoundGroup g("wood", 1.0f, 1.0f); return g; }
BlockSoundGroup& metalSound()  { static BlockSoundGroup g("stone", 1.0f, 1.5f); return g; }
BlockSoundGroup& dirtSound()   { static BlockSoundGroup g("grass", 1.0f, 1.0f); return g; }
BlockSoundGroup& gravelSound(){ static BlockSoundGroup g("gravel", 1.0f, 1.0f); return g; }
BlockSoundGroup& sandSound()   { static SandSoundGroup g("sand", 1.0f, 1.0f); return g; }
BlockSoundGroup& woolSound()   { static BlockSoundGroup g("cloth", 1.0f, 1.0f); return g; }
GlassSoundGroup& glassSound()  { static GlassSoundGroup g("stone", 1.0f, 1.0f); return g; }

} // namespace

void registerVanillaBlocks()
{
    namespace mat = material;
(new StoneBlock(1, 1))->setHardness(1.5f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("stone");
(new GrassBlock(2))->setHardness(0.6f)->setSoundGroup(&dirtSound())->setTranslationKey("grass");
(new DirtBlock(3, 2))->setHardness(0.5f)->setSoundGroup(&gravelSound())->setTranslationKey("dirt");
(new Block(4, 16, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("stonebrick");
(new Block(5, 4, mat::Material::WOOD))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&woodSound())->setTranslationKey("wood")->ignoreMetaUpdates();
(new SaplingBlock(6, 15))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("sapling")->ignoreMetaUpdates();
(new Block(7, 17, mat::Material::STONE))->setUnbreakable()->setResistance(6000000.0f)->setSoundGroup(&stoneSound())->setTranslationKey("bedrock")->disableTrackingStatistics();
(new FlowingLiquidBlock(8, mat::Material::WATER))->setHardness(100.0f)->setOpacity(3)->setTranslationKey("water")->disableTrackingStatistics()->ignoreMetaUpdates();
(new StillLiquidBlock(9, mat::Material::WATER))->setHardness(100.0f)->setOpacity(3)->setTranslationKey("water")->disableTrackingStatistics()->ignoreMetaUpdates();
(new FlowingLiquidBlock(10, mat::Material::LAVA))->setHardness(0.0f)->setLuminance(1.0f)->setOpacity(255)->setTranslationKey("lava")->disableTrackingStatistics()->ignoreMetaUpdates();
(new StillLiquidBlock(11, mat::Material::LAVA))->setHardness(100.0f)->setLuminance(1.0f)->setOpacity(255)->setTranslationKey("lava")->disableTrackingStatistics()->ignoreMetaUpdates();
(new SandBlock(12, 18))->setHardness(0.5f)->setSoundGroup(&sandSound())->setTranslationKey("sand");
(new GravelBlock(13, 19))->setHardness(0.6f)->setSoundGroup(&gravelSound())->setTranslationKey("gravel");
(new OreBlock(14, 32))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreGold");
(new OreBlock(15, 33))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreIron");
(new OreBlock(16, 34))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreCoal");
(new LogBlock(17))->setHardness(2.0f)->setSoundGroup(&woodSound())->setTranslationKey("log")->ignoreMetaUpdates();
(new LeavesBlock(18, 52))->setHardness(0.2f)->setOpacity(1)->setSoundGroup(&dirtSound())->setTranslationKey("leaves")->disableTrackingStatistics()->ignoreMetaUpdates();
(new SpongeBlock(19))->setHardness(0.6f)->setSoundGroup(&dirtSound())->setTranslationKey("sponge");
(new GlassBlock(20, 49, mat::Material::GLASS, false))->setHardness(0.3f)->setSoundGroup(&glassSound())->setTranslationKey("glass");
(new OreBlock(21, 160))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreLapis");
(new Block(22, 144, mat::Material::STONE))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("blockLapis");
(new DispenserBlock(23))->setHardness(3.5f)->setSoundGroup(&stoneSound())->setTranslationKey("dispenser")->ignoreMetaUpdates();
(new SandstoneBlock(24))->setSoundGroup(&stoneSound())->setHardness(0.8f)->setTranslationKey("sandStone");
(new NoteBlock(25))->setHardness(0.8f)->setTranslationKey("musicBlock")->ignoreMetaUpdates();
(new BedBlock(26))->setHardness(0.2f)->setTranslationKey("bed")->disableTrackingStatistics()->ignoreMetaUpdates();
(new RailBlock(27, 179, true))->setHardness(0.7f)->setSoundGroup(&metalSound())->setTranslationKey("goldenRail")->ignoreMetaUpdates();
(new DetectorRailBlock(28, 195))->setHardness(0.7f)->setSoundGroup(&metalSound())->setTranslationKey("detectorRail")->ignoreMetaUpdates();
(new PistonBlock(29, 106, true))->setHardness(0.5f)->setSoundGroup(&stoneSound())->setTranslationKey("pistonStickyBase")->ignoreMetaUpdates();
(new CobwebBlock(30, 11))->setOpacity(1)->setHardness(4.0f)->setTranslationKey("web");
(new TallPlantBlock(31, 39))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("tallgrass");
(new DeadBushBlock(32, 55))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("deadbush");
(new PistonBlock(33, 107, false))->setHardness(0.5f)->setSoundGroup(&stoneSound())->setTranslationKey("pistonBase")->ignoreMetaUpdates();
(new PistonHeadBlock(34, 107))->ignoreMetaUpdates();
(new WoolBlock())->setHardness(0.8f)->setSoundGroup(&woolSound())->setTranslationKey("cloth")->ignoreMetaUpdates();
new PistonExtensionBlock(36);
(new PlantBlock(37, 13))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("flower");
(new PlantBlock(38, 12))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("rose");
(new MushroomPlantBlock(39, 29))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setLuminance(0.125f)->setTranslationKey("mushroom");
(new MushroomPlantBlock(40, 28))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("mushroom");
(new OreStorageBlock(41, 23))->setHardness(3.0f)->setResistance(10.0f)->setSoundGroup(&metalSound())->setTranslationKey("blockGold");
(new OreStorageBlock(42, 22))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&metalSound())->setTranslationKey("blockIron");
(new SlabBlock(43, true))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("stoneSlab");
(new SlabBlock(44, false))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("stoneSlab");
(new Block(45, 7, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("brick");
(new TntBlock(46, 8))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("tnt");
(new BookshelfBlock(47, 35))->setHardness(1.5f)->setSoundGroup(&woodSound())->setTranslationKey("bookshelf");
(new Block(48, 36, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("stoneMoss");
(new ObsidianBlock(49, 37))->setHardness(10.0f)->setResistance(2000.0f)->setSoundGroup(&stoneSound())->setTranslationKey("obsidian");
(new TorchBlock(50, 80))->setHardness(0.0f)->setLuminance(0.9375f)->setSoundGroup(&woodSound())->setTranslationKey("torch")->ignoreMetaUpdates();
(new FireBlock(51, 31))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&woodSound())->setTranslationKey("fire")->disableTrackingStatistics()->ignoreMetaUpdates();
(new SpawnerBlock(52, 65))->setHardness(5.0f)->setSoundGroup(&metalSound())->setTranslationKey("mobSpawner")->disableTrackingStatistics();
(new StairsBlock(53, *Block::BLOCKS[5]))->setTranslationKey("stairsWood")->ignoreMetaUpdates();
(new ChestBlock(54))->setHardness(2.5f)->setSoundGroup(&woodSound())->setTranslationKey("chest")->ignoreMetaUpdates();
(new RedstoneWireBlock(55, 164))->setHardness(0.0f)->setSoundGroup(&defaultSound())->setTranslationKey("redstoneDust")->disableTrackingStatistics()->ignoreMetaUpdates();
(new OreBlock(56, 50))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreDiamond");
(new OreStorageBlock(57, 24))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&metalSound())->setTranslationKey("blockDiamond");
(new WorkbenchBlock(58))->setHardness(2.5f)->setSoundGroup(&woodSound())->setTranslationKey("workbench");
(new CropBlock(59, 88))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("crops")->disableTrackingStatistics()->ignoreMetaUpdates();
(new FarmlandBlock(60))->setHardness(0.6f)->setSoundGroup(&gravelSound())->setTranslationKey("farmland");
(new FurnaceBlock(61, false))->setHardness(3.5f)->setSoundGroup(&stoneSound())->setTranslationKey("furnace")->ignoreMetaUpdates();
(new FurnaceBlock(62, true))->setHardness(3.5f)->setSoundGroup(&stoneSound())->setLuminance(0.875f)->setTranslationKey("furnace")->ignoreMetaUpdates();
(new SignBlock(63, true))->setHardness(1.0f)->setSoundGroup(&woodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
(new DoorBlock(64, mat::Material::WOOD))->setHardness(3.0f)->setSoundGroup(&woodSound())->setTranslationKey("doorWood")->disableTrackingStatistics()->ignoreMetaUpdates();
(new LadderBlock(65, 83))->setHardness(0.4f)->setSoundGroup(&woodSound())->setTranslationKey("ladder")->ignoreMetaUpdates();
(new RailBlock(66, 128, false))->setHardness(0.7f)->setSoundGroup(&metalSound())->setTranslationKey("rail")->ignoreMetaUpdates();
(new StairsBlock(67, *Block::BLOCKS[4]))->setTranslationKey("stairsStone")->ignoreMetaUpdates();
(new SignBlock(68, false))->setHardness(1.0f)->setSoundGroup(&woodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
(new LeverBlock(69, 96))->setHardness(0.5f)->setSoundGroup(&woodSound())->setTranslationKey("lever")->ignoreMetaUpdates();
(new PressurePlateBlock(70, Block::BLOCKS[1]->textureId, PressurePlateActivationRule::MOBS, mat::Material::STONE))->setHardness(0.5f)->setSoundGroup(&stoneSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
(new DoorBlock(71, mat::Material::METAL))->setHardness(5.0f)->setSoundGroup(&metalSound())->setTranslationKey("doorIron")->disableTrackingStatistics()->ignoreMetaUpdates();
(new PressurePlateBlock(72, Block::BLOCKS[5]->textureId, PressurePlateActivationRule::EVERYTHING, mat::Material::WOOD))->setHardness(0.5f)->setSoundGroup(&woodSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
(new RedstoneOreBlock(73, 51, false))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
(new RedstoneOreBlock(74, 51, true))->setLuminance(0.625f)->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&stoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
(new RedstoneTorchBlock(75, 115, false))->setHardness(0.0f)->setSoundGroup(&woodSound())->setTranslationKey("notGate")->ignoreMetaUpdates();
(new RedstoneTorchBlock(76, 99, true))->setHardness(0.0f)->setLuminance(0.5f)->setSoundGroup(&woodSound())->setTranslationKey("notGate")->ignoreMetaUpdates();
(new ButtonBlock(77, Block::BLOCKS[1]->textureId))->setHardness(0.5f)->setSoundGroup(&stoneSound())->setTranslationKey("button")->ignoreMetaUpdates();
(new SnowyBlock(78, 66))->setHardness(0.1f)->setSoundGroup(&woolSound())->setTranslationKey("snow");
(new IceBlock(79, 67))->setHardness(0.5f)->setOpacity(3)->setSoundGroup(&glassSound())->setTranslationKey("ice");
(new SnowBlock(80, 66))->setHardness(0.2f)->setSoundGroup(&woolSound())->setTranslationKey("snow");
(new CactusBlock(81, 70))->setHardness(0.4f)->setSoundGroup(&woolSound())->setTranslationKey("cactus");
(new ClayBlock(82, 72))->setHardness(0.6f)->setSoundGroup(&gravelSound())->setTranslationKey("clay");
(new SugarCaneBlock(83, 73))->setHardness(0.0f)->setSoundGroup(&dirtSound())->setTranslationKey("reeds")->disableTrackingStatistics();
(new JukeboxBlock(84, 74))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&stoneSound())->setTranslationKey("jukebox")->ignoreMetaUpdates();
(new FenceBlock(85, 4))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&woodSound())->setTranslationKey("fence")->ignoreMetaUpdates();
(new PumpkinBlock(86, 102, false))->setHardness(1.0f)->setSoundGroup(&woodSound())->setTranslationKey("pumpkin")->ignoreMetaUpdates();
(new NetherrackBlock(87, 103))->setHardness(0.4f)->setSoundGroup(&stoneSound())->setTranslationKey("hellrock");
(new SoulSandBlock(88, 104))->setHardness(0.5f)->setSoundGroup(&sandSound())->setTranslationKey("hellsand");
(new GlowstoneBlock(89, 105, mat::Material::STONE))->setHardness(0.3f)->setSoundGroup(&glassSound())->setLuminance(1.0f)->setTranslationKey("lightgem");
(new NetherPortalBlock(90, 14))->setHardness(-1.0f)->setSoundGroup(&glassSound())->setLuminance(0.75f)->setTranslationKey("portal");
(new PumpkinBlock(91, 102, true))->setHardness(1.0f)->setSoundGroup(&woodSound())->setLuminance(1.0f)->setTranslationKey("litpumpkin")->ignoreMetaUpdates();
(new CakeBlock(92, 121))->setHardness(0.5f)->setSoundGroup(&woolSound())->setTranslationKey("cake")->disableTrackingStatistics()->ignoreMetaUpdates();
(new RepeaterBlock(93, false))->setHardness(0.0f)->setSoundGroup(&woodSound())->setTranslationKey("diode")->disableTrackingStatistics()->ignoreMetaUpdates();
(new RepeaterBlock(94, true))->setHardness(0.0f)->setLuminance(0.625f)->setSoundGroup(&woodSound())->setTranslationKey("diode")->disableTrackingStatistics()->ignoreMetaUpdates();
(new LockedChestBlock(95))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&woodSound())->setTranslationKey("lockedchest")->setTickRandomly(true)->ignoreMetaUpdates();
(new TrapdoorBlock(96, mat::Material::WOOD))->setHardness(3.0f)->setSoundGroup(&woodSound())->setTranslationKey("trapdoor")->disableTrackingStatistics()->ignoreMetaUpdates();

    // Bind Java's named static block instances to their registered objects.
    Block::STONE = Block::BLOCKS[1];
    Block::GRASS_BLOCK = Block::BLOCKS[2];
    Block::DIRT = Block::BLOCKS[3];
    Block::COBBLESTONE = Block::BLOCKS[4];
    Block::PLANKS = Block::BLOCKS[5];
    Block::SAPLING = Block::BLOCKS[6];
    Block::BEDROCK = Block::BLOCKS[7];
    Block::FLOWING_WATER = Block::BLOCKS[8];
    Block::WATER = Block::BLOCKS[9];
    Block::FLOWING_LAVA = Block::BLOCKS[10];
    Block::LAVA = Block::BLOCKS[11];
    Block::SAND = Block::BLOCKS[12];
    Block::GRAVEL = Block::BLOCKS[13];
    Block::GOLD_ORE = Block::BLOCKS[14];
    Block::IRON_ORE = Block::BLOCKS[15];
    Block::COAL_ORE = Block::BLOCKS[16];
    Block::LOG = Block::BLOCKS[17];
    Block::LEAVES = Block::BLOCKS[18];
    Block::SPONGE = Block::BLOCKS[19];
    Block::GLASS = Block::BLOCKS[20];
    Block::LAPIS_ORE = Block::BLOCKS[21];
    Block::LAPIS_BLOCK = Block::BLOCKS[22];
    Block::DISPENSER = Block::BLOCKS[23];
    Block::SANDSTONE = Block::BLOCKS[24];
    Block::NOTE_BLOCK = Block::BLOCKS[25];
    Block::BED = Block::BLOCKS[26];
    Block::POWERED_RAIL = Block::BLOCKS[27];
    Block::DETECTOR_RAIL = Block::BLOCKS[28];
    Block::STICKY_PISTON = Block::BLOCKS[29];
    Block::COBWEB = Block::BLOCKS[30];
    Block::GRASS = Block::BLOCKS[31];
    Block::DEAD_BUSH = Block::BLOCKS[32];
    Block::PISTON = Block::BLOCKS[33];
    Block::PISTON_HEAD = Block::BLOCKS[34];
    Block::WOOL = Block::BLOCKS[35];
    Block::MOVING_PISTON = Block::BLOCKS[36];
    Block::DANDELION = Block::BLOCKS[37];
    Block::ROSE = Block::BLOCKS[38];
    Block::BROWN_MUSHROOM = Block::BLOCKS[39];
    Block::RED_MUSHROOM = Block::BLOCKS[40];
    Block::GOLD_BLOCK = Block::BLOCKS[41];
    Block::IRON_BLOCK = Block::BLOCKS[42];
    Block::DOUBLE_SLAB = Block::BLOCKS[43];
    Block::SLAB = Block::BLOCKS[44];
    Block::BRICKS = Block::BLOCKS[45];
    Block::TNT = Block::BLOCKS[46];
    Block::BOOKSHELF = Block::BLOCKS[47];
    Block::MOSSY_COBBLESTONE = Block::BLOCKS[48];
    Block::OBSIDIAN = Block::BLOCKS[49];
    Block::TORCH = Block::BLOCKS[50];
    Block::FIRE = Block::BLOCKS[51];
    Block::SPAWNER = Block::BLOCKS[52];
    Block::WOODEN_STAIRS = Block::BLOCKS[53];
    Block::CHEST = Block::BLOCKS[54];
    Block::REDSTONE_WIRE = Block::BLOCKS[55];
    Block::DIAMOND_ORE = Block::BLOCKS[56];
    Block::DIAMOND_BLOCK = Block::BLOCKS[57];
    Block::CRAFTING_TABLE = Block::BLOCKS[58];
    Block::WHEAT = Block::BLOCKS[59];
    Block::FARMLAND = Block::BLOCKS[60];
    Block::FURNACE = Block::BLOCKS[61];
    Block::LIT_FURNACE = Block::BLOCKS[62];
    Block::SIGN = Block::BLOCKS[63];
    Block::DOOR = Block::BLOCKS[64];
    Block::LADDER = Block::BLOCKS[65];
    Block::RAIL = Block::BLOCKS[66];
    Block::COBBLESTONE_STAIRS = Block::BLOCKS[67];
    Block::WALL_SIGN = Block::BLOCKS[68];
    Block::LEVER = Block::BLOCKS[69];
    Block::STONE_PRESSURE_PLATE = Block::BLOCKS[70];
    Block::IRON_DOOR = Block::BLOCKS[71];
    Block::WOODEN_PRESSURE_PLATE = Block::BLOCKS[72];
    Block::REDSTONE_ORE = Block::BLOCKS[73];
    Block::LIT_REDSTONE_ORE = Block::BLOCKS[74];
    Block::REDSTONE_TORCH = Block::BLOCKS[75];
    Block::LIT_REDSTONE_TORCH = Block::BLOCKS[76];
    Block::BUTTON = Block::BLOCKS[77];
    Block::SNOW = Block::BLOCKS[78];
    Block::ICE = Block::BLOCKS[79];
    Block::SNOW_BLOCK = Block::BLOCKS[80];
    Block::CACTUS = Block::BLOCKS[81];
    Block::CLAY = Block::BLOCKS[82];
    Block::SUGAR_CANE = Block::BLOCKS[83];
    Block::JUKEBOX = Block::BLOCKS[84];
    Block::FENCE = Block::BLOCKS[85];
    Block::PUMPKIN = Block::BLOCKS[86];
    Block::NETHERRACK = Block::BLOCKS[87];
    Block::SOUL_SAND = Block::BLOCKS[88];
    Block::GLOWSTONE = Block::BLOCKS[89];
    Block::NETHER_PORTAL = Block::BLOCKS[90];
    Block::JACK_O_LANTERN = Block::BLOCKS[91];
    Block::CAKE = Block::BLOCKS[92];
    Block::REPEATER = Block::BLOCKS[93];
    Block::POWERED_REPEATER = Block::BLOCKS[94];
    Block::LOCKED_CHEST = Block::BLOCKS[95];
    Block::TRAPDOOR = Block::BLOCKS[96];

    net::minecraft::Biomes::init();
}

void registerBlockItems()
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

} // namespace net::minecraft::block
