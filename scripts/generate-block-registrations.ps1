$ErrorActionPreference = "Stop"
$BlockDir = Resolve-Path (Join-Path $PSScriptRoot "..\src\net\minecraft\block")

function Make-Snippet {
    param([string]$Fn, [int]$Priority, [string]$Body)
    $indented = ($Body -split "`n" | ForEach-Object { "    $_" }) -join "`n"
    return @"

namespace {

void $Fn()
{
$indented
}

MINECRAFT_REGISTER_BLOCK($Fn, $Priority);

} // namespace
"@
}

function Write-NewReg {
    param([string]$Name, [string[]]$Includes, [string]$Fn, [int]$Priority, [string]$Body)
    $path = Join-Path $BlockDir $Name
    $incs = ($Includes | ForEach-Object { "#include `"$_`"" }) -join "`n"
    $snippet = Make-Snippet -Fn $Fn -Priority $Priority -Body $Body
    $content = "$incs`n`nnamespace net::minecraft::block {$snippet`n} // namespace net::minecraft::block`n"
    Set-Content -LiteralPath $path -Value $content -Encoding UTF8
}

function Add-Reg {
    param([string]$Name, [string[]]$ExtraIncludes, [string]$Fn, [int]$Priority, [string]$Body)
    $path = Join-Path $BlockDir $Name
    if (-not (Test-Path $path)) { throw "Missing $Name" }
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -match 'MINECRAFT_REGISTER_BLOCK') { return }
    foreach ($inc in $ExtraIncludes) {
        if ($content -notmatch [regex]::Escape($inc)) {
            $content = "#include `"$inc`"`n" + $content
        }
    }
    if ($content -notmatch 'BlockAutoRegistry\.hpp') {
        $content = "#include `"net/minecraft/block/BlockAutoRegistry.hpp`"`n" + $content
    }
    $snippet = Make-Snippet -Fn $Fn -Priority $Priority -Body $Body
    if ($content -match '(?s)(.*namespace net::minecraft::block\s*\{)(.*)(\}\s*//\s*namespace net::minecraft::block\s*)$') {
        $content = $Matches[1] + $Matches[2].TrimEnd() + $snippet + "`n" + $Matches[3]
    } else {
        throw "Could not parse namespace in $Name"
    }
    Set-Content -LiteralPath $path -Value $content -Encoding UTF8
}

# --- new registration-only translation units ---
Write-NewReg "StoneBlock.cpp" @(
    "net/minecraft/block/StoneBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerStoneBlock" 1 '(new StoneBlock(1, 1))->setHardness(1.5f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stone");'

Write-NewReg "CobblestoneBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerCobblestoneBlock" 4 @'
namespace mat = material;
(new Block(4, 16, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stonebrick");
'@

Write-NewReg "PlanksBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerPlanksBlock" 5 @'
namespace mat = material;
(new Block(5, 4, mat::Material::WOOD))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("wood")->ignoreMetaUpdates();
'@

Write-NewReg "SaplingBlock.cpp" @(
    "net/minecraft/block/SaplingBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSaplingBlock" 6 '(new SaplingBlock(6, 15))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sapling")->ignoreMetaUpdates();'

Write-NewReg "BedrockBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerBedrockBlock" 7 @'
namespace mat = material;
(new Block(7, 17, mat::Material::STONE))->setUnbreakable()->setResistance(6000000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("bedrock")->disableTrackingStatistics();
'@

Write-NewReg "SandBlock.cpp" @(
    "net/minecraft/block/SandBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSandBlock" 12 '(new SandBlock(12, 18))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("sand");'

Write-NewReg "GravelBlock.cpp" @(
    "net/minecraft/block/GravelBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerGravelBlock" 13 '(new GravelBlock(13, 19))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("gravel");'

Write-NewReg "OreBlock.cpp" @(
    "net/minecraft/block/OreBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerOreBlocks" 14 @'
(new OreBlock(14, 32))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreGold");
(new OreBlock(15, 33))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreIron");
(new OreBlock(16, 34))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreCoal");
(new OreBlock(21, 160))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreLapis");
(new OreBlock(56, 50))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreDiamond");
'@

Write-NewReg "SpongeBlock.cpp" @(
    "net/minecraft/block/SpongeBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSpongeBlock" 19 '(new SpongeBlock(19))->setHardness(0.6f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sponge");'

Write-NewReg "GlassBlock.cpp" @(
    "net/minecraft/block/GlassBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerGlassBlock" 20 @'
namespace mat = material;
(new GlassBlock(20, 49, mat::Material::GLASS, false))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("glass");
'@

Write-NewReg "LapisBlockBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerLapisBlock" 22 @'
namespace mat = material;
(new Block(22, 144, mat::Material::STONE))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("blockLapis");
'@

Write-NewReg "SandstoneBlock.cpp" @(
    "net/minecraft/block/SandstoneBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSandstoneBlock" 24 '(new SandstoneBlock(24))->setSoundGroup(&vanillaStoneSound())->setHardness(0.8f)->setTranslationKey("sandStone");'

Write-NewReg "CobwebBlock.cpp" @(
    "net/minecraft/block/CobwebBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp"
) "registerCobwebBlock" 30 '(new CobwebBlock(30, 11))->setOpacity(1)->setHardness(4.0f)->setTranslationKey("web");'

Write-NewReg "TallPlantBlock.cpp" @(
    "net/minecraft/block/TallPlantBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerTallPlantBlock" 31 '(new TallPlantBlock(31, 39))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("tallgrass");'

Write-NewReg "DeadBushBlock.cpp" @(
    "net/minecraft/block/DeadBushBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerDeadBushBlock" 32 '(new DeadBushBlock(32, 55))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("deadbush");'

Write-NewReg "WoolBlock.cpp" @(
    "net/minecraft/block/WoolBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerWoolBlock" 35 '(new WoolBlock())->setHardness(0.8f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cloth")->ignoreMetaUpdates();'

Write-NewReg "PlantBlock.cpp" @(
    "net/minecraft/block/PlantBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerPlantBlocks" 37 @'
(new PlantBlock(37, 13))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("flower");
(new PlantBlock(38, 12))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("rose");
'@

Write-NewReg "MushroomPlantBlock.cpp" @(
    "net/minecraft/block/MushroomPlantBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerMushroomPlantBlocks" 39 @'
(new MushroomPlantBlock(39, 29))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setLuminance(0.125f)->setTranslationKey("mushroom");
(new MushroomPlantBlock(40, 28))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("mushroom");
'@

Write-NewReg "OreStorageBlock.cpp" @(
    "net/minecraft/block/OreStorageBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerOreStorageBlocks" 41 @'
(new OreStorageBlock(41, 23))->setHardness(3.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockGold");
(new OreStorageBlock(42, 22))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockIron");
(new OreStorageBlock(57, 24))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockDiamond");
'@

Write-NewReg "BricksBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerBricksBlock" 45 @'
namespace mat = material;
(new Block(45, 7, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("brick");
'@

Write-NewReg "BookshelfBlock.cpp" @(
    "net/minecraft/block/BookshelfBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerBookshelfBlock" 47 '(new BookshelfBlock(47, 35))->setHardness(1.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("bookshelf");'

Write-NewReg "MossyCobblestoneBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerMossyCobblestoneBlock" 48 @'
namespace mat = material;
(new Block(48, 36, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneMoss");
'@

Write-NewReg "ObsidianBlock.cpp" @(
    "net/minecraft/block/ObsidianBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerObsidianBlock" 49 '(new ObsidianBlock(49, 37))->setHardness(10.0f)->setResistance(2000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("obsidian");'

Write-NewReg "SpawnerBlock.cpp" @(
    "net/minecraft/block/SpawnerBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSpawnerBlock" 52 '(new SpawnerBlock(52, 65))->setHardness(5.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("mobSpawner")->disableTrackingStatistics();'

Write-NewReg "CropBlock.cpp" @(
    "net/minecraft/block/CropBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerCropBlock" 59 '(new CropBlock(59, 88))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("crops")->disableTrackingStatistics()->ignoreMetaUpdates();'

Write-NewReg "FarmlandBlock.cpp" @(
    "net/minecraft/block/FarmlandBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerFarmlandBlock" 60 '(new FarmlandBlock(60))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("farmland");'

Write-NewReg "CactusBlock.cpp" @(
    "net/minecraft/block/CactusBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerCactusBlock" 81 '(new CactusBlock(81, 70))->setHardness(0.4f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cactus");'

Write-NewReg "ClayBlock.cpp" @(
    "net/minecraft/block/ClayBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerClayBlock" 82 '(new ClayBlock(82, 72))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("clay");'

Write-NewReg "SugarCaneBlock.cpp" @(
    "net/minecraft/block/SugarCaneBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSugarCaneBlock" 83 '(new SugarCaneBlock(83, 73))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("reeds")->disableTrackingStatistics();'

Write-NewReg "FenceBlock.cpp" @(
    "net/minecraft/block/FenceBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerFenceBlock" 85 '(new FenceBlock(85, 4))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fence")->ignoreMetaUpdates();'

Write-NewReg "NetherrackBlock.cpp" @(
    "net/minecraft/block/NetherrackBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerNetherrackBlock" 87 '(new NetherrackBlock(87, 103))->setHardness(0.4f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("hellrock");'

Write-NewReg "SoulSandBlock.cpp" @(
    "net/minecraft/block/SoulSandBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerSoulSandBlock" 88 '(new SoulSandBlock(88, 104))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("hellsand");'

Write-NewReg "GlowstoneBlock.cpp" @(
    "net/minecraft/block/GlowstoneBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerGlowstoneBlock" 89 @'
namespace mat = material;
(new GlowstoneBlock(89, 105, mat::Material::STONE))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setLuminance(1.0f)->setTranslationKey("lightgem");
'@

Write-NewReg "CakeBlock.cpp" @(
    "net/minecraft/block/CakeBlock.hpp",
    "net/minecraft/block/BlockAutoRegistry.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp"
) "registerCakeBlock" 92 '(new CakeBlock(92, 121))->setHardness(0.5f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cake")->disableTrackingStatistics()->ignoreMetaUpdates();'

# --- append to existing implementation files ---
Add-Reg "GrassBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerGrassBlock" 2 '(new GrassBlock(2))->setHardness(0.6f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("grass");'

Add-Reg "FlowingLiquidBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp", "net/minecraft/block/material/Material.hpp") "registerFlowingLiquidBlocks" 8 @'
namespace mat = material;
(new FlowingLiquidBlock(8, mat::Material::WATER))->setHardness(100.0f)->setOpacity(3)->setTranslationKey("water")->disableTrackingStatistics()->ignoreMetaUpdates();
(new FlowingLiquidBlock(10, mat::Material::LAVA))->setHardness(0.0f)->setLuminance(1.0f)->setOpacity(255)->setTranslationKey("lava")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Add-Reg "StillLiquidBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp", "net/minecraft/block/material/Material.hpp") "registerStillLiquidBlocks" 9 @'
namespace mat = material;
(new StillLiquidBlock(9, mat::Material::WATER))->setHardness(100.0f)->setOpacity(3)->setTranslationKey("water")->disableTrackingStatistics()->ignoreMetaUpdates();
(new StillLiquidBlock(11, mat::Material::LAVA))->setHardness(100.0f)->setLuminance(1.0f)->setOpacity(255)->setTranslationKey("lava")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Add-Reg "LogBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerLogBlock" 17 '(new LogBlock(17))->setHardness(2.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("log")->ignoreMetaUpdates();'

Add-Reg "LeavesBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerLeavesBlock" 18 '(new LeavesBlock(18, 52))->setHardness(0.2f)->setOpacity(1)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("leaves")->disableTrackingStatistics()->ignoreMetaUpdates();'

Add-Reg "DispenserBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerDispenserBlock" 23 '(new DispenserBlock(23))->setHardness(3.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("dispenser")->ignoreMetaUpdates();'

Add-Reg "NoteBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerNoteBlock" 25 '(new NoteBlock(25))->setHardness(0.8f)->setTranslationKey("musicBlock")->ignoreMetaUpdates();'

Add-Reg "BedBlock.cpp" @() "registerBedBlock" 26 '(new BedBlock(26))->setHardness(0.2f)->setTranslationKey("bed")->disableTrackingStatistics()->ignoreMetaUpdates();'

Add-Reg "RailBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerRailBlocks" 27 @'
(new RailBlock(27, 179, true))->setHardness(0.7f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("goldenRail")->ignoreMetaUpdates();
(new RailBlock(66, 128, false))->setHardness(0.7f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("rail")->ignoreMetaUpdates();
'@

Add-Reg "DetectorRailBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerDetectorRailBlock" 28 '(new DetectorRailBlock(28, 195))->setHardness(0.7f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("detectorRail")->ignoreMetaUpdates();'

Add-Reg "PistonBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerPistonBlocks" 29 @'
(new PistonBlock(29, 106, true))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("pistonStickyBase")->ignoreMetaUpdates();
(new PistonBlock(33, 107, false))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("pistonBase")->ignoreMetaUpdates();
'@

Add-Reg "PistonHeadBlock.cpp" @() "registerPistonHeadBlock" 34 '(new PistonHeadBlock(34, 107))->ignoreMetaUpdates();'

Add-Reg "PistonExtensionBlock.cpp" @() "registerPistonExtensionBlock" 36 'new PistonExtensionBlock(36);'

Add-Reg "SlabBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerSlabBlocks" 43 @'
(new SlabBlock(43, true))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneSlab");
(new SlabBlock(44, false))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneSlab");
'@

Add-Reg "TntBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerTntBlock" 46 '(new TntBlock(46, 8))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("tnt");'

Add-Reg "TorchBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerTorchBlock" 50 '(new TorchBlock(50, 80))->setHardness(0.0f)->setLuminance(0.9375f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("torch")->ignoreMetaUpdates();'

Add-Reg "FireBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerFireBlock" 51 '(new FireBlock(51, 31))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fire")->disableTrackingStatistics()->ignoreMetaUpdates();'

Add-Reg "StairsBlock.cpp" @("net/minecraft/block/Block.hpp") "registerStairsBlocks" 67 @'
(new StairsBlock(53, *Block::BLOCKS[5]))->setTranslationKey("stairsWood")->ignoreMetaUpdates();
(new StairsBlock(67, *Block::BLOCKS[4]))->setTranslationKey("stairsStone")->ignoreMetaUpdates();
'@

Add-Reg "ChestBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerChestBlock" 54 '(new ChestBlock(54))->setHardness(2.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("chest")->ignoreMetaUpdates();'

Add-Reg "RedstoneWireBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerRedstoneWireBlock" 55 '(new RedstoneWireBlock(55, 164))->setHardness(0.0f)->setSoundGroup(&vanillaDefaultSound())->setTranslationKey("redstoneDust")->disableTrackingStatistics()->ignoreMetaUpdates();'

Add-Reg "WorkbenchBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerWorkbenchBlock" 58 '(new WorkbenchBlock(58))->setHardness(2.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("workbench");'

Add-Reg "FurnaceBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerFurnaceBlocks" 61 @'
(new FurnaceBlock(61, false))->setHardness(3.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("furnace")->ignoreMetaUpdates();
(new FurnaceBlock(62, true))->setHardness(3.5f)->setSoundGroup(&vanillaStoneSound())->setLuminance(0.875f)->setTranslationKey("furnace")->ignoreMetaUpdates();
'@

Add-Reg "SignBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerSignBlocks" 63 @'
(new SignBlock(63, true))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
(new SignBlock(68, false))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("sign")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Add-Reg "DoorBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp", "net/minecraft/block/material/Material.hpp") "registerDoorBlocks" 71 @'
namespace mat = material;
(new DoorBlock(64, mat::Material::WOOD))->setHardness(3.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("doorWood")->disableTrackingStatistics()->ignoreMetaUpdates();
(new DoorBlock(71, mat::Material::METAL))->setHardness(5.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("doorIron")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Add-Reg "LadderBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerLadderBlock" 65 '(new LadderBlock(65, 83))->setHardness(0.4f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("ladder")->ignoreMetaUpdates();'

Add-Reg "LeverBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerLeverBlock" 69 '(new LeverBlock(69, 96))->setHardness(0.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("lever")->ignoreMetaUpdates();'

Add-Reg "PressurePlateBlock.cpp" @(
    "net/minecraft/block/Block.hpp",
    "net/minecraft/block/PressurePlateActivationRule.hpp",
    "net/minecraft/block/VanillaBlockSounds.hpp",
    "net/minecraft/block/material/Material.hpp"
) "registerPressurePlateBlocks" 72 @'
namespace mat = material;
(new PressurePlateBlock(70, Block::BLOCKS[1]->textureId, PressurePlateActivationRule::MOBS, mat::Material::STONE))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
(new PressurePlateBlock(72, Block::BLOCKS[5]->textureId, PressurePlateActivationRule::EVERYTHING, mat::Material::WOOD))->setHardness(0.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
'@

Add-Reg "RedstoneOreBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerRedstoneOreBlocks" 74 @'
(new RedstoneOreBlock(73, 51, false))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
(new RedstoneOreBlock(74, 51, true))->setLuminance(0.625f)->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
'@

Add-Reg "RedstoneTorchBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerRedstoneTorchBlocks" 76 @'
(new RedstoneTorchBlock(75, 115, false))->setHardness(0.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("notGate")->ignoreMetaUpdates();
(new RedstoneTorchBlock(76, 99, true))->setHardness(0.0f)->setLuminance(0.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("notGate")->ignoreMetaUpdates();
'@

Add-Reg "ButtonBlock.cpp" @("net/minecraft/block/Block.hpp", "net/minecraft/block/VanillaBlockSounds.hpp") "registerButtonBlock" 77 '(new ButtonBlock(77, Block::BLOCKS[1]->textureId))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("button")->ignoreMetaUpdates();'

Add-Reg "SnowyBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerSnowyBlock" 78 '(new SnowyBlock(78, 66))->setHardness(0.1f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("snow");'

Add-Reg "IceBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerIceBlock" 79 '(new IceBlock(79, 67))->setHardness(0.5f)->setOpacity(3)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("ice");'

Add-Reg "SnowBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerSnowBlock" 80 '(new SnowBlock(80, 66))->setHardness(0.2f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("snow");'

Add-Reg "JukeboxBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerJukeboxBlock" 84 '(new JukeboxBlock(84, 74))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("jukebox")->ignoreMetaUpdates();'

Add-Reg "PumpkinBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerPumpkinBlocks" 91 @'
(new PumpkinBlock(86, 102, false))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("pumpkin")->ignoreMetaUpdates();
(new PumpkinBlock(91, 102, true))->setHardness(1.0f)->setSoundGroup(&vanillaWoodSound())->setLuminance(1.0f)->setTranslationKey("litpumpkin")->ignoreMetaUpdates();
'@

Add-Reg "NetherPortalBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerNetherPortalBlock" 90 '(new NetherPortalBlock(90, 14))->setHardness(-1.0f)->setSoundGroup(&vanillaGlassSound())->setLuminance(0.75f)->setTranslationKey("portal");'

Add-Reg "RepeaterBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerRepeaterBlocks" 94 @'
(new RepeaterBlock(93, false))->setHardness(0.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("diode")->disableTrackingStatistics()->ignoreMetaUpdates();
(new RepeaterBlock(94, true))->setHardness(0.0f)->setLuminance(0.625f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("diode")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Add-Reg "LockedChestBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp") "registerLockedChestBlock" 95 '(new LockedChestBlock(95))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("lockedchest")->setTickRandomly(true)->ignoreMetaUpdates();'

Add-Reg "TrapdoorBlock.cpp" @("net/minecraft/block/VanillaBlockSounds.hpp", "net/minecraft/block/material/Material.hpp") "registerTrapdoorBlock" 96 @'
namespace mat = material;
(new TrapdoorBlock(96, mat::Material::WOOD))->setHardness(3.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("trapdoor")->disableTrackingStatistics()->ignoreMetaUpdates();
'@

Write-Host "Block registration migration complete."
