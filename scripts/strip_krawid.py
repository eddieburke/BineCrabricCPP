#!/usr/bin/env python3
"""Finish removing kRawId: build id map from Java Item.java, replace refs, inline recipes."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
JAVA_ITEM = ROOT.parent / "mcp" / "src" / "net" / "minecraft" / "item" / "Item.java"

ITEM_RAW_REF_RE = re.compile(
    r"(?:item::|::net::minecraft::item::)?(\w+(?:Item|FlintAndSteel))::kRawId"
)
KREG_RE = re.compile(r"\s*static\s+constexpr\s+bool\s+kRegisters\s*=\s*(?:true|false)\s*;\s*\n")
KRAW_LINE_RE = re.compile(r"\s*static\s+constexpr\s+int\s+kRawId\s*=\s*\d+\s*;\s*\n")
RECIPE_REGISTRAR_STRUCT_RE = re.compile(r"\nstruct \w+RecipeRegistrar \{[^}]+\};\s*\n", re.DOTALL)
RECIPE_REGISTER_CUSTOM_RE = re.compile(
    r"\nstatic registry::RegisterCustom<\w+RecipeRegistrar> s_recipeReg\([^)]+\);\s*\n"
)
REGISTER_CLASS_BODY_RE = re.compile(
    r"(void (\w+)::registerClass\(\)\s*\{)(.*?)(\n\})",
    re.DOTALL,
)
JAVA_LINE_RE = re.compile(
    r"public static Item (\w+) = new (?:\w+\()?(\d+)"
)
JAVA_TYPED_RE = re.compile(
    r"public static Item (\w+) = new (\w+)\((\d+)"
)
JAVA_CAST_RE = re.compile(
    r"public static \w+ (\w+) = (?:\([^)]+\))?new \w+\((\d+)"
)


def build_map_from_java() -> dict[str, int]:
    mapping: dict[str, int] = {}
    static_to_class = {
        "IRON_SHOVEL": "IronShovelItem", "IRON_PICKAXE": "IronPickaxeItem", "IRON_AXE": "IronAxeItem",
        "FLINT_AND_STEEL": "FlintAndSteel", "APPLE": "AppleItem", "BOW": "BowItem", "ARROW": "ArrowItem",
        "COAL": "CoalItem", "DIAMOND": "DiamondItem", "IRON_INGOT": "IronIngotItem",
        "GOLD_INGOT": "GoldIngotItem", "IRON_SWORD": "IronSwordItem", "WOODEN_SWORD": "WoodenSwordItem",
        "WOODEN_SHOVEL": "WoodenShovelItem", "WOODEN_PICKAXE": "WoodenPickaxeItem",
        "WOODEN_AXE": "WoodenAxeItem", "STONE_SWORD": "StoneSwordItem", "STONE_SHOVEL": "StoneShovelItem",
        "STONE_PICKAXE": "StonePickaxeItem", "STONE_AXE": "StoneAxeItem", "DIAMOND_SWORD": "DiamondSwordItem",
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
        "RECORD_13": "RecordThirteenItem", "RECORD_CAT": "RecordCatItem",
    }
    text = JAVA_ITEM.read_text(encoding="utf-8")
    for line in text.splitlines():
        cm = JAVA_CAST_RE.search(line)
        if cm:
            static_name, raw = cm.group(1), int(cm.group(2))
        else:
            tm = JAVA_TYPED_RE.search(line)
            if tm:
                static_name, raw = tm.group(1), int(tm.group(3))
            else:
                m = JAVA_LINE_RE.search(line)
                if not m:
                    continue
                static_name, raw = m.group(1), int(m.group(2))
        cls = static_to_class.get(static_name, static_name.title().replace("_", "") + "Item")
        mapping[cls] = raw
        if cls == "FlintAndSteel":
            mapping["FlintAndSteelItem"] = raw
        if cls.endswith("Item"):
            mapping[cls[:-4]] = raw
    # Extra aliases from stub headers
    mapping.setdefault("RecordThirteenItem", 2000)
    mapping.setdefault("RecordCatItem", 2001)
    mapping.setdefault("MapItem", 102)
    mapping.setdefault("ShearsItem", 103)
    mapping.setdefault("WaterBucketItem", 70)
    mapping.setdefault("LavaBucketItem", 71)
    mapping.setdefault("MilkBucketItem", 79)
    return mapping


def replace_raw_refs(text: str, mapping: dict[str, int]) -> str:
    def repl(match: re.Match[str]) -> str:
        name = match.group(1)
        if name in mapping:
            return str(mapping[name])
        if name.endswith("Item") and name[:-4] in mapping:
            return str(mapping[name[:-4]])
        raise KeyError(name)

    text = ITEM_RAW_REF_RE.sub(repl, text)
    text = re.sub(r"\bkRawId\b", lambda m: "0", text)  # leftover bare kRawId
    return text


def inline_recipe_registration(text: str) -> str:
    if "registerRecipes" not in text:
        return text
    text = RECIPE_REGISTRAR_STRUCT_RE.sub("\n", text)
    text = RECIPE_REGISTER_CUSTOM_RE.sub("\n", text)

    def patch(match: re.Match[str]) -> str:
        prefix, _, body, suffix = match.groups()
        if "registerRecipes(" in body or "registerInItemsArray" not in body:
            return match.group(0)
        body = body.rstrip() + "\n    registerRecipes(recipe::CraftingRecipeManager::getInstance());\n"
        return prefix + body + suffix

    return REGISTER_CLASS_BODY_RE.sub(patch, text)


def strip_header_constants(text: str) -> str:
    return KRAW_LINE_RE.sub("", KREG_RE.sub("\n", text))


def process(path: Path, mapping: dict[str, int]) -> bool:
    original = path.read_text(encoding="utf-8")
    text = original
    text = text.replace('#include "net/minecraft/item/VanillaItemKRawId.hpp"\n', "")
    if "kRawId" in text or "::kRawId" in text or "item::" in text and "Item::" in text:
        text = replace_raw_refs(text, mapping)
    if path.suffix == ".cpp":
        text = inline_recipe_registration(text)
    if path.suffix == ".hpp":
        text = strip_header_constants(text)
    if text != original:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    mapping = build_map_from_java()
    print(f"Java map: {len(mapping)} entries")
    n = 0
    for path in SRC.rglob("*"):
        if path.suffix in {".hpp", ".cpp"} and process(path, mapping):
            n += 1
    print(f"Updated {n} files")


if __name__ == "__main__":
    main()
