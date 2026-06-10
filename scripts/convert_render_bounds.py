# One-shot refactor: move updateBoundingBox math into const getRenderBounds.
# Run from native/src/net/minecraft/block. Delete after use.
import io
import os

os.chdir(os.path.join(os.path.dirname(__file__), "..", "src", "net", "minecraft", "block"))


def sub(path, old, new):
    s = io.open(path, encoding="utf-8", newline="").read()
    if old not in s and "\r\n" in s:
        old = old.replace("\n", "\r\n")
        new = new.replace("\n", "\r\n")
    if new.replace("\r\n", "\n") in s.replace("\r\n", "\n"):
        print("already converted", path)
        return
    assert old in s, f"pattern not found in {path}"
    s = s.replace(old, new, 1)
    io.open(path, "w", encoding="utf-8", newline="").write(s)
    print("ok", path)


def wrapper(cls):
    return (
        f"void {cls}::updateBoundingBox(const BlockView* blockView, int x, int y, int z)\n"
        "{\n"
        "    setBoundingBox(getRenderBounds(blockView, x, y, z));\n"
        "}\n"
        "\n"
    )


sub("BedBlock.cpp",
"""void BedBlock::updateBoundingBox(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/)
{
    setDefaultShape();
}
""",
wrapper("BedBlock") +
"""net::minecraft::Box BedBlock::getRenderBounds(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/) const
{
    return {0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f};
}
""")

sub("ButtonBlock.cpp",
"""void ButtonBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    const int face = meta & 7;
    const bool pressed = (meta & 8) != 0;
    constexpr float yMin = 0.375f;
    constexpr float yMax = 0.625f;
    constexpr float halfWidth = 0.1875f;
    const float depth = pressed ? 0.0625f : 0.125f;

    if (face == 1) {
        setBoundingBox(0.0f, yMin, 0.5f - halfWidth, depth, yMax, 0.5f + halfWidth);
    } else if (face == 2) {
        setBoundingBox(1.0f - depth, yMin, 0.5f - halfWidth, 1.0f, yMax, 0.5f + halfWidth);
    } else if (face == 3) {
        setBoundingBox(0.5f - halfWidth, yMin, 0.0f, 0.5f + halfWidth, yMax, depth);
    } else if (face == 4) {
        setBoundingBox(0.5f - halfWidth, yMin, 1.0f - depth, 0.5f + halfWidth, yMax, 1.0f);
    }
}
""",
wrapper("ButtonBlock") +
"""net::minecraft::Box ButtonBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    const int face = meta & 7;
    const bool pressed = (meta & 8) != 0;
    constexpr float yMin = 0.375f;
    constexpr float yMax = 0.625f;
    constexpr float halfWidth = 0.1875f;
    const float depth = pressed ? 0.0625f : 0.125f;

    if (face == 1) {
        return {0.0f, yMin, 0.5f - halfWidth, depth, yMax, 0.5f + halfWidth};
    }
    if (face == 2) {
        return {1.0f - depth, yMin, 0.5f - halfWidth, 1.0f, yMax, 0.5f + halfWidth};
    }
    if (face == 3) {
        return {0.5f - halfWidth, yMin, 0.0f, 0.5f + halfWidth, yMax, depth};
    }
    if (face == 4) {
        return {0.5f - halfWidth, yMin, 1.0f - depth, 0.5f + halfWidth, yMax, 1.0f};
    }
    return {minX, minY, minZ, maxX, maxY, maxZ};
}
""")

sub("LeverBlock.cpp",
"""void LeverBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int face = blockView != nullptr ? (blockView->getBlockMeta(x, y, z) & 7) : 0;
    constexpr float halfThickness = 0.1875f;
    if (face == 1) {
        setBoundingBox(0.0f, 0.2f, 0.5f - halfThickness, halfThickness * 2.0f, 0.8f, 0.5f + halfThickness);
    } else if (face == 2) {
        setBoundingBox(1.0f - halfThickness * 2.0f, 0.2f, 0.5f - halfThickness, 1.0f, 0.8f, 0.5f + halfThickness);
    } else if (face == 3) {
        setBoundingBox(0.5f - halfThickness, 0.2f, 0.0f, 0.5f + halfThickness, 0.8f, halfThickness * 2.0f);
    } else if (face == 4) {
        setBoundingBox(0.5f - halfThickness, 0.2f, 1.0f - halfThickness * 2.0f, 0.5f + halfThickness, 0.8f, 1.0f);
    } else {
        constexpr float halfWidth = 0.25f;
        setBoundingBox(0.5f - halfWidth, 0.0f, 0.5f - halfWidth, 0.5f + halfWidth, 0.6f, 0.5f + halfWidth);
    }
}
""",
wrapper("LeverBlock") +
"""net::minecraft::Box LeverBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int face = blockView != nullptr ? (blockView->getBlockMeta(x, y, z) & 7) : 0;
    constexpr float halfThickness = 0.1875f;
    if (face == 1) {
        return {0.0f, 0.2f, 0.5f - halfThickness, halfThickness * 2.0f, 0.8f, 0.5f + halfThickness};
    }
    if (face == 2) {
        return {1.0f - halfThickness * 2.0f, 0.2f, 0.5f - halfThickness, 1.0f, 0.8f, 0.5f + halfThickness};
    }
    if (face == 3) {
        return {0.5f - halfThickness, 0.2f, 0.0f, 0.5f + halfThickness, 0.8f, halfThickness * 2.0f};
    }
    if (face == 4) {
        return {0.5f - halfThickness, 0.2f, 1.0f - halfThickness * 2.0f, 0.5f + halfThickness, 0.8f, 1.0f};
    }
    constexpr float halfWidth = 0.25f;
    return {0.5f - halfWidth, 0.0f, 0.5f - halfWidth, 0.5f + halfWidth, 0.6f, 0.5f + halfWidth};
}
""")

sub("NetherPortalBlock.cpp",
"""void NetherPortalBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return;
    }
    if (blockView->getBlockId(x - 1, y, z) == id || blockView->getBlockId(x + 1, y, z) == id) {
        constexpr float halfWidth = 0.5f;
        constexpr float halfDepth = 0.125f;
        setBoundingBox(0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth);
    } else {
        constexpr float halfWidth = 0.125f;
        constexpr float halfDepth = 0.5f;
        setBoundingBox(0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth);
    }
}
""",
wrapper("NetherPortalBlock") +
"""net::minecraft::Box NetherPortalBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
    if (blockView->getBlockId(x - 1, y, z) == id || blockView->getBlockId(x + 1, y, z) == id) {
        constexpr float halfWidth = 0.5f;
        constexpr float halfDepth = 0.125f;
        return {0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth};
    }
    constexpr float halfWidth = 0.125f;
    constexpr float halfDepth = 0.5f;
    return {0.5f - halfWidth, 0.0f, 0.5f - halfDepth, 0.5f + halfWidth, 1.0f, 0.5f + halfDepth};
}
""")

sub("PistonBlock.cpp",
"""void PistonBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return;
    }
    const int meta = blockView->getBlockMeta(x, y, z);
    if (!isExtended(meta)) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return;
    }
    switch (getFacing(meta)) {
    case 0:
        setBoundingBox(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f);
        break;
    case 1:
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
        break;
    case 2:
        setBoundingBox(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f);
        break;
    case 3:
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f);
        break;
    case 4:
        setBoundingBox(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        break;
    case 5:
        setBoundingBox(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);
        break;
    default:
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        break;
    }
}
""",
wrapper("PistonBlock") +
"""net::minecraft::Box PistonBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    const int meta = blockView->getBlockMeta(x, y, z);
    if (!isExtended(meta)) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    switch (getFacing(meta)) {
    case 0:
        return {0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f};
    case 1:
        return {0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f};
    case 2:
        return {0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f};
    case 3:
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f};
    case 4:
        return {0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    case 5:
        return {0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f};
    default:
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
}
""")

sub("PistonHeadBlock.cpp",
"""void PistonHeadBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return;
    }
    switch (getFacing(blockView->getBlockMeta(x, y, z))) {
    case 0:
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
        break;
    case 1:
        setBoundingBox(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
        break;
    case 2:
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
        break;
    case 3:
        setBoundingBox(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
        break;
    case 4:
        setBoundingBox(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
        break;
    case 5:
        setBoundingBox(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        break;
    default:
        break;
    }
}
""",
wrapper("PistonHeadBlock") +
"""net::minecraft::Box PistonHeadBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
    switch (getFacing(blockView->getBlockMeta(x, y, z))) {
    case 0:
        return {0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f};
    case 1:
        return {0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f};
    case 2:
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f};
    case 3:
        return {0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f};
    case 4:
        return {0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f};
    case 5:
        return {0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    default:
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
}
""")

sub("PressurePlateBlock.cpp",
"""void PressurePlateBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const bool pressed = blockView != nullptr && blockView->getBlockMeta(x, y, z) == 1;
    const float inset = 0.0625f;
    const float height = pressed ? 0.03125f : 0.0625f;
    setBoundingBox(inset, 0.0f, inset, 1.0f - inset, height, 1.0f - inset);
}
""",
wrapper("PressurePlateBlock") +
"""net::minecraft::Box PressurePlateBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const bool pressed = blockView != nullptr && blockView->getBlockMeta(x, y, z) == 1;
    const float inset = 0.0625f;
    const float height = pressed ? 0.03125f : 0.0625f;
    return {inset, 0.0f, inset, 1.0f - inset, height, 1.0f - inset};
}
""")

sub("RailBlock.cpp",
"""void RailBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    if (meta >= 2 && meta <= 5) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f);
    } else {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    }
}
""",
wrapper("RailBlock") +
"""net::minecraft::Box RailBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    if (meta >= 2 && meta <= 5) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f};
    }
    return {0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f};
}
""")

sub("SignBlock.cpp",
"""void SignBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (standing) {
        return;
    }

    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    constexpr float yMin = 0.28125f;
    constexpr float yMax = 0.78125f;
    constexpr float thickness = 0.125f;
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    if (meta == 2) {
        setBoundingBox(0.0f, yMin, 1.0f - thickness, 1.0f, yMax, 1.0f);
    } else if (meta == 3) {
        setBoundingBox(0.0f, yMin, 0.0f, 1.0f, yMax, thickness);
    } else if (meta == 4) {
        setBoundingBox(1.0f - thickness, yMin, 0.0f, 1.0f, yMax, 1.0f);
    } else if (meta == 5) {
        setBoundingBox(0.0f, yMin, 0.0f, thickness, yMax, 1.0f);
    }
}
""",
"""void SignBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (standing) {
        return;
    }
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box SignBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (standing) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }

    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    constexpr float yMin = 0.28125f;
    constexpr float yMax = 0.78125f;
    constexpr float thickness = 0.125f;
    if (meta == 2) {
        return {0.0f, yMin, 1.0f - thickness, 1.0f, yMax, 1.0f};
    }
    if (meta == 3) {
        return {0.0f, yMin, 0.0f, 1.0f, yMax, thickness};
    }
    if (meta == 4) {
        return {1.0f - thickness, yMin, 0.0f, 1.0f, yMax, 1.0f};
    }
    if (meta == 5) {
        return {0.0f, yMin, 0.0f, thickness, yMax, 1.0f};
    }
    return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
}
""")

sub("StairsBlock.cpp",
"""void StairsBlock::updateBoundingBox(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/)
{
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}
""",
wrapper("StairsBlock") +
"""net::minecraft::Box StairsBlock::getRenderBounds(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/) const
{
    return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
}
""")

# --- DoorBlock: rotate() keeps mutating; const boundsForMeta carries the math ---
sub("DoorBlock.cpp",
"""void DoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return;
    }
    rotate(blockView->getBlockMeta(x, y, z));
}
""",
"""void DoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return;
    }
    rotate(blockView->getBlockMeta(x, y, z));
}

net::minecraft::Box DoorBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }
    return boundsForMeta(blockView->getBlockMeta(x, y, z));
}
""")

sub("DoorBlock.cpp",
"""void DoorBlock::rotate(int meta)
{
    constexpr float thickness = 0.1875f;
    const int oriented = facingFromMeta(meta);
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    if (oriented == 0) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    } else if (oriented == 1) {
        setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else if (oriented == 2) {
        setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    } else {
        setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
    }
}
""",
"""void DoorBlock::rotate(int meta)
{
    setBoundingBox(boundsForMeta(meta));
}

net::minecraft::Box DoorBlock::boundsForMeta(int meta) const
{
    constexpr float thickness = 0.1875f;
    const int oriented = facingFromMeta(meta);
    if (oriented == 0) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness};
    }
    if (oriented == 1) {
        return {1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    if (oriented == 2) {
        return {0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f};
    }
    return {0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f};
}
""")

sub("LadderBlock.cpp",
"""void LadderBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}
""",
"""void LadderBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}

net::minecraft::Box LadderBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    return boundsForMeta(meta);
}
""")

sub("LadderBlock.cpp",
"""void LadderBlock::applyBoundsForMeta(int meta)
{
    constexpr float thickness = 0.125f;
    if (meta == 2) {
        setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    } else if (meta == 3) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    } else if (meta == 4) {
        setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else if (meta == 5) {
        setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
    }
}
""",
"""void LadderBlock::applyBoundsForMeta(int meta)
{
    if (meta >= 2 && meta <= 5) {
        setBoundingBox(boundsForMeta(meta));
    }
}

net::minecraft::Box LadderBlock::boundsForMeta(int meta) const
{
    constexpr float thickness = 0.125f;
    if (meta == 2) {
        return {0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f};
    }
    if (meta == 3) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness};
    }
    if (meta == 4) {
        return {1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    if (meta == 5) {
        return {0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f};
    }
    return {minX, minY, minZ, maxX, maxY, maxZ};
}
""")

sub("TrapdoorBlock.cpp",
"""void TrapdoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}
""",
"""void TrapdoorBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    applyBoundsForMeta(meta);
}

net::minecraft::Box TrapdoorBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    return boundsForMeta(meta);
}
""")

sub("TrapdoorBlock.cpp",
"""void TrapdoorBlock::applyBoundsForMeta(int meta)
{
    constexpr float thickness = 0.1875f;
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, thickness, 1.0f);
    if (!isOpen(meta)) {
        return;
    }
    if ((meta & 3) == 0) {
        setBoundingBox(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    } else if ((meta & 3) == 1) {
        setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    } else if ((meta & 3) == 2) {
        setBoundingBox(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    } else {
        setBoundingBox(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
    }
}
""",
"""void TrapdoorBlock::applyBoundsForMeta(int meta)
{
    setBoundingBox(boundsForMeta(meta));
}

net::minecraft::Box TrapdoorBlock::boundsForMeta(int meta) const
{
    constexpr float thickness = 0.1875f;
    if (!isOpen(meta)) {
        return {0.0f, 0.0f, 0.0f, 1.0f, thickness, 1.0f};
    }
    if ((meta & 3) == 0) {
        return {0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f};
    }
    if ((meta & 3) == 1) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness};
    }
    if ((meta & 3) == 2) {
        return {1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    return {0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f};
}
""")

# --- PistonExtensionBlock: render bounds via pushed block's const query ---
sub("PistonExtensionBlock.cpp",
"""void PistonExtensionBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    entity::PistonBlockEntity* piston = getPistonBlockEntity(blockView, x, y, z);
    if (piston == nullptr) {
        return;
    }
    const int pushedBlockId = piston->getPushedBlockId();
    if (pushedBlockId <= 0 || pushedBlockId >= Block::BLOCK_COUNT) {
        return;
    }
    Block* pushedBlock = Block::BLOCKS[static_cast<std::size_t>(pushedBlockId)];
    if (pushedBlock == nullptr || pushedBlock == this) {
        return;
    }
    pushedBlock->updateBoundingBox(blockView, x, y, z);
    float progress = piston->getProgress(0.0f);
    if (piston->isExtending()) {
        progress = 1.0f - progress;
    }
    const int facing = piston->getFacing();
    if (facing < 0 || facing >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
        return;
    }
    const float offsetX = static_cast<float>(PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)]) * progress;
    const float offsetY = static_cast<float>(PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)]) * progress;
    const float offsetZ = static_cast<float>(PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)]) * progress;
    minX = pushedBlock->minX - static_cast<double>(offsetX);
    minY = pushedBlock->minY - static_cast<double>(offsetY);
    minZ = pushedBlock->minZ - static_cast<double>(offsetZ);
    maxX = pushedBlock->maxX - static_cast<double>(offsetX);
    maxY = pushedBlock->maxY - static_cast<double>(offsetY);
    maxZ = pushedBlock->maxZ - static_cast<double>(offsetZ);
}
""",
"""void PistonExtensionBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box PistonExtensionBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const net::minecraft::Box current {minX, minY, minZ, maxX, maxY, maxZ};
    entity::PistonBlockEntity* piston = getPistonBlockEntity(blockView, x, y, z);
    if (piston == nullptr) {
        return current;
    }
    const int pushedBlockId = piston->getPushedBlockId();
    if (pushedBlockId <= 0 || pushedBlockId >= Block::BLOCK_COUNT) {
        return current;
    }
    Block* pushedBlock = Block::BLOCKS[static_cast<std::size_t>(pushedBlockId)];
    if (pushedBlock == nullptr || pushedBlock == this) {
        return current;
    }
    const net::minecraft::Box pushed = pushedBlock->getRenderBounds(blockView, x, y, z);
    float progress = piston->getProgress(0.0f);
    if (piston->isExtending()) {
        progress = 1.0f - progress;
    }
    const int facing = piston->getFacing();
    if (facing < 0 || facing >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
        return current;
    }
    const float offsetX = static_cast<float>(PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)]) * progress;
    const float offsetY = static_cast<float>(PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)]) * progress;
    const float offsetZ = static_cast<float>(PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)]) * progress;
    return pushed.offset(-static_cast<double>(offsetX), -static_cast<double>(offsetY), -static_cast<double>(offsetZ));
}
""")

print("all conversions done")
