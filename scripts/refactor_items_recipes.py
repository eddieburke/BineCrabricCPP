#!/usr/bin/env python3
"""Generate ItemIds, tool/sword leaves, replace Item::NAME references."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "net" / "minecraft"

# rawId -> (CppName, translationKey suffix, texX, texY) for simple Item leaves
SIMPLE_ITEMS = {
    8: ("DiamondItem", "emerald", 7, 3),
    9: ("IronIngotItem", "ingotIron", 7, 1),
    10: ("GoldIngotItem", "ingotGold", 7, 2),
    24: ("StickItem", "stick", 5, 3),
    25: ("BowlItem", "bowl", 7, 4),
    31: ("StringItem", "string", 8, 0),
    32: ("FeatherItem", "feather", 8, 1),
    33: ("GunpowderItem", "sulphur", 8, 2),
    40: ("WheatItem", "wheat", 9, 1),
    62: ("FlintItem", "flint", 6, 0),
    78: ("LeatherItem", "leather", 7, 6),
    80: ("BrickItem", "brick", 6, 1),
    81: ("ClayItem", "clay", 9, 3),
    83: ("PaperItem", "paper", 10, 3),
    84: ("BookItem", "book", 11, 3),
    85: ("SlimeballItem", "slimeball", 14, 1),
    89: ("CompassItem", "compass", 6, 3),
    91: ("ClockItem", "clock", 6, 4),
    92: ("GlowstoneDustItem", "yellowDust", 9, 4),
    96: ("BoneItem", "bone", 12, 1),
    97: ("SugarItem", "sugar", 13, 0),
}

SWORD_LEAVES = [
    (11, "IronSwordItem", "Iron", "swordIron", 2, 4, "X", "X", "#", "IronIngot"),
    (12, "WoodenSwordItem", "Wood", "swordWood", 0, 4, "X", "X", "#", "Planks"),
    (16, "StoneSwordItem", "Stone", "swordStone", 1, 4, "X", "X", "#", "Cobblestone"),
    (20, "DiamondSwordItem", "Diamond", "swordDiamond", 3, 4, "X", "X", "#", "DiamondItem"),
    (27, "GoldenSwordItem", "Gold", "swordGold", 4, 4, "X", "X", "#", "GoldIngotItem"),
]

PICKAXE_LEAVES = [
    (1, "IronPickaxeItem", "Iron", "pickaxeIron", 2, 6, "IronIngot"),
    (14, "WoodenPickaxeItem", "Wood", "pickaxeWood", 0, 6, "Planks"),
    (18, "StonePickaxeItem", "Stone", "pickaxeStone", 1, 6, "Cobblestone"),
    (22, "DiamondPickaxeItem", "Diamond", "pickaxeDiamond", 3, 6, "DiamondItem"),
    (29, "GoldenPickaxeItem", "Gold", "pickaxeGold", 4, 6, "GoldIngotItem"),
]

SHOVEL_LEAVES = [
    (0, "IronShovelItem", "Iron", "shovelIron", 2, 5, "IronIngot"),
    (13, "WoodenShovelItem", "Wood", "shovelWood", 0, 5, "Planks"),
    (17, "StoneShovelItem", "Stone", "shovelStone", 1, 5, "Cobblestone"),
    (21, "DiamondShovelItem", "Diamond", "shovelDiamond", 3, 5, "DiamondItem"),
    (28, "GoldenShovelItem", "Gold", "shovelGold", 4, 5, "GoldIngotItem"),
]

AXE_LEAVES = [
    (2, "IronAxeItem", "Iron", "hatchetIron", 2, 7, "IronIngot"),
    (15, "WoodenAxeItem", "Wood", "hatchetWood", 0, 7, "Planks"),
    (19, "StoneAxeItem", "Stone", "hatchetStone", 1, 7, "Cobblestone"),
    (23, "DiamondAxeItem", "Diamond", "hatchetDiamond", 3, 7, "DiamondItem"),
    (30, "GoldenAxeItem", "Gold", "hatchetGold", 4, 7, "GoldIngotItem"),
]

HOE_LEAVES = [
    (34, "WoodenHoeItem", "Wood", "hoeWood", 0, 8, "Planks"),
    (35, "StoneHoeItem", "Stone", "hoeStone", 1, 8, "Cobblestone"),
    (36, "IronHoeItem", "Iron", "hoeIron", 2, 8, "IronIngot"),
    (37, "DiamondHoeItem", "Diamond", "hoeDiamond", 3, 8, "DiamondItem"),
    (38, "GoldenHoeItem", "Gold", "hoeGold", 4, 8, "GoldIngotItem"),
]

# Item.java static name -> raw id
ITEM_NAME_TO_RAW = {
    "IRON_SHOVEL": 0, "IRON_PICKAXE": 1, "IRON_AXE": 2, "FLINT_AND_STEEL": 3,
    "APPLE": 4, "BOW": 5, "ARROW": 6, "COAL": 7, "DIAMOND": 8,
    "IRON_INGOT": 9, "GOLD_INGOT": 10, "IRON_SWORD": 11, "WOODEN_SWORD": 12,
    "WOODEN_SHOVEL": 13, "WOODEN_PICKAXE": 14, "WOODEN_AXE": 15,
    "STONE_SWORD": 16, "STONE_SHOVEL": 17, "STONE_PICKAXE": 18, "STONE_AXE": 19,
    "DIAMOND_SWORD": 20, "DIAMOND_SHOVEL": 21, "DIAMOND_PICKAXE": 22,
    "DIAMOND_AXE": 23, "STICK": 24, "BOWL": 25, "MUSHROOM_STEW": 26,
    "GOLDEN_SWORD": 27, "GOLDEN_SHOVEL": 28, "GOLDEN_PICKAXE": 29,
    "GOLDEN_AXE": 30, "STRING": 31, "FEATHER": 32, "GUNPOWDER": 33,
    "WOODEN_HOE": 34, "STONE_HOE": 35, "IRON_HOE": 36, "DIAMOND_HOE": 37,
    "GOLDEN_HOE": 38, "SEEDS": 39, "WHEAT": 40, "BREAD": 41,
    "LEATHER_HELMET": 42, "LEATHER_CHESTPLATE": 43, "LEATHER_LEGGINGS": 44,
    "LEATHER_BOOTS": 45, "CHAIN_HELMET": 46, "CHAIN_CHESTPLATE": 47,
    "CHAIN_LEGGINGS": 48, "CHAIN_BOOTS": 49, "IRON_HELMET": 50,
    "IRON_CHESTPLATE": 51, "IRON_LEGGINGS": 52, "IRON_BOOTS": 53,
    "DIAMOND_HELMET": 54, "DIAMOND_CHESTPLATE": 55, "DIAMOND_LEGGINGS": 56,
    "DIAMOND_BOOTS": 57, "GOLDEN_HELMET": 58, "GOLDEN_CHESTPLATE": 59,
    "GOLDEN_LEGGINGS": 60, "GOLDEN_BOOTS": 61, "FLINT": 62,
    "RAW_PORKCHOP": 63, "COOKED_PORKCHOP": 64, "PAINTING": 65,
    "GOLDEN_APPLE": 66, "SIGN": 67, "WOODEN_DOOR": 68, "BUCKET": 69,
    "WATER_BUCKET": 70, "LAVA_BUCKET": 71, "MINECART": 72, "SADDLE": 73,
    "IRON_DOOR": 74, "REDSTONE": 75, "SNOWBALL": 76, "BOAT": 77,
    "LEATHER": 78, "MILK_BUCKET": 79, "BRICK": 80, "CLAY": 81,
    "SUGAR_CANE": 82, "PAPER": 83, "BOOK": 84, "SLIMEBALL": 85,
    "CHEST_MINECART": 86, "FURNACE_MINECART": 87, "EGG": 88, "COMPASS": 89,
    "FISHING_ROD": 90, "CLOCK": 91, "GLOWSTONE_DUST": 92, "RAW_FISH": 93,
    "COOKED_FISH": 94, "DYE": 95, "BONE": 96, "SUGAR": 97, "CAKE": 98,
    "BED": 99, "REPEATER": 100, "COOKIE": 101, "MAP": 102, "SHEARS": 103,
    "RECORD_THIRTEEN": 2000, "RECORD_CAT": 2001,
}

RAW_TO_PASCAL = {v: k.title().replace("_", "") for k, v in ITEM_NAME_TO_RAW.items()}
# Fix multi-word: IRON_SHOVEL -> IronShovel
def to_pascal(snake: str) -> str:
    parts = snake.lower().split("_")
    return "".join(p.capitalize() for p in parts)

RAW_TO_PASCAL = {v: to_pascal(k) for k, v in ITEM_NAME_TO_RAW.items()}


def write_item_ids() -> None:
    lines = [
        "#pragma once",
        "",
        "namespace net::minecraft::itemids {",
        "",
    ]
    for name, raw in sorted(ITEM_NAME_TO_RAW.items(), key=lambda x: x[1]):
        lines.append(f"inline constexpr int {to_pascal(name)} = {raw};")
    lines.extend(["", "} // namespace net::minecraft::itemids", ""])
    path = SRC / "item" / "ItemIds.hpp"
    path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {path}")


def material_arg(name: str) -> str:
    return f"ToolMaterial::{name}"


def mat_ingredient(ing: str) -> str:
    if ing == "Planks":
        return "Block::PLANKS"
    if ing == "Cobblestone":
        return "Block::COBBLESTONE"
    if ing.endswith("Item"):
        return f"Item::byRawId(itemids::{ing.replace('Item', '')})"
    if ing == "IronIngot":
        return "Item::byRawId(itemids::IronIngot)"
    if ing == "GoldIngotItem":
        return "Item::byRawId(itemids::GoldIngot)"
    return f"Item::byRawId(itemids::{ing})"


def write_tool_leaf(parent: str, entries: list, weapon: bool = False) -> None:
    subdir = "weapon" if weapon else "tool"
    out_dir = SRC / "item" / subdir
    out_dir.mkdir(parents=True, exist_ok=True)

    for raw, cls, mat, trans_key, tx, ty, *rest in entries:
        hpp = out_dir / f"{cls}.hpp"
        cpp = out_dir / f"{cls}.cpp"

        hpp.write_text(f"""#pragma once

#include "net/minecraft/item/{parent}.hpp"

namespace net::minecraft::recipe {{
class CraftingRecipeManager;
}} // namespace net::minecraft::recipe

namespace net::minecraft::item {{

class {cls} : public {parent} {{
public:
    static constexpr bool kRegisters = true;
    static constexpr int kRawId = {raw};

    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    {cls}();
}};

}} // namespace net::minecraft::item
""", encoding="utf-8")

        if weapon:
            p1, p2, p3, ing = rest
            recipe_body = f"""    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(kRawId)),
        {{std::string("{p1}"), std::string("{p2}"), std::string("{p3}"), '#', Item::byRawId(itemids::Stick), 'X', {mat_ingredient(ing)}}});"""
        else:
            ing = rest[0]
            recipe_body = f"""    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(kRawId)),
        {{std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(itemids::Stick), 'X', {mat_ingredient(ing)}}});"""

        cpp.write_text(f"""#include "net/minecraft/item/{subdir}/{cls}.hpp"

#include "net/minecraft/item/ItemIds.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>

namespace net::minecraft::item {{

{cls}::{cls}() : {parent}({raw}, {material_arg(mat)}) {{}}

void {cls}::registerClass()
{{
    static {cls} instance;
    instance.setTexturePosition({tx}, {ty})->setTranslationKey("{trans_key}");
}}

void {cls}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{{
{recipe_body}
}}

struct {cls}RecipeRegistrar {{
    static void registerClass() {{ {cls}::registerRecipes(recipe::CraftingRecipeManager::getInstance()); }}
}};

static registry::RegisterItem<{cls}> s_itemReg;
static registry::RegisterCustom<{cls}RecipeRegistrar> s_recipeReg(
    registry::kCraftingRecipeRegistrarBase + {cls}::kRawId);

}} // namespace net::minecraft::item
""", encoding="utf-8")


def write_simple_items() -> None:
    out_dir = SRC / "item" / "material"
    out_dir.mkdir(parents=True, exist_ok=True)
    for raw, (cls, trans_key, tx, ty) in SIMPLE_ITEMS.items():
        hpp = out_dir / f"{cls}.hpp"
        cpp = out_dir / f"{cls}.cpp"
        hpp.write_text(f"""#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {{

class {cls} : public Item {{
public:
    static constexpr bool kRegisters = true;
    static constexpr int kRawId = {raw};
    static void registerClass();
    explicit {cls}();
}};

}} // namespace net::minecraft::item
""", encoding="utf-8")
        cpp.write_text(f"""#include "net/minecraft/item/material/{cls}.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {{

{cls}::{cls}() : Item(kRawId) {{}}

void {cls}::registerClass()
{{
    static {cls} instance;
    instance.setTexturePosition({tx}, {ty})->setTranslationKey("{trans_key}");
}}

static registry::RegisterItem<{cls}> s_reg;

}} // namespace net::minecraft::item
""", encoding="utf-8")


def replace_item_refs() -> None:
    pattern = re.compile(r"\bItem::([A-Z][A-Z0-9_]*)\b")
    skip = {"ITEMS", "random", "byRawId", "byId", "registerInItemsArray"}

    for path in SRC.rglob("*"):
        if path.suffix not in {".cpp", ".hpp"}:
            continue
        if path.name in {"ItemIds.hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        changed = False

        def repl(m: re.Match) -> str:
            nonlocal changed
            name = m.group(1)
            if name in skip:
                return m.group(0)
            if name not in ITEM_NAME_TO_RAW:
                return m.group(0)
            changed = True
            return f"Item::byRawId(itemids::{to_pascal(name)})"

        new_text = pattern.sub(repl, text)
        if changed and '#include "net/minecraft/item/ItemIds.hpp"' not in new_text:
            if "Item::byRawId(itemids::" in new_text:
                if '#include "net/minecraft/item/Item.hpp"' in new_text:
                    new_text = new_text.replace(
                        '#include "net/minecraft/item/Item.hpp"',
                        '#include "net/minecraft/item/Item.hpp"\n#include "net/minecraft/item/ItemIds.hpp"',
                        1,
                    )
                elif '#include "net/minecraft/block/Block.hpp"' in new_text:
                    new_text = new_text.replace(
                        '#include "net/minecraft/block/Block.hpp"',
                        '#include "net/minecraft/block/Block.hpp"\n#include "net/minecraft/item/ItemIds.hpp"',
                        1,
                    )
        if changed:
            path.write_text(new_text, encoding="utf-8")
            print(f"Updated refs in {path.relative_to(ROOT)}")


def main() -> None:
    write_item_ids()
    write_simple_items()
    write_tool_leaf("SwordItem", SWORD_LEAVES, weapon=True)
    write_tool_leaf("PickaxeItem", PICKAXE_LEAVES)
    write_tool_leaf("ShovelItem", SHOVEL_LEAVES)
    write_tool_leaf("AxeItem", AXE_LEAVES)
    write_tool_leaf("HoeItem", HOE_LEAVES)
    replace_item_refs()
    print("Done.")


if __name__ == "__main__":
    main()
