#pragma once

namespace net::minecraft::item {

enum class ToolMaterial {
    Wood,
    Stone,
    Iron,
    Diamond,
    Gold
};

inline int toolMaterialDurability(ToolMaterial material)
{
    switch (material) {
    case ToolMaterial::Wood:
        return 59;
    case ToolMaterial::Stone:
        return 131;
    case ToolMaterial::Iron:
        return 250;
    case ToolMaterial::Diamond:
        return 1561;
    case ToolMaterial::Gold:
        return 32;
    }
    return 0;
}

inline float toolMaterialMiningSpeed(ToolMaterial material)
{
    switch (material) {
    case ToolMaterial::Wood:
        return 2.0f;
    case ToolMaterial::Stone:
        return 4.0f;
    case ToolMaterial::Iron:
        return 6.0f;
    case ToolMaterial::Diamond:
        return 8.0f;
    case ToolMaterial::Gold:
        return 12.0f;
    }
    return 1.0f;
}

inline int toolMaterialAttackDamage(ToolMaterial material)
{
    switch (material) {
    case ToolMaterial::Wood:
        return 0;
    case ToolMaterial::Stone:
        return 1;
    case ToolMaterial::Iron:
        return 2;
    case ToolMaterial::Diamond:
        return 3;
    case ToolMaterial::Gold:
        return 0;
    }
    return 0;
}

inline int toolMaterialMiningLevel(ToolMaterial material)
{
    switch (material) {
    case ToolMaterial::Wood:
        return 0;
    case ToolMaterial::Stone:
        return 1;
    case ToolMaterial::Iron:
        return 2;
    case ToolMaterial::Diamond:
        return 3;
    case ToolMaterial::Gold:
        return 0;
    }
    return 0;
}

} // namespace net::minecraft::item
