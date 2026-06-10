#!/usr/bin/env python3
"""Move vanilla recipes from central managers into item/block registerRecipes()."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ITEM = ROOT / "src" / "net" / "minecraft" / "item"
BLOCK = ROOT / "src" / "net" / "minecraft" / "block"

RECIPE_INCLUDE = '#include "net/minecraft/recipe/CraftingRecipeManager.hpp"'
BLOCK_INCLUDE = '#include "net/minecraft/block/Block.hpp"'
SMELT_INCLUDE = '#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"'


def ensure_includes(content: str, extra: list[str]) -> str:
    for inc in extra:
        if inc not in content:
            # insert after last #include in header block
            m = list(re.finditer(r'^#include .+\n', content, re.M))
            if m:
                pos = m[-1].end()
                content = content[:pos] + inc + "\n" + content[pos:]
            else:
                content = inc + "\n" + content
    return content


def replace_void_recipes(content: str, class_name: str, body: str) -> str:
    pattern = rf"void {class_name}::registerRecipes\(recipe::CraftingRecipeManager& recipeManager\)\s*\{{\s*\(void\)recipeManager;\s*\}}"
    repl = f"void {class_name}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)\n{{\n{body.strip()}\n}}"
    return re.sub(pattern, repl, content, count=1, flags=re.S)


def inject_register_call(content: str, class_name: str) -> str:
    needle = f"void {class_name}::registerClass()"
    if needle not in content:
        return content
    if "registerRecipes(recipe::CraftingRecipeManager::getInstance())" in content:
        return content
    # add before closing brace of registerClass
    pattern = rf"(void {class_name}::registerClass\(\)\s*\{{)(.*?)(\n\}})"
    def repl(m):
        body = m.group(2)
        if "registerRecipes" in body:
            return m.group(0)
        return m.group(1) + body + "\n    registerRecipes(recipe::CraftingRecipeManager::getInstance());" + m.group(3)
    return re.sub(pattern, repl, content, count=1, flags=re.S)


def add_block_register_recipes(cpp_path: Path, class_name: str, recipe_body: str) -> None:
    content = cpp_path.read_text(encoding="utf-8")
    hpp_path = cpp_path.with_suffix(".hpp")

    if hpp_path.exists():
        hpp = hpp_path.read_text(encoding="utf-8")
        if "registerRecipes" not in hpp:
            fwd = "namespace net::minecraft::recipe { class CraftingRecipeManager; }\n"
            if fwd.strip() not in hpp:
                hpp = hpp.replace("namespace net::minecraft::block {", fwd + "namespace net::minecraft::block {", 1)
            hpp = re.sub(
                rf"(class {class_name}[^\{{]+\{{)",
                r"\1\n    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);",
                hpp,
                count=1,
            )
            hpp_path.write_text(hpp, encoding="utf-8")
    elif f"struct {class_name}Registrar" in content:
        registrar = f"{class_name}Registrar"
        if f"{registrar}::registerRecipes" not in content:
            fn = f"""
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager)
    {{
{recipe_body}
    }}
"""
            content = re.sub(
                rf"(struct {registrar} \{{)",
                r"\1" + fn,
                content,
                count=1,
            )
            content = re.sub(
                rf"(struct {registrar}[\s\S]*?static void registerClass\(\)\s*\{{)([\s\S]*?)(\n\s*\}})",
                rf"\1\2\n        registerRecipes(recipe::CraftingRecipeManager::getInstance());\3",
                content,
                count=1,
            )
        content = ensure_includes(content, [RECIPE_INCLUDE, BLOCK_INCLUDE, '#include "net/minecraft/item/Item.hpp"'])
        cpp_path.write_text(content, encoding="utf-8")
        return
    else:
        print(f"SKIP (no hpp/registrar): {cpp_path.name}")
        return

    content = ensure_includes(content, [RECIPE_INCLUDE, BLOCK_INCLUDE, '#include "net/minecraft/item/Item.hpp"'])
    if f"void {class_name}::registerRecipes" not in content:
        fn = f"""
void {class_name}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{{
{recipe_body}
}}
"""
        content = re.sub(
            rf"(void {class_name}::registerClass\(\)[\s\S]*?\n\}})",
            r"\1" + fn,
            content,
            count=1,
        )
    content = inject_register_call(content, class_name)
    cpp_path.write_text(content, encoding="utf-8")


def patch_item_recipes(rel: str, class_name: str, recipe_lines: list[str]) -> None:
    cpp = ITEM / rel
    if not cpp.exists():
        print(f"MISSING {cpp}")
        return
    content = cpp.read_text(encoding="utf-8")
    body = "\n".join(f"    {line}" for line in recipe_lines)
    content = ensure_includes(content, [RECIPE_INCLUDE, BLOCK_INCLUDE])
    if f"void {class_name}::registerRecipes" in content:
        content = re.sub(
            rf"void {class_name}::registerRecipes\(recipe::CraftingRecipeManager& recipeManager\)\s*\{{[\s\S]*?\n\}}",
            f"void {class_name}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)\n{{\n{body}\n}}",
            content,
            count=1,
        )
    else:
        fn = f"""
void {class_name}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{{
{body}
}}
"""
        content = re.sub(
            rf"(void {class_name}::registerClass\(\)[\s\S]*?\n\}})",
            r"\1" + fn,
            content,
            count=1,
        )
    content = inject_register_call(content, class_name)
    cpp.write_text(content, encoding="utf-8")


def add_smelting(rel: str, class_name: str, lines: list[str]) -> None:
    cpp = ITEM / rel
    content = cpp.read_text(encoding="utf-8")
    content = ensure_includes(content, [SMELT_INCLUDE, BLOCK_INCLUDE])
    block = "\n".join(f"    {l}" for l in lines)
    if "SmeltingRecipeManager::instance()" in content:
        return
    content = re.sub(
        rf"(void {class_name}::registerClass\(\)[\s\S]*?)(\n\}})",
        rf"\1\n    {block}\2",
        content,
        count=1,
    )
    cpp.write_text(content, encoding="utf-8")


# --- item crafting recipes (output item owns recipe) ---
patch_item_recipes("misc/arrow.cpp", "ArrowItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(6), 4),',
    '    {std::string("X"), std::string("#"), std::string("Y"), \'Y\', Item::byRawId(32), \'X\', Item::byRawId(62), \'#\', Item::byRawId(24)});',
])

patch_item_recipes("misc/sugar.cpp", "SugarItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(97)),',
    '    {std::string("#"), \'#\', Item::byRawId(82)});',
])

patch_item_recipes("misc/iron_ingot.cpp", "IronIngotItem", [
    'recipeManager.addShapedRecipe(ItemStack(Block::IRON_BLOCK),',
    '    {std::string("###"), std::string("###"), std::string("###"), \'#\', ItemStack(Item::byRawId(9), 9)});',
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(9), 9),',
    '    {std::string("#"), \'#\', Block::IRON_BLOCK});',
])

patch_item_recipes("misc/gold_ingot.cpp", "GoldIngotItem", [
    'recipeManager.addShapedRecipe(ItemStack(Block::GOLD_BLOCK),',
    '    {std::string("###"), std::string("###"), std::string("###"), \'#\', ItemStack(Item::byRawId(10), 9)});',
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(10), 9),',
    '    {std::string("#"), \'#\', Block::GOLD_BLOCK});',
])

patch_item_recipes("misc/diamond.cpp", "DiamondItem", [
    'recipeManager.addShapedRecipe(ItemStack(Block::DIAMOND_BLOCK),',
    '    {std::string("###"), std::string("###"), std::string("###"), \'#\', ItemStack(Item::byRawId(8), 9)});',
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(8), 9),',
    '    {std::string("#"), \'#\', Block::DIAMOND_BLOCK});',
])

patch_item_recipes("misc/compass.cpp", "CompassItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(89)),',
    '    {std::string(" # "), std::string("#X#"), std::string(" # "), \'#\', Item::byRawId(9), \'X\', Item::byRawId(75)});',
])

patch_item_recipes("misc/clock.cpp", "ClockItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(91)),',
    '    {std::string(" # "), std::string("#X#"), std::string(" # "), \'#\', Item::byRawId(10), \'X\', Item::byRawId(75)});',
])

patch_item_recipes("food/golden_apple.cpp", "GoldenAppleItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(66)),',
    '    {std::string("###"), std::string("#X#"), std::string("###"), \'#\', Item::byRawId(24), \'X\', Block::GOLD_BLOCK});',
])

patch_item_recipes("transport/chest_minecart.cpp", "ChestMinecartItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(86)),',
    '    {std::string("A"), std::string("B"), \'A\', Block::CHEST, \'B\', Item::byRawId(72)});',
])

patch_item_recipes("transport/furnace_minecart.cpp", "FurnaceMinecartItem", [
    'recipeManager.addShapedRecipe(ItemStack(Item::byRawId(87)),',
    '    {std::string("A"), std::string("B"), \'A\', Block::FURNACE, \'B\', Item::byRawId(72)});',
])

# smelting on output items
add_smelting("misc/iron_ingot.cpp", "IronIngotItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Block::IRON_ORE->id, ItemStack(Item::byRawId(9)));',
])
add_smelting("misc/gold_ingot.cpp", "GoldIngotItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Block::GOLD_ORE->id, ItemStack(Item::byRawId(10)));',
])
add_smelting("misc/diamond.cpp", "DiamondItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Block::DIAMOND_ORE->id, ItemStack(Item::byRawId(8)));',
])
add_smelting("food/cooked_porkchop.cpp", "CookedPorkchopItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Item::byRawId(63)->id, ItemStack(Item::byRawId(64)));',
])
add_smelting("food/cooked_fish.cpp", "CookedFishItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Item::byRawId(93)->id, ItemStack(Item::byRawId(94)));',
])
add_smelting("misc/brick.cpp", "BrickItem", [
    'recipe::SmeltingRecipeManager::instance().addRecipe(Item::byRawId(81)->id, ItemStack(Item::byRawId(80)));',
])

# block recipes
BLOCK_RECIPES: list[tuple[str, str, list[str]]] = [
    ("FenceBlock.cpp", "FenceBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::FENCE, 2),',
        '    {std::string("###"), std::string("###"), \'#\', Item::byRawId(24)});',
    ]),
    ("JukeboxBlock.cpp", "JukeboxBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::JUKEBOX),',
        '    {std::string("###"), std::string("#X#"), std::string("###"), \'#\', Block::PLANKS, \'X\', Item::byRawId(8)});',
    ]),
    ("NoteBlock.cpp", "NoteBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::NOTE_BLOCK),',
        '    {std::string("###"), std::string("#X#"), std::string("###"), \'#\', Block::PLANKS, \'X\', Item::byRawId(75)});',
    ]),
    ("BookshelfBlock.cpp", "BookshelfBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::BOOKSHELF),',
        '    {std::string("###"), std::string("XXX"), std::string("###"), \'#\', Block::PLANKS, \'X\', Item::byRawId(84)});',
    ]),
    ("SnowBlock.cpp", "SnowBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::SNOW_BLOCK),',
        '    {std::string("##"), std::string("##"), \'#\', Item::byRawId(76)});',
    ]),
    ("ClayBlock.cpp", "ClayBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::CLAY),',
        '    {std::string("##"), std::string("##"), \'#\', Item::byRawId(81)});',
    ]),
    ("BricksBlock.cpp", "BricksBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::BRICKS),',
        '    {std::string("##"), std::string("##"), \'#\', Item::byRawId(80)});',
    ]),
    ("GlowstoneBlock.cpp", "GlowstoneBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::GLOWSTONE),',
        '    {std::string("##"), std::string("##"), \'#\', Item::byRawId(92)});',
    ]),
    ("WoolBlock.cpp", "WoolBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::WOOL),',
        '    {std::string("##"), std::string("##"), \'#\', Item::byRawId(31)});',
    ]),
    ("TntBlock.cpp", "TntBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::TNT),',
        '    {std::string("X#X"), std::string("#X#"), std::string("X#X"), \'X\', Item::byRawId(33), \'#\', Block::SAND});',
    ]),
    ("SlabBlock.cpp", "SlabBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 3), {std::string("###"), \'#\', Block::COBBLESTONE});',
        'recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 0), {std::string("###"), \'#\', Block::STONE});',
        'recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 1), {std::string("###"), \'#\', Block::SANDSTONE});',
        'recipeManager.addShapedRecipe(ItemStack(Block::SLAB, 3, 2), {std::string("###"), \'#\', Block::PLANKS});',
    ]),
    ("LadderBlock.cpp", "LadderBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::LADDER, 2),',
        '    {std::string("# #"), std::string("###"), std::string("# #"), \'#\', Item::byRawId(24)});',
    ]),
    ("TrapdoorBlock.cpp", "TrapdoorBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::TRAPDOOR, 2),',
        '    {std::string("###"), std::string("###"), \'#\', Block::PLANKS});',
    ]),
    ("PlanksBlock.cpp", "PlanksBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::PLANKS, 4), {std::string("#"), \'#\', Block::LOG});',
    ]),
    ("TorchBlock.cpp", "TorchBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::TORCH, 4), {std::string("X"), std::string("#"), \'X\', Item::byRawId(7), \'#\', Item::byRawId(24)});',
        'recipeManager.addShapedRecipe(ItemStack(Block::TORCH, 4),',
        '    {std::string("X"), std::string("#"), \'X\', ItemStack(Item::byRawId(7), 1, 1), \'#\', Item::byRawId(24)});',
    ]),
    ("RailBlock.cpp", "RailBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::RAIL, 16),',
        '    {std::string("X X"), std::string("X#X"), std::string("X X"), \'X\', Item::byRawId(9), \'#\', Item::byRawId(24)});',
        'recipeManager.addShapedRecipe(ItemStack(Block::POWERED_RAIL, 6),',
        '    {std::string("X X"), std::string("X#X"), std::string("XRX"), \'X\', Item::byRawId(10), \'R\', Item::byRawId(75), \'#\', Item::byRawId(24)});',
    ]),
    ("DetectorRailBlock.cpp", "DetectorRailBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::DETECTOR_RAIL, 6),',
        '    {std::string("X X"), std::string("X#X"), std::string("XRX"), \'X\', Item::byRawId(9), \'R\', Item::byRawId(75), \'#\', Block::STONE_PRESSURE_PLATE});',
    ]),
    ("PumpkinBlock.cpp", "PumpkinBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::JACK_O_LANTERN),',
        '    {std::string("A"), std::string("B"), \'A\', Block::PUMPKIN, \'B\', Block::TORCH});',
    ]),
    ("StairsBlock.cpp", "StairsBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::WOODEN_STAIRS, 4),',
        '    {std::string("#  "), std::string("## "), std::string("###"), \'#\', Block::PLANKS});',
        'recipeManager.addShapedRecipe(ItemStack(Block::COBBLESTONE_STAIRS, 4),',
        '    {std::string("#  "), std::string("## "), std::string("###"), \'#\', Block::COBBLESTONE});',
    ]),
    ("LeverBlock.cpp", "LeverBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::LEVER),',
        '    {std::string("X"), std::string("#"), \'#\', Block::COBBLESTONE, \'X\', Item::byRawId(24)});',
    ]),
    ("RedstoneTorchBlock.cpp", "RedstoneTorchBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::LIT_REDSTONE_TORCH),',
        '    {std::string("X"), std::string("#"), \'#\', Item::byRawId(24), \'X\', Item::byRawId(75)});',
    ]),
    ("ButtonBlock.cpp", "ButtonBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::BUTTON), {std::string("#"), std::string("#"), \'#\', Block::STONE});',
    ]),
    ("PressurePlateBlock.cpp", "PressurePlateBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::STONE_PRESSURE_PLATE), {std::string("##"), \'#\', Block::STONE});',
        'recipeManager.addShapedRecipe(ItemStack(Block::WOODEN_PRESSURE_PLATE), {std::string("##"), \'#\', Block::PLANKS});',
    ]),
    ("DispenserBlock.cpp", "DispenserBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::DISPENSER),',
        '    {std::string("###"), std::string("#X#"), std::string("#R#"), \'#\', Block::COBBLESTONE, \'X\', Item::byRawId(5), \'R\', Item::byRawId(75)});',
    ]),
    ("PistonBlock.cpp", "PistonBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::PISTON),',
        '    {std::string("TTT"), std::string("#X#"), std::string("#R#"), \'#\', Block::COBBLESTONE, \'X\', Item::byRawId(9), \'R\', Item::byRawId(75), \'T\', Block::PLANKS});',
        'recipeManager.addShapedRecipe(ItemStack(Block::STICKY_PISTON),',
        '    {std::string("S"), std::string("P"), \'S\', Item::byRawId(85), \'P\', Block::PISTON});',
    ]),
    ("ChestBlock.cpp", "ChestBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::CHEST),',
        '    {std::string("###"), std::string("# #"), std::string("###"), \'#\', Block::PLANKS});',
    ]),
    ("FurnaceBlock.cpp", "FurnaceBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::FURNACE),',
        '    {std::string("###"), std::string("# #"), std::string("###"), \'#\', Block::COBBLESTONE});',
    ]),
    ("SandstoneBlock.cpp", "SandstoneBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::SANDSTONE),',
        '    {std::string("##"), std::string("##"), \'#\', Block::SAND});',
    ]),
    ("WorkbenchBlock.cpp", "WorkbenchBlock", [
        'recipeManager.addShapedRecipe(ItemStack(Block::CRAFTING_TABLE),',
        '    {std::string("##"), std::string("##"), \'#\', Block::PLANKS});',
    ]),
]

for fname, cls, lines in BLOCK_RECIPES:
    path = BLOCK / fname
    if path.exists():
        add_block_register_recipes(path, cls, "\n".join(f"    {l}" for l in lines))
    else:
        print(f"MISSING BLOCK {path}")

print("Done block/item migration script.")
