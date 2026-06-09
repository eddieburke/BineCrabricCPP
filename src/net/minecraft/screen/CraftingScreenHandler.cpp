#include "net/minecraft/screen/CraftingScreenHandler.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::screen {

CraftingScreenHandler::CraftingScreenHandler(entity::player::PlayerInventory* inventory, int xIn, int yIn, int zIn)
    : craftingInput(this, 3, 3),
      x(xIn),
      y(yIn),
      z(zIn)
{
    if (inventory == nullptr) {
        return;
    }
    addSlot(new slot::CraftingResultSlot(inventory->player(), &craftingInput, &craftingResult, 0, 124, 35));
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            addSlot(new slot::Slot(&craftingInput, column + row * 3, 30 + column * 18, 17 + row * 18));
        }
    }
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 9; ++column) {
            addSlot(new slot::Slot(inventory, column + row * 9 + 9, 8 + column * 18, 84 + row * 18));
        }
    }
    for (int column = 0; column < 9; ++column) {
        addSlot(new slot::Slot(inventory, column, 8 + column * 18, 142));
    }
    onSlotUpdate(&craftingInput);
}

void CraftingScreenHandler::onSlotUpdate(Inventory* inventory)
{
    (void)inventory;
    craftingResult.setStack(0, recipe::CraftingRecipeManager::getInstance().craft(craftingInput));
}

void CraftingScreenHandler::onClosed(PlayerEntity* player)
{
    ScreenHandler::onClosed(player);
    if (player == nullptr || player->world == nullptr || player->world->isRemote()) {
        return;
    }
    for (std::size_t i = 0; i < craftingInput.size(); ++i) {
        ItemStack stack = craftingInput.getStack(i);
        if (stack.empty()) {
            continue;
        }
        player->dropItem(stack);
    }
}

bool CraftingScreenHandler::canUse(PlayerEntity* player)
{
    if (player == nullptr || player->world == nullptr) {
        return false;
    }
    if (Block::CRAFTING_TABLE == nullptr
        || player->world->getBlockId(x, y, z) != Block::CRAFTING_TABLE->id) {
        return false;
    }
    return player->getSquaredDistance(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
               static_cast<double>(z) + 0.5)
        <= 64.0;
}

ItemStack CraftingScreenHandler::quickMove(int slotIndex)
{
    ItemStack moved {};
    slot::Slot* currentSlot = getSlot(slotIndex);
    if (currentSlot == nullptr || !currentSlot->hasStack()) {
        return {};
    }
    ItemStack slotStack = currentSlot->getStack();
    moved = slotStack.copy();
    if (slotIndex == 0) {
        insertItem(slotStack, 10, 46, true);
    } else if (slotIndex >= 10 && slotIndex < 37) {
        insertItem(slotStack, 37, 46, false);
    } else if (slotIndex >= 37 && slotIndex < 46) {
        insertItem(slotStack, 10, 37, false);
    } else {
        insertItem(slotStack, 10, 46, false);
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
