#!/usr/bin/env python3
"""Convert leaf item headers to kRawId + registerClass/registerRecipes; generate .cpp TUs."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ITEM = ROOT / "src" / "net" / "minecraft" / "item"

STATIC_TO_CLASS = {
    "IRON_SHOVEL": "IronShovelItem", "IRON_PICKAXE": "IronPickaxeItem", "IRON_AXE": "IronAxeItem",
    "FLINT_AND_STEEL": "FlintAndSteelItem", "APPLE": "AppleItem", "BOW": "BowItem",
    "ARROW": "ArrowItem", "COAL": "CoalItem", "DIAMOND": "DiamondItem",
    "IRON_INGOT": "IronIngotItem", "GOLD_INGOT": "GoldIngotItem",
    "IRON_SWORD": "IronSwordItem", "WOODEN_SWORD": "WoodenSwordItem",
    "WOODEN_SHOVEL": "WoodenShovelItem", "WOODEN_PICKAXE": "WoodenPickaxeItem",
    "WOODEN_AXE": "WoodenAxeItem", "STONE_SWORD": "StoneSwordItem",
    "STONE_SHOVEL": "StoneShovelItem", "STONE_PICKAXE": "StonePickaxeItem",
    "STONE_AXE": "StoneAxeItem", "DIAMOND_SWORD": "DiamondSwordItem",
    "DIAMOND_SHOVEL": "DiamondShovelItem", "DIAMOND_PICKAXE": "DiamondPickaxeItem",
    "DIAMOND_AXE": "DiamondAxeItem", "STICK": "StickItem", "BOWL": "BowlItem",
    "MUSHROOM_STEW": "MushroomStewItem", "GOLDEN_SWORD": "GoldenSwordItem",
    "GOLDEN_SHOVEL": "GoldenShovelItem", "GOLDEN_PICKAXE": "GoldenPickaxeItem",
    "GOLDEN_AXE": "GoldenAxeItem", "STRING": "StringItem", "FEATHER": "FeatherItem",
    "GUNPOWDER": "GunpowderItem", "WOODEN_HOE": "WoodenHoeItem", "STONE_HOE": "StoneHoeItem",
    "IRON_HOE": "IronHoeItem", "DIAMOND_HOE": "DiamondHoeItem", "GOLDEN_HOE": "GoldenHoeItem",
    "SEEDS": "SeedsItem", "WHEAT": "WheatItem", "BREAD": "BreadItem",
    "LEATHER_HELMET": "LeatherHelmetItem", "LEATHER_CHESTPLATE": "LeatherChestplateItem",
    "LEATHER_LEGGINGS": "LeatherLeggingsItem", "LEATHER_BOOTS": "LeatherBootsItem",
    "CHAIN_HELMET": "ChainHelmetItem", "CHAIN_CHESTPLATE": "ChainChestplateItem",
    "CHAIN_LEGGINGS": "ChainLeggingsItem", "CHAIN_BOOTS": "ChainBootsItem",
    "IRON_HELMET": "IronHelmetItem", "IRON_CHESTPLATE": "IronChestplateItem",
    "IRON_LEGGINGS": "IronLeggingsItem", "IRON_BOOTS": "IronBootsItem",
    "DIAMOND_HELMET": "DiamondHelmetItem", "DIAMOND_CHESTPLATE": "DiamondChestplateItem",
    "DIAMOND_LEGGINGS": "DiamondLeggingsItem", "DIAMOND_BOOTS": "DiamondBootsItem",
    "GOLDEN_HELMET": "GoldenHelmetItem", "GOLDEN_CHESTPLATE": "GoldenChestplateItem",
    "GOLDEN_LEGGINGS": "GoldenLeggingsItem", "GOLDEN_BOOTS": "GoldenBootsItem",
    "FLINT": "FlintItem", "RAW_PORKCHOP": "RawPorkchopItem", "COOKED_PORKCHOP": "CookedPorkchopItem",
    "PAINTING": "PaintingItem", "GOLDEN_APPLE": "GoldenAppleItem", "SIGN": "SignItem",
    "WOODEN_DOOR": "WoodenDoorItem", "BUCKET": "BucketItem", "WATER_BUCKET": "WaterBucketItem",
    "LAVA_BUCKET": "LavaBucketItem", "MINECART": "MinecartItem", "SADDLE": "SaddleItem",
    "IRON_DOOR": "IronDoorItem", "REDSTONE": "RedstoneItem", "SNOWBALL": "SnowballItem",
    "BOAT": "BoatItem", "LEATHER": "LeatherItem", "MILK_BUCKET": "MilkBucketItem",
    "BRICK": "BrickItem", "CLAY": "ClayItem", "SUGAR_CANE": "SugarCaneItem",
    "PAPER": "PaperItem", "BOOK": "BookItem", "SLIMEBALL": "SlimeballItem",
    "CHEST_MINECART": "ChestMinecartItem", "FURNACE_MINECART": "FurnaceMinecartItem",
    "EGG": "EggItem", "COMPASS": "CompassItem", "FISHING_ROD": "FishingRodItem",
    "CLOCK": "ClockItem", "GLOWSTONE_DUST": "GlowstoneDustItem", "RAW_FISH": "RawFishItem",
    "COOKED_FISH": "CookedFishItem", "DYE": "DyeItem", "BONE": "BoneItem", "SUGAR": "SugarItem",
    "CAKE": "CakeItem", "BED": "BedItem", "REPEATER": "RepeaterItem", "COOKIE": "CookieItem",
    "MAP": "MapItem", "SHEARS": "ShearsItem", "RECORD_THIRTEEN": "RecordThirteenItem",
    "RECORD_CAT": "RecordCatItem",
}

SKIP_LEAF = {
    ITEM / "misc" / "bow.hpp",
    ITEM / "transport" / "minecart.hpp",
    ITEM / "food" / "mushroom_stew.hpp",
    ITEM / "misc" / "bed.hpp",
    ITEM / "misc" / "map.hpp",
    ITEM / "misc" / "dye.hpp",
    ITEM / "misc" / "shears.hpp",
    ITEM / "misc" / "fishing_rod.hpp",
    ITEM / "misc" / "flint_and_steel.hpp",
    ITEM / "misc" / "bucket.hpp",
    ITEM / "misc" / "water_bucket.hpp",
    ITEM / "misc" / "lava_bucket.hpp",
    ITEM / "misc" / "milk_bucket.hpp",
    ITEM / "misc" / "coal.hpp",
    ITEM / "misc" / "sign.hpp",
    ITEM / "misc" / "wooden_door.hpp",
    ITEM / "misc" / "iron_door.hpp",
    ITEM / "transport" / "boat.hpp",
    ITEM / "transport" / "saddle.hpp",
    ITEM / "misc" / "redstone.hpp",
    ITEM / "misc" / "snowball.hpp",
    ITEM / "misc" / "painting.hpp",
    ITEM / "misc" / "egg.hpp",
    ITEM / "misc" / "record_thirteen.hpp",
    ITEM / "misc" / "record_cat.hpp",
    ITEM / "misc" / "repeater.hpp",
    ITEM / "food" / "cake.hpp",
    ITEM / "food" / "cookie.hpp",
}

CLASS_RE = re.compile(
    r"class\s+(\w+)\s*:\s*public\s+(?:item::|net::minecraft::item::)?(\w+)",
)
CTOR_RE = re.compile(
    r":\s*(?:item::|net::minecraft::item::)?(\w+)\(([^)]*)\)",
)
TEX_RE = re.compile(r"setTexturePosition\((\d+),\s*(\d+)\)")
KEY_RE = re.compile(r'setTranslationKey\("([^"]+)"\)')
HANDHELD_RE = re.compile(r"setHandheld\(\)")
MAXCOUNT_RE = re.compile(r"setMaxCount\((\d+)\)")

TOOL_MATERIAL_INGREDIENT = {
    "ToolMaterial::Wood": ("Block::PLANKS", None),
    "ToolMaterial::Stone": ("Block::COBBLESTONE", None),
    "ToolMaterial::Iron": (None, "IronIngotItem"),
    "ToolMaterial::Diamond": (None, "DiamondItem"),
    "ToolMaterial::Gold": (None, "GoldIngotItem"),
}

TOOL_PATTERNS = {
    "SwordItem": ["X", "X", "#"],
    "PickaxeItem": ["XXX", " # ", " # "],
    "ShovelItem": ["X", "#", "#"],
    "AxeItem": ["XX", "X#", " #"],
    "HoeItem": ["XX", " # ", " # "],
}

ARMOR_MATERIAL = {
    "helmetCloth": "Item::byRawId(LeatherItem::kRawId)",
    "chestplateCloth": "Item::byRawId(LeatherItem::kRawId)",
    "leggingsCloth": "Item::byRawId(LeatherItem::kRawId)",
    "bootsCloth": "Item::byRawId(LeatherItem::kRawId)",
    "helmetChain": "Block::FIRE",
    "chestplateChain": "Block::FIRE",
    "leggingsChain": "Block::FIRE",
    "bootsChain": "Block::FIRE",
    "helmetIron": "Item::byRawId(IronIngotItem::kRawId)",
    "chestplateIron": "Item::byRawId(IronIngotItem::kRawId)",
    "leggingsIron": "Item::byRawId(IronIngotItem::kRawId)",
    "bootsIron": "Item::byRawId(IronIngotItem::kRawId)",
    "helmetDiamond": "Item::byRawId(DiamondItem::kRawId)",
    "chestplateDiamond": "Item::byRawId(DiamondItem::kRawId)",
    "leggingsDiamond": "Item::byRawId(DiamondItem::kRawId)",
    "bootsDiamond": "Item::byRawId(DiamondItem::kRawId)",
    "helmetGold": "Item::byRawId(GoldIngotItem::kRawId)",
    "chestplateGold": "Item::byRawId(GoldIngotItem::kRawId)",
    "leggingsGold": "Item::byRawId(GoldIngotItem::kRawId)",
    "bootsGold": "Item::byRawId(GoldIngotItem::kRawId)",
}

ARMOR_PATTERN = {
    "helmet": ['{"XXX", "X X"}', "X"],
    "chestplate": ['{"X X", "XXX", "XXX"}', "X"],
    "leggings": ['{"XXX", "X X", "X X"}', "X"],
    "boots": ['{"X X", "X X"}', "X"],
}


def parse_leaf(path: Path) -> dict | None:
    text = path.read_text(encoding="utf-8")
    cm = CLASS_RE.search(text)
    if not cm:
        return None
    class_name, parent = cm.group(1), cm.group(2)
    tcm = CTOR_RE.search(text)
    if not tcm:
        return None
    ctor_class, ctor_args = tcm.group(1), tcm.group(2).strip()
    tex = TEX_RE.search(text)
    key = KEY_RE.search(text)
    raw_id = None
    if ctor_class in (
        "Item", "FoodItem", "SwordItem", "PickaxeItem", "ShovelItem", "AxeItem", "HoeItem",
        "ArmorItem", "MinecartItem", "MushroomStewItem", "FlintAndSteel", "BowItem",
        "BucketItem", "DoorItem", "SecondaryBlockItem", "SeedsItem", "StackableFoodItem",
        "MusicDiscItem", "DyeItem", "RedstoneItem", "PaintingItem", "FishingRodItem",
        "ShearsItem", "BoatItem", "SaddleItem", "SnowballItem", "SignItem", "EggItem",
    ):
        first = ctor_args.split(",")[0].strip()
        if first.isdigit():
            raw_id = int(first)
    if raw_id is None:
        id_m = re.search(r"static constexpr int ID = (\d+)", text)
        if id_m:
            raw_id = int(id_m.group(1)) - 256
    if raw_id is None:
        return None
    max_count_m = MAXCOUNT_RE.search(text)
    return {
        "path": path,
        "class_name": class_name,
        "parent": parent,
        "raw_id": raw_id,
        "ctor_class": ctor_class,
        "ctor_args": ctor_args,
        "tex": (int(tex.group(1)), int(tex.group(2))) if tex else None,
        "key": key.group(1) if key else "",
        "handheld": HANDHELD_RE.search(text) is not None,
        "max_count": max_count_m.group(1) if max_count_m else None,
    }


def parent_include(parent: str) -> str:
    return f"net/minecraft/item/{parent}.hpp"


def write_header(info: dict) -> None:
    parent = info["parent"]
    cls = info["class_name"]
    content = f"""#pragma once

#include "{parent_include(parent)}"

namespace net::minecraft::recipe {{
class CraftingRecipeManager;
}} // namespace net::minecraft::recipe

namespace net::minecraft::item {{

class {cls} : public {parent} {{
public:
    static constexpr bool kRegisters = true;
    static constexpr int kRawId = {info['raw_id']};

    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    {cls}();
}};

}} // namespace net::minecraft::item
"""
    info["path"].write_text(content, encoding="utf-8")


def ctor_body(info: dict) -> str:
    cc, args = info["ctor_class"], info["ctor_args"]
    cls = info["class_name"]
    if cc in ("SwordItem", "PickaxeItem", "ShovelItem", "AxeItem", "HoeItem"):
        mat = args.split(",")[-1].strip()
        return f"{cls}::{cls}() : {cc}(kRawId, {mat}) {{}}"
    if cc == "FoodItem":
        return f"{cls}::{cls}() : FoodItem(kRawId, {args.split(',', 1)[1].strip()}) {{}}"
    if cc == "ArmorItem":
        rest = args.split(",", 1)[1].strip()
        return f"{cls}::{cls}() : ArmorItem(kRawId, {rest}) {{}}"
    if cc == "MinecartItem":
        typ = args.split(",")[1].strip()
        return f"{cls}::{cls}() : MinecartItem(kRawId, {typ}) {{}}"
    if cc == "Item":
        return f"{cls}::{cls}() : Item(kRawId, RegistrationMode::Deferred) {{}}"
    parts = args.split(",")
    if parts and parts[0].strip().isdigit():
        parts[0] = "kRawId"
        args = ", ".join(parts)
    return f"{cls}::{cls}() : {cc}({args}) {{}}"


def register_setup(info: dict) -> str:
    lines = []
    if info["tex"]:
        lines.append(f"    instance.setTexturePosition({info['tex'][0]}, {info['tex'][1]});")
    if info["handheld"]:
        lines.append("    instance.setHandheld();")
    if info["max_count"]:
        lines.append(f"    instance.setMaxCount({info['max_count']});")
    if info["key"]:
        lines.append(f'    instance.setTranslationKey("{info["key"]}");')
    if not lines:
        lines.append("    (void)instance;")
    return "\n".join(lines)


def item_ref(cls: str) -> str:
    return f"Item::byRawId({cls}::kRawId)"


def material_x(parent: str, mat: str) -> str:
    block, ing = TOOL_MATERIAL_INGREDIENT.get(mat, (None, None))
    if ing:
        return f"Item::byRawId({ing}::kRawId)"
    return block or "Block::PLANKS"


def shaped_recipe(cls: str, pattern: list[str], x_ref: str, count: int = 1) -> str:
    stack = f"ItemStack({item_ref(cls)}{', ' + str(count) if count != 1 else ''})"
    pat = ", ".join(f'std::string("{line}")' for line in pattern)
    return f"""    recipeManager.addShapedRecipe({stack},
        {{{pat}, '#', Item::byRawId(StickItem::kRawId), 'X', {x_ref}}});"""


def recipe_body(info: dict) -> str:
    cls = info["class_name"]
    parent = info["parent"]
    key = info["key"]
    if parent in TOOL_PATTERNS:
        mat = info["ctor_args"].split(",")[-1].strip()
        return shaped_recipe(cls, TOOL_PATTERNS[parent], material_x(parent, mat))
    if parent == "ArmorItem":
        slot = (
            "helmet" if "helmet" in key
            else "chestplate" if "chestplate" in key
            else "leggings" if "leggings" in key
            else "boots"
        )
        pat, sym = ARMOR_PATTERN[slot]
        mat = ARMOR_MATERIAL.get(key, "Item::byRawId(IronIngotItem::kRawId)")
        return f"""    recipeManager.addShapedRecipe(ItemStack({item_ref(cls)}),
        {{{pat}, '{sym}', {mat}}});"""
    if cls == "BreadItem":
        return f"""    recipeManager.addShapedRecipe(ItemStack({item_ref(cls)}),
        {{std::string("###"), '#', Item::byRawId(WheatItem::kRawId)}});"""
    if cls == "StickItem":
        return f"""    recipeManager.addShapedRecipe(ItemStack({item_ref(cls)}, 4),
        {{std::string("#"), std::string("#"), '#', Block::PLANKS}});"""
    if cls == "BowlItem":
        return f"""    recipeManager.addShapedRecipe(ItemStack({item_ref(cls)}, 4),
        {{std::string("# #"), std::string(" # "), '#', Block::PLANKS}});"""
    return "    (void)recipeManager;"


def recipe_includes(info: dict) -> list[str]:
    incs = ['#include "net/minecraft/item/misc/stick.hpp"']
    parent = info["parent"]
    if parent in TOOL_PATTERNS:
        incs.extend([
            '#include "net/minecraft/block/Block.hpp"',
            '#include "net/minecraft/item/misc/iron_ingot.hpp"',
            '#include "net/minecraft/item/misc/diamond.hpp"',
            '#include "net/minecraft/item/misc/gold_ingot.hpp"',
        ])
    if parent == "ArmorItem":
        incs.extend([
            '#include "net/minecraft/block/Block.hpp"',
            '#include "net/minecraft/item/misc/leather.hpp"',
            '#include "net/minecraft/item/misc/iron_ingot.hpp"',
            '#include "net/minecraft/item/misc/diamond.hpp"',
            '#include "net/minecraft/item/misc/gold_ingot.hpp"',
        ])
    if cls := info["class_name"]:
        if cls == "BreadItem":
            incs.append('#include "net/minecraft/item/food/wheat.hpp"')
    return incs


def write_cpp(info: dict) -> None:
    cls = info["class_name"]
    rel = info["path"].relative_to(ITEM).as_posix()
    inc = f"net/minecraft/item/{rel}"
    extra = "\n".join(recipe_includes(info))
    content = f"""#include "{inc}"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

{extra}

namespace net::minecraft::item {{

{ctor_body(info)}

void {cls}::registerClass()
{{
    static {cls} instance;
{register_setup(info)}
    Item::registerInItemsArray(&instance);
}}

void {cls}::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{{
{recipe_body(info)}
}}

struct {cls}RecipeRegistrar {{
    static void registerClass() {{ {cls}::registerRecipes(recipe::CraftingRecipeManager::getInstance()); }}
}};

static registry::RegisterItem<{cls}> s_itemReg;
static registry::RegisterCustom<{cls}RecipeRegistrar> s_recipeReg(
    registry::kCraftingRecipeRegistrarBase + {cls}::kRawId);

}} // namespace net::minecraft::item
"""
    info["path"].with_suffix(".cpp").write_text(content, encoding="utf-8")


def replace_item_statics() -> None:
    src = ROOT / "src"
    for path in src.rglob("*"):
        if path.suffix not in {".cpp", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        orig = text
        for static, cls in STATIC_TO_CLASS.items():
            text = re.sub(rf"\bItem::{static}\b", f"Item::byRawId({cls}::kRawId)", text)
        if text != orig:
            path.write_text(text, encoding="utf-8")


def main() -> None:
    count = 0
    for hpp in ITEM.rglob("*.hpp"):
        if hpp.parent == ITEM:
            continue
        if hpp in SKIP_LEAF:
            continue
        info = parse_leaf(hpp)
        if not info:
            print(f"skip {hpp.relative_to(ITEM)}")
            continue
        write_header(info)
        write_cpp(info)
        count += 1
    replace_item_statics()
    print(f"Processed {count} leaf items")


if __name__ == "__main__":
    main()
