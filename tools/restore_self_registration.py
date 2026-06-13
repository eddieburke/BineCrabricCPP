#!/usr/bin/env python3
"""Restore per-TU self-registration as terse MC_REGISTER_* macros.

Reads `git diff` for removed Register{Block,Item,Entity}<T> statics, then
appends the matching MC_REGISTER_*(T) macro to each file (before the closing
namespace line, where the old static lived). Skips files that no longer exist
(header-only registrars) and files that already carry the macro.
"""
import os
import re
import subprocess
import sys

NATIVE = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
DIRS = ["src/net/minecraft/block", "src/net/minecraft/item", "src/net/minecraft/entity"]
DEL_RE = re.compile(r'Register(Block|Item|Entity)<([A-Za-z0-9_]+)>')

def main():
    diff = subprocess.run(
        ["git", "diff", "-U0", "--"] + DIRS,
        cwd=NATIVE, capture_output=True, text=True).stdout
    mapping = []  # (relpath, kind, type)
    cur = None
    for line in diff.splitlines():
        if line.startswith("--- a/"):
            cur = line[6:]
        elif line.startswith("-") and not line.startswith("---"):
            m = DEL_RE.search(line)
            if m:
                mapping.append((cur, m.group(1), m.group(2)))

    inserted, skipped = 0, []
    for relpath, kind, typ in mapping:
        path = os.path.join(NATIVE, relpath)
        if not os.path.exists(path):
            skipped.append((relpath, typ, "file gone (header-only registrar)"))
            continue
        with open(path, encoding="utf-8") as fh:
            lines = fh.readlines()
        macro = "MC_REGISTER_%s(%s)\n" % (kind.upper(), typ)
        if any(macro.strip() in ln for ln in lines):
            skipped.append((relpath, typ, "already present"))
            continue
        # insert before the last closing-namespace line
        idx = None
        for i in range(len(lines) - 1, -1, -1):
            if lines[i].startswith("} // namespace"):
                idx = i
                break
        if idx is None:
            skipped.append((relpath, typ, "no closing namespace found"))
            continue
        lines.insert(idx, macro)
        with open(path, "w", encoding="utf-8") as fh:
            fh.writelines(lines)
        inserted += 1

    print("inserted: %d" % inserted)
    for s in skipped:
        print("skipped: %s <%s> — %s" % s)

if __name__ == "__main__":
    main()
