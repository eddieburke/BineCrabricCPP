#pragma once

#include "net/minecraft/item/ItemStack.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <string>

namespace net::minecraft::achievement {

struct AchievementDef {
    int index = 0;
    int parentIndex = -1;
    int column = 0;
    int row = 0;
    const char* key = nullptr;
    bool localOnly = false;
    bool challenge = false;

    [[nodiscard]] int statId() const noexcept { return kBase + index; }
    [[nodiscard]] int parentStatId() const noexcept
    {
        return parentIndex >= 0 ? kBase + parentIndex : -1;
    }

    static constexpr int kBase = 0x500000;
};

struct Achievements {
    static constexpr AchievementDef OPEN_INVENTORY {0, -1, 0, 0, "openInventory", true, false};
    static constexpr AchievementDef MINE_WOOD {1, 0, 2, 1, "mineWood", false, false};
    static constexpr AchievementDef CRAFT_WORKBENCH {2, 1, 4, -1, "buildWorkBench", false, false};
    static constexpr AchievementDef CRAFT_PICKAXE {3, 2, 4, 2, "buildPickaxe", false, false};
    static constexpr AchievementDef CRAFT_FURNACE {4, 3, 3, 4, "buildFurnace", false, false};
    static constexpr AchievementDef ACQUIRE_IRON {5, 4, 1, 4, "acquireIron", false, false};
    static constexpr AchievementDef CRAFT_HOE {6, 2, 2, -3, "buildHoe", false, false};
    static constexpr AchievementDef CRAFT_BREAD {7, 6, -1, -3, "makeBread", false, false};
    static constexpr AchievementDef CRAFT_CAKE {8, 6, 0, -5, "bakeCake", false, false};
    static constexpr AchievementDef CRAFT_STONE_PICKAXE {9, 3, 6, 2, "buildBetterPickaxe", false, false};
    static constexpr AchievementDef COOK_FISH {10, 4, 2, 6, "cookFish", false, false};
    static constexpr AchievementDef CRAFT_RAIL {11, 5, 2, 3, "onARail", false, true};
    static constexpr AchievementDef CRAFT_SWORD {12, 2, 6, -1, "buildSword", false, false};
    static constexpr AchievementDef KILL_ENEMY {13, 12, 8, -1, "killEnemy", false, false};
    static constexpr AchievementDef KILL_COW {14, 12, 7, -3, "killCow", false, false};
    static constexpr AchievementDef FLY_PIG {15, 14, 8, -4, "flyPig", false, true};

    static constexpr std::array<AchievementDef, 16> ALL {
        OPEN_INVENTORY,
        MINE_WOOD,
        CRAFT_WORKBENCH,
        CRAFT_PICKAXE,
        CRAFT_FURNACE,
        ACQUIRE_IRON,
        CRAFT_HOE,
        CRAFT_BREAD,
        CRAFT_CAKE,
        CRAFT_STONE_PICKAXE,
        COOK_FISH,
        CRAFT_RAIL,
        CRAFT_SWORD,
        KILL_ENEMY,
        KILL_COW,
        FLY_PIG,
    };

    [[nodiscard]] static bool isAchievementStatId(int statId) noexcept
    {
        return statId >= AchievementDef::kBase && statId < AchievementDef::kBase + static_cast<int>(ALL.size());
    }

    [[nodiscard]] static const AchievementDef* getByStatId(int statId) noexcept
    {
        if (!isAchievementStatId(statId)) {
            return nullptr;
        }
        return &ALL[static_cast<std::size_t>(statId - AchievementDef::kBase)];
    }

    static constexpr int minColumn = -5;
    static constexpr int maxColumn = 8;
    static constexpr int minRow = -5;
    static constexpr int maxRow = 6;

    [[nodiscard]] static ItemStack iconStack(const AchievementDef& achievement);
    [[nodiscard]] static std::string getTranslatedTitle(const AchievementDef& achievement);
    [[nodiscard]] static std::string getTranslatedDescription(
        const AchievementDef& achievement,
        const std::function<std::string(const std::string&)>& formatter = {});
    [[nodiscard]] static std::string getFormattedDescription(
        const AchievementDef& achievement, int inventoryKeyCode);
};

} // namespace net::minecraft::achievement
