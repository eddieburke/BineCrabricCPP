#!/usr/bin/env python3
"""Fix RegisterItem missing ids; strip RecipeRegistrar boilerplate."""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from strip_krawid import build_map_from_java  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

RECIPE_REGISTRAR_STRUCT_RE = re.compile(r"\nstruct \w+RecipeRegistrar \{[^}]+\};\s*\n", re.DOTALL)
RECIPE_REGISTER_CUSTOM_RE = re.compile(
    r"\nstatic registry::RegisterCustom<\w+RecipeRegistrar> s_recipeReg\([^)]+\);\s*\n"
)
EMPTY_AUTO_REG_RE = re.compile(
    r"RegisterItem<(\w+)> autoReg;\s*(\}|// namespace)"
)
EMPTY_S_ITEM_REG_RE = re.compile(r"RegisterItem<(\w+)> s_itemReg;")


def fix_file(path: Path, mapping: dict[str, int]) -> bool:
    text = path.read_text(encoding="utf-8")
    original = text
    text = RECIPE_REGISTRAR_STRUCT_RE.sub("\n", text)
    text = RECIPE_REGISTER_CUSTOM_RE.sub("\n", text)

    def auto_fix(m: re.Match[str]) -> str:
        cls = m.group(1)
        raw = mapping.get(cls)
        if raw is None and cls.endswith("Item"):
            raw = mapping.get(cls[:-4] + "Item") or mapping.get(cls.replace("Item", ""))
        if raw is None:
            return m.group(0)
        return f"RegisterItem<{cls}> autoReg({raw}); {m.group(2)}"

    text = EMPTY_AUTO_REG_RE.sub(auto_fix, text)

    def sreg_fix(m: re.Match[str]) -> str:
        cls = m.group(1)
        raw = mapping.get(cls)
        if raw is None:
            return m.group(0)
        return f"RegisterItem<{cls}> s_itemReg({raw});"

    text = EMPTY_S_ITEM_REG_RE.sub(sreg_fix, text)
    if text != original:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    mapping = build_map_from_java()
    n = 0
    for path in SRC.rglob("*.cpp"):
        if fix_file(path, mapping):
            n += 1
    print(f"Fixed {n} cpp files")


if __name__ == "__main__":
    main()
