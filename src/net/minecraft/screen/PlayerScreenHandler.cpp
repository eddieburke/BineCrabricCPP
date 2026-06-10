#include "net/minecraft/screen/PlayerScreenHandler.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/screen/slot/CraftingResultSlot.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"

namespace net::minecraft::screen {

namespace {

class ArmorSlot : public slot::Slot {
public:
    ArmorSlot(PlayerScreenHandler* handler, entity::player::PlayerInventory* inventory, int armorSlotIndex, int x, int y)
        : Slot(inventory, static_cast<int>(inventory->size()) - 1 - armorSlotIndex, x, y),
          armorSlotIndex_(armorSlotIndex)
    {
        (void)handler;
    }

    [[nodiscard]] int getMaxItemCount() const override
    {
        return 1;
    }

    [[nodiscard]] bool canInsert(const ItemStack& stack) const override
    {
        Item* item = stack.getItem();
        if (auto* armor = dynamic_cast<item::ArmorItem*>(item); armor != nullptr) {
            return armor->getEquipmentSlot() == armorSlotIndex_;
        }
        if (Block::PUMPKIN != nullptr && stack.itemId == Block::PUMPKIN->id) {
            return armorSlotIndex_ == 0;
        }
        return false;
    }

private:
    int armorSlotIndex_ = 0;
};

} // namespace

PlayerScreenHandler::PlayerScreenHandler(entity::player::PlayerInventory* inventory, bool isLocalIn)
    : craftingInput(this, 2, 2),
      isLocal(isLocalIn)
{
    if (inventory == nullptr) {
        return;
    }
    entity::player::PlayerEntity* player = inventory->player();
    addSlot(new slot::CraftingResultSlot(player, &craftingInput, &craftingResult, 0, 144, 36));
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            addSlot(new slot::Slot(&craftingInput, column + row * 2, 88 + column * 18, 26 + row * 18));
        }
    }
    for (int armorSlot = 0; armorSlot < 4; ++armorSlot) {
        addSlot(new ArmorSlot(this, inventory, armorSlot, 8, 8 + armorSlot * 18));
    }
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 9; ++column) {
            addSlot(new slot::Slot(inventory, column + (row + 1) * 9, 8 + column * 18, 84 + row * 18));
        }
    }
    for (int column = 0; column < 9; ++column) {
        addSlot(new slot::Slot(inventory, column, 8 + column * 18, 142));
    }
    onSlotUpdate(&craftingInput);
}

void PlayerScreenHandler::onSlotUpdate(Inventory* inventory)
{
    (void)inventory;
    craftingResult.setStack(0, recipe::CraftingRecipeManager::getInstance().craft(craftingInput));
}

void PlayerScreenHandler::onClosed(PlayerEntity* player)
{
    ScreenHandler::onClosed(player);
    if (player == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < craftingInput.size(); ++i) {
        ItemStack stack = craftingInput.getStack(i);
        if (stack.empty()) {
            continue;
        }
        player->dropItem(stack);
        craftingInput.setStack(i, {});
    }
}

bool PlayerScreenHandler::canUse(PlayerEntity* player)
{
    (void)player;
    return true;
}

ItemStack PlayerScreenHandler::quickMove(int slotIndex)
{
    ItemStack moved {};
    slot::Slot* currentSlot = getSlot(slotIndex);
    if (currentSlot == nullptr || !currentSlot->hasStack()) {
        return {};
    }
    ItemStack slotStack = currentSlot->getStack();
    moved = slotStack.copy();
    if (slotIndex == 0) {
        insertItem(slotStack, 9, 45, true);
    } else if (slotIndex >= 9 && slotIndex < 36) {
        insertItem(slotStack, 36, 45, false);
    } else if (slotIndex >= 36 && slotIndex < 45) {
        insertItem(slotStack, 9, 36, false);
    } else {
        insertItem(slotStack, 9, 45, false);
    }
    if (slotStack.count == 0) {
        currentSlot->setStack({});
    } else {
        currentSlot->setStack(slotStack);
    }
    if (slotStack.count != moved.count) {
        currentSlot->onTakeItem(slotStack);
        return moved;
    }
    return {};
}

} // namespace net::minecraft::screen
