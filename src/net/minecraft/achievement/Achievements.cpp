#include "net/minecraft/achievement/Achievements.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::achievement {

ItemStack Achievements::iconStack(const AchievementDef& achievement)
{
    using block::Block;
    switch (achievement.index) {
    case 0:
        return ItemStack(Item::BOOK != nullptr ? Item::BOOK->id : 0);
    case 1:
        return ItemStack(Block::LOG != nullptr ? Block::LOG->id : 0);
    case 2:
        return ItemStack(Block::CRAFTING_TABLE != nullptr ? Block::CRAFTING_TABLE->id : 0);
    case 3:
        return ItemStack(Item::WOODEN_PICKAXE != nullptr ? Item::WOODEN_PICKAXE->id : 0);
    case 4:
        return ItemStack(Block::LIT_FURNACE != nullptr ? Block::LIT_FURNACE->id : 0);
    case 5:
        return ItemStack(Item::IRON_INGOT != nullptr ? Item::IRON_INGOT->id : 0);
    case 6:
        return ItemStack(Item::WOODEN_HOE != nullptr ? Item::WOODEN_HOE->id : 0);
    case 7:
        return ItemStack(Item::BREAD != nullptr ? Item::BREAD->id : 0);
    case 8:
        return ItemStack(Item::CAKE != nullptr ? Item::CAKE->id : 0);
    case 9:
        return ItemStack(Item::STONE_PICKAXE != nullptr ? Item::STONE_PICKAXE->id : 0);
    case 10:
        return ItemStack(Item::COOKED_FISH != nullptr ? Item::COOKED_FISH->id : 0);
    case 11:
        return ItemStack(Block::RAIL != nullptr ? Block::RAIL->id : 0);
    case 12:
        return ItemStack(Item::WOODEN_SWORD != nullptr ? Item::WOODEN_SWORD->id : 0);
    case 13:
        return ItemStack(Item::BONE != nullptr ? Item::BONE->id : 0);
    case 14:
        return ItemStack(Item::LEATHER != nullptr ? Item::LEATHER->id : 0);
    case 15:
        return ItemStack(Item::SADDLE != nullptr ? Item::SADDLE->id : 0);
    default:
        return {};
    }
}

std::string Achievements::getTranslatedTitle(const AchievementDef& achievement)
{
    return client::resource::language::I18n::getTranslation(std::string("achievement.") + achievement.key);
}

std::string Achievements::getTranslatedDescription(
    const AchievementDef& achievement, const std::function<std::string(const std::string&)>& formatter)
{
    const std::string key = std::string("achievement.") + achievement.key + ".desc";
    const std::string translated = client::resource::language::I18n::getTranslation(key);
    if (formatter) {
        return formatter(translated);
    }
    return translated;
}

std::string Achievements::getFormattedDescription(const AchievementDef& achievement, int inventoryKeyCode)
{
    if (achievement.index != 0) {
        return getTranslatedDescription(achievement);
    }
    return getTranslatedDescription(achievement, [inventoryKeyCode](const std::string& text) {
        const char* keyName = client::input::keyDisplayName(inventoryKeyCode);
        return client::resource::language::I18n::formatJava(text, {keyName != nullptr ? keyName : "?"});
    });
}

} // namespace net::minecraft::achievement
