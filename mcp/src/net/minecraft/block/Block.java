/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.ArrayList;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.BedBlock;
import net.minecraft.block.BookshelfBlock;
import net.minecraft.block.ButtonBlock;
import net.minecraft.block.CactusBlock;
import net.minecraft.block.CakeBlock;
import net.minecraft.block.ChestBlock;
import net.minecraft.block.ClayBlock;
import net.minecraft.block.CobwebBlock;
import net.minecraft.block.CropBlock;
import net.minecraft.block.DeadBushBlock;
import net.minecraft.block.DetectorRailBlock;
import net.minecraft.block.DirtBlock;
import net.minecraft.block.DispenserBlock;
import net.minecraft.block.DoorBlock;
import net.minecraft.block.FarmlandBlock;
import net.minecraft.block.FenceBlock;
import net.minecraft.block.FireBlock;
import net.minecraft.block.FlowingLiquidBlock;
import net.minecraft.block.FurnaceBlock;
import net.minecraft.block.GlassBlock;
import net.minecraft.block.GlassSoundGroup;
import net.minecraft.block.GlowstoneBlock;
import net.minecraft.block.GrassBlock;
import net.minecraft.block.GravelBlock;
import net.minecraft.block.IceBlock;
import net.minecraft.block.JukeboxBlock;
import net.minecraft.block.LadderBlock;
import net.minecraft.block.LeavesBlock;
import net.minecraft.block.LeverBlock;
import net.minecraft.block.LockedChestBlock;
import net.minecraft.block.LogBlock;
import net.minecraft.block.MushroomPlantBlock;
import net.minecraft.block.NetherPortalBlock;
import net.minecraft.block.NetherrackBlock;
import net.minecraft.block.NoteBlock;
import net.minecraft.block.ObsidianBlock;
import net.minecraft.block.OreBlock;
import net.minecraft.block.OreStorageBlock;
import net.minecraft.block.PistonBlock;
import net.minecraft.block.PistonExtensionBlock;
import net.minecraft.block.PistonHeadBlock;
import net.minecraft.block.PlantBlock;
import net.minecraft.block.PressurePlateActivationRule;
import net.minecraft.block.PressurePlateBlock;
import net.minecraft.block.PumpkinBlock;
import net.minecraft.block.RailBlock;
import net.minecraft.block.RedstoneOreBlock;
import net.minecraft.block.RedstoneTorchBlock;
import net.minecraft.block.RedstoneWireBlock;
import net.minecraft.block.RepeaterBlock;
import net.minecraft.block.SandBlock;
import net.minecraft.block.SandSoundGroup;
import net.minecraft.block.SandstoneBlock;
import net.minecraft.block.SaplingBlock;
import net.minecraft.block.SignBlock;
import net.minecraft.block.SlabBlock;
import net.minecraft.block.SnowBlock;
import net.minecraft.block.SnowyBlock;
import net.minecraft.block.SoulSandBlock;
import net.minecraft.block.SpawnerBlock;
import net.minecraft.block.SpongeBlock;
import net.minecraft.block.StairsBlock;
import net.minecraft.block.StillLiquidBlock;
import net.minecraft.block.StoneBlock;
import net.minecraft.block.SugarCaneBlock;
import net.minecraft.block.TallPlantBlock;
import net.minecraft.block.TntBlock;
import net.minecraft.block.TorchBlock;
import net.minecraft.block.TrapdoorBlock;
import net.minecraft.block.WoolBlock;
import net.minecraft.block.WorkbenchBlock;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.BlockItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.LeavesBlockItem;
import net.minecraft.item.LogBlockItem;
import net.minecraft.item.PistonBlockItem;
import net.minecraft.item.SaplingBlockItem;
import net.minecraft.item.SlabBlockItem;
import net.minecraft.item.WoolBlockItem;
import net.minecraft.sound.BlockSoundGroup;
import net.minecraft.stat.Stats;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class Block {
    public static final BlockSoundGroup DEFAULT_SOUND_GROUP = new BlockSoundGroup("stone", 1.0f, 1.0f);
    public static final BlockSoundGroup WOOD_SOUND_GROUP = new BlockSoundGroup("wood", 1.0f, 1.0f);
    public static final BlockSoundGroup GRAVEL_SOUND_GROUP = new BlockSoundGroup("gravel", 1.0f, 1.0f);
    public static final BlockSoundGroup DIRT_SOUND_GROUP = new BlockSoundGroup("grass", 1.0f, 1.0f);
    public static final BlockSoundGroup STONE_SOUND_GROUP = new BlockSoundGroup("stone", 1.0f, 1.0f);
    public static final BlockSoundGroup METAL_SOUND_GROUP = new BlockSoundGroup("stone", 1.0f, 1.5f);
    public static final BlockSoundGroup GLASS_SOUND_GROUP = new GlassSoundGroup("stone", 1.0f, 1.0f);
    public static final BlockSoundGroup WOOL_SOUND_GROUP = new BlockSoundGroup("cloth", 1.0f, 1.0f);
    public static final BlockSoundGroup SAND_SOUND_GROUP = new SandSoundGroup("sand", 1.0f, 1.0f);
    public static final Block[] BLOCKS = new Block[256];
    public static final boolean[] BLOCKS_RANDOM_TICK = new boolean[256];
    public static final boolean[] BLOCKS_OPAQUE = new boolean[256];
    public static final boolean[] BLOCKS_WITH_ENTITY = new boolean[256];
    public static final int[] BLOCKS_LIGHT_OPACITY = new int[256];
    public static final boolean[] BLOCKS_ALLOW_VISION = new boolean[256];
    public static final int[] BLOCKS_LIGHT_LUMINANCE = new int[256];
    public static final boolean[] BLOCKS_IGNORE_META_UPDATE = new boolean[256];
    public static final Block STONE = new StoneBlock(1, 1).setHardness(1.5f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("stone");
    public static final GrassBlock GRASS_BLOCK = (GrassBlock)new GrassBlock(2).setHardness(0.6f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("grass");
    public static final Block DIRT = new DirtBlock(3, 2).setHardness(0.5f).setSoundGroup(GRAVEL_SOUND_GROUP).setTranslationKey("dirt");
    public static final Block COBBLESTONE = new Block(4, 16, Material.STONE).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("stonebrick");
    public static final Block PLANKS = new Block(5, 4, Material.WOOD).setHardness(2.0f).setResistance(5.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("wood").ignoreMetaUpdates();
    public static final Block SAPLING = new SaplingBlock(6, 15).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("sapling").ignoreMetaUpdates();
    public static final Block BEDROCK = new Block(7, 17, Material.STONE).setUnbreakable().setResistance(6000000.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("bedrock").disableTrackingStatistics();
    public static final Block FLOWING_WATER = new FlowingLiquidBlock(8, Material.WATER).setHardness(100.0f).setOpacity(3).setTranslationKey("water").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block WATER = new StillLiquidBlock(9, Material.WATER).setHardness(100.0f).setOpacity(3).setTranslationKey("water").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block FLOWING_LAVA = new FlowingLiquidBlock(10, Material.LAVA).setHardness(0.0f).setLuminance(1.0f).setOpacity(255).setTranslationKey("lava").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block LAVA = new StillLiquidBlock(11, Material.LAVA).setHardness(100.0f).setLuminance(1.0f).setOpacity(255).setTranslationKey("lava").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block SAND = new SandBlock(12, 18).setHardness(0.5f).setSoundGroup(SAND_SOUND_GROUP).setTranslationKey("sand");
    public static final Block GRAVEL = new GravelBlock(13, 19).setHardness(0.6f).setSoundGroup(GRAVEL_SOUND_GROUP).setTranslationKey("gravel");
    public static final Block GOLD_ORE = new OreBlock(14, 32).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreGold");
    public static final Block IRON_ORE = new OreBlock(15, 33).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreIron");
    public static final Block COAL_ORE = new OreBlock(16, 34).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreCoal");
    public static final Block LOG = new LogBlock(17).setHardness(2.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("log").ignoreMetaUpdates();
    public static final LeavesBlock LEAVES = (LeavesBlock)new LeavesBlock(18, 52).setHardness(0.2f).setOpacity(1).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("leaves").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block SPONGE = new SpongeBlock(19).setHardness(0.6f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("sponge");
    public static final Block GLASS = new GlassBlock(20, 49, Material.GLASS, false).setHardness(0.3f).setSoundGroup(GLASS_SOUND_GROUP).setTranslationKey("glass");
    public static final Block LAPIS_ORE = new OreBlock(21, 160).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreLapis");
    public static final Block LAPIS_BLOCK = new Block(22, 144, Material.STONE).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("blockLapis");
    public static final Block DISPENSER = new DispenserBlock(23).setHardness(3.5f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("dispenser").ignoreMetaUpdates();
    public static final Block SANDSTONE = new SandstoneBlock(24).setSoundGroup(STONE_SOUND_GROUP).setHardness(0.8f).setTranslationKey("sandStone");
    public static final Block NOTE_BLOCK = new NoteBlock(25).setHardness(0.8f).setTranslationKey("musicBlock").ignoreMetaUpdates();
    public static final Block BED = new BedBlock(26).setHardness(0.2f).setTranslationKey("bed").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block POWERED_RAIL = new RailBlock(27, 179, true).setHardness(0.7f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("goldenRail").ignoreMetaUpdates();
    public static final Block DETECTOR_RAIL = new DetectorRailBlock(28, 195).setHardness(0.7f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("detectorRail").ignoreMetaUpdates();
    public static final Block STICKY_PISTON = new PistonBlock(29, 106, true).setTranslationKey("pistonStickyBase").ignoreMetaUpdates();
    public static final Block COBWEB = new CobwebBlock(30, 11).setOpacity(1).setHardness(4.0f).setTranslationKey("web");
    public static final TallPlantBlock GRASS = (TallPlantBlock)new TallPlantBlock(31, 39).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("tallgrass");
    public static final DeadBushBlock DEAD_BUSH = (DeadBushBlock)new DeadBushBlock(32, 55).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("deadbush");
    public static final Block PISTON = new PistonBlock(33, 107, false).setTranslationKey("pistonBase").ignoreMetaUpdates();
    public static final PistonHeadBlock PISTON_HEAD = (PistonHeadBlock)new PistonHeadBlock(34, 107).ignoreMetaUpdates();
    public static final Block WOOL = new WoolBlock().setHardness(0.8f).setSoundGroup(WOOL_SOUND_GROUP).setTranslationKey("cloth").ignoreMetaUpdates();
    public static final PistonExtensionBlock MOVING_PISTON = new PistonExtensionBlock(36);
    public static final PlantBlock DANDELION = (PlantBlock)new PlantBlock(37, 13).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("flower");
    public static final PlantBlock ROSE = (PlantBlock)new PlantBlock(38, 12).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("rose");
    public static final PlantBlock BROWN_MUSHROOM = (PlantBlock)new MushroomPlantBlock(39, 29).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setLuminance(0.125f).setTranslationKey("mushroom");
    public static final PlantBlock RED_MUSHROOM = (PlantBlock)new MushroomPlantBlock(40, 28).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("mushroom");
    public static final Block GOLD_BLOCK = new OreStorageBlock(41, 23).setHardness(3.0f).setResistance(10.0f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("blockGold");
    public static final Block IRON_BLOCK = new OreStorageBlock(42, 22).setHardness(5.0f).setResistance(10.0f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("blockIron");
    public static final Block DOUBLE_SLAB = new SlabBlock(43, true).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("stoneSlab");
    public static final Block SLAB = new SlabBlock(44, false).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("stoneSlab");
    public static final Block BRICKS = new Block(45, 7, Material.STONE).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("brick");
    public static final Block TNT = new TntBlock(46, 8).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("tnt");
    public static final Block BOOKSHELF = new BookshelfBlock(47, 35).setHardness(1.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("bookshelf");
    public static final Block MOSSY_COBBLESTONE = new Block(48, 36, Material.STONE).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("stoneMoss");
    public static final Block OBSIDIAN = new ObsidianBlock(49, 37).setHardness(10.0f).setResistance(2000.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("obsidian");
    public static final Block TORCH = new TorchBlock(50, 80).setHardness(0.0f).setLuminance(0.9375f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("torch").ignoreMetaUpdates();
    public static final FireBlock FIRE = (FireBlock)new FireBlock(51, 31).setHardness(0.0f).setLuminance(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("fire").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block SPAWNER = new SpawnerBlock(52, 65).setHardness(5.0f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("mobSpawner").disableTrackingStatistics();
    public static final Block WOODEN_STAIRS = new StairsBlock(53, PLANKS).setTranslationKey("stairsWood").ignoreMetaUpdates();
    public static final Block CHEST = new ChestBlock(54).setHardness(2.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("chest").ignoreMetaUpdates();
    public static final Block REDSTONE_WIRE = new RedstoneWireBlock(55, 164).setHardness(0.0f).setSoundGroup(DEFAULT_SOUND_GROUP).setTranslationKey("redstoneDust").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block DIAMOND_ORE = new OreBlock(56, 50).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreDiamond");
    public static final Block DIAMOND_BLOCK = new OreStorageBlock(57, 24).setHardness(5.0f).setResistance(10.0f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("blockDiamond");
    public static final Block CRAFTING_TABLE = new WorkbenchBlock(58).setHardness(2.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("workbench");
    public static final Block WHEAT = new CropBlock(59, 88).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("crops").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block FARMLAND = new FarmlandBlock(60).setHardness(0.6f).setSoundGroup(GRAVEL_SOUND_GROUP).setTranslationKey("farmland");
    public static final Block FURNACE = new FurnaceBlock(61, false).setHardness(3.5f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("furnace").ignoreMetaUpdates();
    public static final Block LIT_FURNACE = new FurnaceBlock(62, true).setHardness(3.5f).setSoundGroup(STONE_SOUND_GROUP).setLuminance(0.875f).setTranslationKey("furnace").ignoreMetaUpdates();
    public static final Block SIGN = new SignBlock(63, SignBlockEntity.class, true).setHardness(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("sign").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block DOOR = new DoorBlock(64, Material.WOOD).setHardness(3.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("doorWood").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block LADDER = new LadderBlock(65, 83).setHardness(0.4f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("ladder").ignoreMetaUpdates();
    public static final Block RAIL = new RailBlock(66, 128, false).setHardness(0.7f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("rail").ignoreMetaUpdates();
    public static final Block COBBLESTONE_STAIRS = new StairsBlock(67, COBBLESTONE).setTranslationKey("stairsStone").ignoreMetaUpdates();
    public static final Block WALL_SIGN = new SignBlock(68, SignBlockEntity.class, false).setHardness(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("sign").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block LEVER = new LeverBlock(69, 96).setHardness(0.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("lever").ignoreMetaUpdates();
    public static final Block STONE_PRESSURE_PLATE = new PressurePlateBlock(70, Block.STONE.textureId, PressurePlateActivationRule.MOBS, Material.STONE).setHardness(0.5f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("pressurePlate").ignoreMetaUpdates();
    public static final Block IRON_DOOR = new DoorBlock(71, Material.METAL).setHardness(5.0f).setSoundGroup(METAL_SOUND_GROUP).setTranslationKey("doorIron").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block WOODEN_PRESSURE_PLATE = new PressurePlateBlock(72, Block.PLANKS.textureId, PressurePlateActivationRule.EVERYTHING, Material.WOOD).setHardness(0.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("pressurePlate").ignoreMetaUpdates();
    public static final Block REDSTONE_ORE = new RedstoneOreBlock(73, 51, false).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreRedstone").ignoreMetaUpdates();
    public static final Block LIT_REDSTONE_ORE = new RedstoneOreBlock(74, 51, true).setLuminance(0.625f).setHardness(3.0f).setResistance(5.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("oreRedstone").ignoreMetaUpdates();
    public static final Block REDSTONE_TORCH = new RedstoneTorchBlock(75, 115, false).setHardness(0.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("notGate").ignoreMetaUpdates();
    public static final Block LIT_REDSTONE_TORCH = new RedstoneTorchBlock(76, 99, true).setHardness(0.0f).setLuminance(0.5f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("notGate").ignoreMetaUpdates();
    public static final Block BUTTON = new ButtonBlock(77, Block.STONE.textureId).setHardness(0.5f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("button").ignoreMetaUpdates();
    public static final Block SNOW = new SnowyBlock(78, 66).setHardness(0.1f).setSoundGroup(WOOL_SOUND_GROUP).setTranslationKey("snow");
    public static final Block ICE = new IceBlock(79, 67).setHardness(0.5f).setOpacity(3).setSoundGroup(GLASS_SOUND_GROUP).setTranslationKey("ice");
    public static final Block SNOW_BLOCK = new SnowBlock(80, 66).setHardness(0.2f).setSoundGroup(WOOL_SOUND_GROUP).setTranslationKey("snow");
    public static final Block CACTUS = new CactusBlock(81, 70).setHardness(0.4f).setSoundGroup(WOOL_SOUND_GROUP).setTranslationKey("cactus");
    public static final Block CLAY = new ClayBlock(82, 72).setHardness(0.6f).setSoundGroup(GRAVEL_SOUND_GROUP).setTranslationKey("clay");
    public static final Block SUGAR_CANE = new SugarCaneBlock(83, 73).setHardness(0.0f).setSoundGroup(DIRT_SOUND_GROUP).setTranslationKey("reeds").disableTrackingStatistics();
    public static final Block JUKEBOX = new JukeboxBlock(84, 74).setHardness(2.0f).setResistance(10.0f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("jukebox").ignoreMetaUpdates();
    public static final Block FENCE = new FenceBlock(85, 4).setHardness(2.0f).setResistance(5.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("fence").ignoreMetaUpdates();
    public static final Block PUMPKIN = new PumpkinBlock(86, 102, false).setHardness(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("pumpkin").ignoreMetaUpdates();
    public static final Block NETHERRACK = new NetherrackBlock(87, 103).setHardness(0.4f).setSoundGroup(STONE_SOUND_GROUP).setTranslationKey("hellrock");
    public static final Block SOUL_SAND = new SoulSandBlock(88, 104).setHardness(0.5f).setSoundGroup(SAND_SOUND_GROUP).setTranslationKey("hellsand");
    public static final Block GLOWSTONE = new GlowstoneBlock(89, 105, Material.STONE).setHardness(0.3f).setSoundGroup(GLASS_SOUND_GROUP).setLuminance(1.0f).setTranslationKey("lightgem");
    public static final NetherPortalBlock NETHER_PORTAL = (NetherPortalBlock)new NetherPortalBlock(90, 14).setHardness(-1.0f).setSoundGroup(GLASS_SOUND_GROUP).setLuminance(0.75f).setTranslationKey("portal");
    public static final Block JACK_O_LANTERN = new PumpkinBlock(91, 102, true).setHardness(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setLuminance(1.0f).setTranslationKey("litpumpkin").ignoreMetaUpdates();
    public static final Block CAKE = new CakeBlock(92, 121).setHardness(0.5f).setSoundGroup(WOOL_SOUND_GROUP).setTranslationKey("cake").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block REPEATER = new RepeaterBlock(93, false).setHardness(0.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("diode").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block POWERED_REPEATER = new RepeaterBlock(94, true).setHardness(0.0f).setLuminance(0.625f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("diode").disableTrackingStatistics().ignoreMetaUpdates();
    public static final Block LOCKED_CHEST = new LockedChestBlock(95).setHardness(0.0f).setLuminance(1.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("lockedchest").setTickRandomly(true).ignoreMetaUpdates();
    public static final Block TRAPDOOR = new TrapdoorBlock(96, Material.WOOD).setHardness(3.0f).setSoundGroup(WOOD_SOUND_GROUP).setTranslationKey("trapdoor").disableTrackingStatistics().ignoreMetaUpdates();
    public int textureId;
    public final int id;
    protected float hardness;
    protected float resistance;
    protected boolean constructed = true;
    protected boolean shouldTrackStatistics = true;
    public double minX;
    public double minY;
    public double minZ;
    public double maxX;
    public double maxY;
    public double maxZ;
    public BlockSoundGroup soundGroup = DEFAULT_SOUND_GROUP;
    public float particleFallSpeedModifier = 1.0f;
    public final Material material;
    public float slipperiness = 0.6f;
    private String translationKey;

    protected Block(int id, Material material) {
        if (BLOCKS[id] != null) {
            throw new IllegalArgumentException("Slot " + id + " is already occupied by " + BLOCKS[id] + " when adding " + this);
        }
        this.material = material;
        Block.BLOCKS[id] = this;
        this.id = id;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        Block.BLOCKS_OPAQUE[id] = this.isOpaque();
        Block.BLOCKS_LIGHT_OPACITY[id] = this.isOpaque() ? 255 : 0;
        Block.BLOCKS_ALLOW_VISION[id] = !material.blocksVision();
        Block.BLOCKS_WITH_ENTITY[id] = false;
    }

    protected Block ignoreMetaUpdates() {
        Block.BLOCKS_IGNORE_META_UPDATE[this.id] = true;
        return this;
    }

    protected void init() {
    }

    protected Block(int id, int textureId, Material material) {
        this(id, material);
        this.textureId = textureId;
    }

    protected Block setSoundGroup(BlockSoundGroup soundGroup) {
        this.soundGroup = soundGroup;
        return this;
    }

    protected Block setOpacity(int i) {
        Block.BLOCKS_LIGHT_OPACITY[this.id] = i;
        return this;
    }

    protected Block setLuminance(float fractionalValue) {
        Block.BLOCKS_LIGHT_LUMINANCE[this.id] = (int)(15.0f * fractionalValue);
        return this;
    }

    protected Block setResistance(float resistance) {
        this.resistance = resistance * 3.0f;
        return this;
    }

    public boolean isFullCube() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 0;
    }

    protected Block setHardness(float hardness) {
        this.hardness = hardness;
        if (this.resistance < hardness * 5.0f) {
            this.resistance = hardness * 5.0f;
        }
        return this;
    }

    protected Block setUnbreakable() {
        this.setHardness(-1.0f);
        return this;
    }

    public float getHardness() {
        return this.hardness;
    }

    protected Block setTickRandomly(boolean tickRandomly) {
        Block.BLOCKS_RANDOM_TICK[this.id] = tickRandomly;
        return this;
    }

    public void setBoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) {
        this.minX = minX;
        this.minY = minY;
        this.minZ = minZ;
        this.maxX = maxX;
        this.maxY = maxY;
        this.maxZ = maxZ;
    }

    @Environment(value=EnvType.CLIENT)
    public float getLuminance(BlockView blockView, int x, int y, int z) {
        return blockView.getNaturalBrightness(x, y, z, BLOCKS_LIGHT_LUMINANCE[this.id]);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        if (side == 0 && this.minY > 0.0) {
            return true;
        }
        if (side == 1 && this.maxY < 1.0) {
            return true;
        }
        if (side == 2 && this.minZ > 0.0) {
            return true;
        }
        if (side == 3 && this.maxZ < 1.0) {
            return true;
        }
        if (side == 4 && this.minX > 0.0) {
            return true;
        }
        if (side == 5 && this.maxX < 1.0) {
            return true;
        }
        return !blockView.isBlockOpaqueCube(x, y, z);
    }

    public boolean isSolidFace(BlockView blockView, int x, int y, int z, int face) {
        return blockView.getMaterial(x, y, z).isSolid();
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(BlockView blockView, int x, int y, int z, int side) {
        return this.getTexture(side, blockView.getBlockMeta(x, y, z));
    }

    public int getTexture(int side, int meta) {
        return this.getTexture(side);
    }

    public int getTexture(int side) {
        return this.textureId;
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        return Box.createCached((double)x + this.minX, (double)y + this.minY, (double)z + this.minZ, (double)x + this.maxX, (double)y + this.maxY, (double)z + this.maxZ);
    }

    public void addIntersectingBoundingBox(World world, int x, int y, int z, Box box, ArrayList boxes) {
        Box box2 = this.getCollisionShape(world, x, y, z);
        if (box2 != null && box.intersects(box2)) {
            boxes.add(box2);
        }
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return Box.createCached((double)x + this.minX, (double)y + this.minY, (double)z + this.minZ, (double)x + this.maxX, (double)y + this.maxY, (double)z + this.maxZ);
    }

    public boolean isOpaque() {
        return true;
    }

    public boolean hasCollision(int meta, boolean allowLiquids) {
        return this.hasCollision();
    }

    public boolean hasCollision() {
        return true;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
    }

    public void onMetadataChange(World world, int x, int y, int z, int meta) {
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
    }

    public int getTickRate() {
        return 10;
    }

    public void onPlaced(World world, int x, int y, int z) {
    }

    public void onBreak(World world, int x, int y, int z) {
    }

    public int getDroppedItemCount(Random random) {
        return 1;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return this.id;
    }

    public float getHardness(PlayerEntity player) {
        if (this.hardness < 0.0f) {
            return 0.0f;
        }
        if (!player.canHarvest(this)) {
            return 1.0f / this.hardness / 100.0f;
        }
        return player.getBlockBreakingSpeed(this) / this.hardness / 30.0f;
    }

    public final void dropStacks(World world, int x, int y, int z, int l) {
        this.dropStacks(world, x, y, z, l, 1.0f);
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        if (world.isRemote) {
            return;
        }
        int n = this.getDroppedItemCount(world.random);
        for (int i = 0; i < n; ++i) {
            int n2;
            if (world.random.nextFloat() > luck || (n2 = this.getDroppedItemId(meta, world.random)) <= 0) continue;
            this.dropStack(world, x, y, z, new ItemStack(n2, 1, this.getDroppedItemMeta(meta)));
        }
    }

    protected void dropStack(World world, int x, int y, int z, ItemStack itemStack) {
        if (world.isRemote) {
            return;
        }
        float f = 0.7f;
        double d = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        double d2 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        double d3 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        ItemEntity itemEntity = new ItemEntity(world, (double)x + d, (double)y + d2, (double)z + d3, itemStack);
        itemEntity.pickupDelay = 10;
        world.spawnEntity(itemEntity);
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return 0;
    }

    public float getBlastResistance(Entity entity) {
        return this.resistance / 5.0f;
    }

    public HitResult raycast(World world, int x, int y, int z, Vec3d startPos, Vec3d endPos) {
        this.updateBoundingBox(world, x, y, z);
        startPos = startPos.add(-x, -y, -z);
        endPos = endPos.add(-x, -y, -z);
        Vec3d vec3d = startPos.interpolateByX(endPos, this.minX);
        Vec3d vec3d2 = startPos.interpolateByX(endPos, this.maxX);
        Vec3d vec3d3 = startPos.interpolateByY(endPos, this.minY);
        Vec3d vec3d4 = startPos.interpolateByY(endPos, this.maxY);
        Vec3d vec3d5 = startPos.interpolateByZ(endPos, this.minZ);
        Vec3d vec3d6 = startPos.interpolateByZ(endPos, this.maxZ);
        if (!this.containsInYZPlane(vec3d)) {
            vec3d = null;
        }
        if (!this.containsInYZPlane(vec3d2)) {
            vec3d2 = null;
        }
        if (!this.containsInXZPlane(vec3d3)) {
            vec3d3 = null;
        }
        if (!this.containsInXZPlane(vec3d4)) {
            vec3d4 = null;
        }
        if (!this.containsInXYPlane(vec3d5)) {
            vec3d5 = null;
        }
        if (!this.containsInXYPlane(vec3d6)) {
            vec3d6 = null;
        }
        Vec3d vec3d7 = null;
        if (vec3d != null && (vec3d7 == null || startPos.distanceTo(vec3d) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d;
        }
        if (vec3d2 != null && (vec3d7 == null || startPos.distanceTo(vec3d2) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d2;
        }
        if (vec3d3 != null && (vec3d7 == null || startPos.distanceTo(vec3d3) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d3;
        }
        if (vec3d4 != null && (vec3d7 == null || startPos.distanceTo(vec3d4) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d4;
        }
        if (vec3d5 != null && (vec3d7 == null || startPos.distanceTo(vec3d5) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d5;
        }
        if (vec3d6 != null && (vec3d7 == null || startPos.distanceTo(vec3d6) < startPos.distanceTo(vec3d7))) {
            vec3d7 = vec3d6;
        }
        if (vec3d7 == null) {
            return null;
        }
        int n = -1;
        if (vec3d7 == vec3d) {
            n = 4;
        }
        if (vec3d7 == vec3d2) {
            n = 5;
        }
        if (vec3d7 == vec3d3) {
            n = 0;
        }
        if (vec3d7 == vec3d4) {
            n = 1;
        }
        if (vec3d7 == vec3d5) {
            n = 2;
        }
        if (vec3d7 == vec3d6) {
            n = 3;
        }
        return new HitResult(x, y, z, n, vec3d7.add(x, y, z));
    }

    private boolean containsInYZPlane(Vec3d arg) {
        if (arg == null) {
            return false;
        }
        return arg.y >= this.minY && arg.y <= this.maxY && arg.z >= this.minZ && arg.z <= this.maxZ;
    }

    private boolean containsInXZPlane(Vec3d arg) {
        if (arg == null) {
            return false;
        }
        return arg.x >= this.minX && arg.x <= this.maxX && arg.z >= this.minZ && arg.z <= this.maxZ;
    }

    private boolean containsInXYPlane(Vec3d arg) {
        if (arg == null) {
            return false;
        }
        return arg.x >= this.minX && arg.x <= this.maxX && arg.y >= this.minY && arg.y <= this.maxY;
    }

    public void onDestroyedByExplosion(World world, int x, int y, int z) {
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return 0;
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
        return this.canPlaceAt(world, x, y, z);
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y, z);
        return n == 0 || Block.BLOCKS[n].material.isReplaceable();
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        return false;
    }

    public void onSteppedOn(World world, int x, int y, int z, Entity entity) {
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
    }

    public void applyVelocity(World world, int x, int y, int z, Entity entity, Vec3d velocity) {
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
    }

    @Environment(value=EnvType.CLIENT)
    public int getColor(int meta) {
        return 0xFFFFFF;
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        return 0xFFFFFF;
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        return false;
    }

    public boolean canEmitRedstonePower() {
        return false;
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
    }

    public void afterBreak(World world, PlayerEntity playerEntity, int x, int y, int z, int meta) {
        playerEntity.increaseStat(Stats.MINE_BLOCK[this.id], 1);
        this.dropStacks(world, x, y, z, meta);
    }

    public boolean canGrow(World world, int x, int y, int z) {
        return true;
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
    }

    public Block setTranslationKey(String name) {
        this.translationKey = "tile." + name;
        return this;
    }

    public String getTranslatedName() {
        return I18n.getTranslation(this.getTranslationKey() + ".name");
    }

    public String getTranslationKey() {
        return this.translationKey;
    }

    public void onBlockAction(World world, int x, int y, int z, int data1, int data2) {
    }

    public boolean isTrackingStatistics() {
        return this.shouldTrackStatistics;
    }

    protected Block disableTrackingStatistics() {
        this.shouldTrackStatistics = false;
        return this;
    }

    public int getPistonBehavior() {
        return this.material.getPistonBehavior();
    }

    static {
        Item.ITEMS[Block.WOOL.id] = new WoolBlockItem(Block.WOOL.id - 256).setTranslationKey("cloth");
        Item.ITEMS[Block.LOG.id] = new LogBlockItem(Block.LOG.id - 256).setTranslationKey("log");
        Item.ITEMS[Block.SLAB.id] = new SlabBlockItem(Block.SLAB.id - 256).setTranslationKey("stoneSlab");
        Item.ITEMS[Block.SAPLING.id] = new SaplingBlockItem(Block.SAPLING.id - 256).setTranslationKey("sapling");
        Item.ITEMS[Block.LEAVES.id] = new LeavesBlockItem(Block.LEAVES.id - 256).setTranslationKey("leaves");
        Item.ITEMS[Block.PISTON.id] = new PistonBlockItem(Block.PISTON.id - 256);
        Item.ITEMS[Block.STICKY_PISTON.id] = new PistonBlockItem(Block.STICKY_PISTON.id - 256);
        for (int i = 0; i < 256; ++i) {
            if (BLOCKS[i] == null || Item.ITEMS[i] != null) continue;
            Item.ITEMS[i] = new BlockItem(i - 256);
            BLOCKS[i].init();
        }
        Block.BLOCKS_ALLOW_VISION[0] = true;
        Stats.initializeItemStats();
    }
}

