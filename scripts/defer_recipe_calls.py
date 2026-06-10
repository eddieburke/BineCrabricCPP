#!/usr/bin/env python3
"""Remove inline registerRecipes calls from registerClass (Registry defers them)."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1] / "src"

PATTERN = re.compile(
    r"\n[ \t]*registerRecipes\(recipe::CraftingRecipeManager::getInstance\(\)\);\n?",
    re.MULTILINE,
)

SMELT_LINE = re.compile(
    r"\n[ \t]*recipe::SmeltingRecipeManager::instance\(\)\.addRecipe\([^\n]+\);\n?",
    re.MULTILINE,
)

count = 0
for path in ROOT.rglob("*.cpp"):
    text = path.read_text(encoding="utf-8")
    new = PATTERN.sub("\n", text)
    if new != text:
        path.write_text(new, encoding="utf-8")
        count += 1
        print(f"crafting defer: {path.relative_to(ROOT.parent)}")

print(f"Updated {count} files for deferred crafting recipes.")
