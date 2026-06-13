#!/usr/bin/env python3
"""Move per-file self-registration statics into central manifest files.

For every TU under block/, item/, entity/ that ends with a
`static registry::RegisterX<Type> ...;` self-registration, this:
  1. resolves the header that *defines* Type (has a k*Id constant),
  2. deletes the static line from the TU (leaving its real logic),
  3. emits one manifest per category listing the statics.

Types whose registrar struct is .cpp-local (no header, e.g. the *Registrar
wrappers) are left self-registering in place.
"""
import os
import re
import sys

SRC = os.path.join(os.path.dirname(__file__), "..", "src")
SRC = os.path.normpath(SRC)
CATS = {"Block": "block", "Item": "item", "Entity": "entity"}
STATIC_RE = re.compile(
    r'^.*\bRegister(Block|Item|Entity)<([A-Za-z0-9_]+)>\s+[A-Za-z0-9_]+;.*$')
ID_RE = re.compile(r'\bk(BlockId|RawId|EntityId)\b')

def rel(p):
    return os.path.relpath(p, SRC).replace(os.sep, "/")

def find_header(cat_dir, typ):
    """Header under cat_dir that defines `typ` (class + a k*Id constant)."""
    best = None
    for root, _, files in os.walk(os.path.join(SRC, "net", "minecraft", cat_dir)):
        for fn in files:
            if not fn.endswith(".hpp") or fn == "EntityForward.hpp":
                continue
            path = os.path.join(root, fn)
            with open(path, encoding="utf-8") as fh:
                text = fh.read()
            if re.search(r'\b(?:class|struct)\s+%s\b' % re.escape(typ), text) and ID_RE.search(text):
                # prefer a header whose basename matches the type, else first hit
                if fn[:-4].lower() == typ.lower():
                    return path
                best = best or path
    return best

def main():
    manifests = {c: [] for c in CATS}
    skipped = []
    stripped = 0
    for cat, cat_dir in CATS.items():
        base = os.path.join(SRC, "net", "minecraft", cat_dir)
        for root, _, files in os.walk(base):
            for fn in files:
                if not fn.endswith(".cpp"):
                    continue
                path = os.path.join(root, fn)
                with open(path, encoding="utf-8") as fh:
                    lines = fh.readlines()
                hit = None
                for i, ln in enumerate(lines):
                    m = STATIC_RE.match(ln)
                    if m and m.group(1) == cat:
                        hit = (i, m.group(2))
                        break
                if not hit:
                    continue
                idx, typ = hit
                hdr = find_header(cat_dir, typ)
                if hdr is None:
                    skipped.append((rel(path), typ))
                    continue
                del lines[idx]
                with open(path, "w", encoding="utf-8") as fh:
                    fh.writelines(lines)
                stripped += 1
                manifests[cat].append((typ, rel(hdr)))

    out_for = {
        "Block": ("block", "BlockRegistrations.cpp", "net::minecraft::block"),
        "Item": ("item", "ItemRegistrations.cpp", "net::minecraft::item"),
        "Entity": ("entity", "EntityRegistrations.cpp", "net::minecraft::entity"),
    }
    for cat, entries in manifests.items():
        cat_dir, fname, ns = out_for[cat]
        entries.sort()
        headers = sorted({h for _, h in entries})
        body = ['#include "net/minecraft/registry/Registry.hpp"', ""]
        body += ['#include "%s"' % h for h in headers]
        body += ["", "// Central self-registration manifest: instantiating each helper at",
                 "// static-init enqueues the type into its bootstrap phase (ordered by the",
                 "// type's id constant). Generated from the former per-TU autoReg statics.",
                 "namespace %s {" % ns, "namespace {", ""]
        for typ, _ in entries:
            body.append("registry::Register%s<%s> r_%s;" % (cat, typ, typ))
        body += ["", "} // namespace", "} // namespace %s" % ns, ""]
        with open(os.path.join(SRC, "net", "minecraft", cat_dir, fname), "w",
                  encoding="utf-8") as fh:
            fh.write("\n".join(body))
        print("WROTE %s/%s  (%d types)" % (cat_dir, fname, len(entries)))

    print("\nSTRIPPED statics: %d" % stripped)
    print("LEFT in place (no header / local registrar):")
    for f, t in skipped:
        print("   %s  <%s>" % (f, t))

if __name__ == "__main__":
    main()
