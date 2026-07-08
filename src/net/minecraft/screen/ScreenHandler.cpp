#include "net/minecraft/screen/ScreenHandler.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"

namespace net::minecraft::screen {
ItemStack ScreenHandler::onSlotClick(int index, int button, bool shift, PlayerEntity* player) {
    ItemStack carried{};
    if (player == nullptr || (button != 0 && button != 1)) {
        return carried;
    }
    PlayerInventory& playerInventory = player->inventory;
    if (index == -999) {
        ItemStack cursor = playerInventory.getCursorStack();
        if (!cursor.empty()) {
            if (button == 0) {
                player->dropItem(cursor);
                playerInventory.setCursorStack({});
            } else if (button == 1) {
                player->dropItem(cursor.split(1));
                if (cursor.empty()) {
                    playerInventory.setCursorStack({});
                } else {
                    playerInventory.setCursorStack(cursor);
                }
            }
        }
        return carried;
    }
    if (shift) {
        ItemStack moved = quickMove(index);
        if (!moved.empty()) {
            const int previousCount = moved.count;
            carried = moved.copy();
            slot::Slot* currentSlot = getSlot(index);
            if (currentSlot != nullptr && currentSlot->hasStack()) {
                const int remaining = currentSlot->getStack().count;
                if (remaining < previousCount) {
                    onSlotClick(index, button, shift, player);
                }
            }
        }
        return carried;
    }
    slot::Slot* currentSlot = getSlot(index);
    if (currentSlot == nullptr) {
        return carried;
    }
    currentSlot->markDirty();
    ItemStack slotStack = currentSlot->getStack();
    ItemStack cursorStack = playerInventory.getCursorStack();
    if (!slotStack.empty()) {
        carried = slotStack.copy();
    }
    if (slotStack.empty()) {
        if (!cursorStack.empty() && currentSlot->canInsert(cursorStack)) {
            int amount = button == 0 ? cursorStack.count : 1;
            if (amount > currentSlot->getMaxItemCount()) {
                amount = currentSlot->getMaxItemCount();
            }
            currentSlot->setStack(cursorStack.split(amount));
            if (cursorStack.empty()) {
                playerInventory.setCursorStack({});
            } else {
                playerInventory.setCursorStack(cursorStack);
            }
        }
    } else if (cursorStack.empty()) {
        const int amount = button == 0 ? slotStack.count : (slotStack.count + 1) / 2;
        ItemStack taken = currentSlot->takeStack(amount);
        playerInventory.setCursorStack(taken);
        if (!currentSlot->hasStack()) {
            currentSlot->setStack({});
        }
        currentSlot->onTakeItem(playerInventory.getCursorStack());
    } else if (currentSlot->canInsert(cursorStack)) {
        if (slotStack.itemId != cursorStack.itemId ||
            (slotStack.hasSubtypes() && slotStack.getDamage() != cursorStack.getDamage())) {
            if (cursorStack.count <= currentSlot->getMaxItemCount()) {
                ItemStack swapped = slotStack;
                currentSlot->setStack(cursorStack);
                playerInventory.setCursorStack(swapped);
            }
        } else {
            int amount = button == 0 ? cursorStack.count : 1;
            if (amount > currentSlot->getMaxItemCount() - slotStack.count) {
                amount = currentSlot->getMaxItemCount() - slotStack.count;
            }
            if (amount > cursorStack.getMaxCount() - slotStack.count) {
                amount = cursorStack.getMaxCount() - slotStack.count;
            }
            [[maybe_unused]] ItemStack removed = cursorStack.split(amount);
            if (cursorStack.empty()) {
                playerInventory.setCursorStack({});
            } else {
                playerInventory.setCursorStack(cursorStack);
            }
            slotStack.count += amount;
            currentSlot->setStack(slotStack);
        }
    } else if (slotStack.itemId == cursorStack.itemId && cursorStack.getMaxCount() > 1 &&
               (!slotStack.hasSubtypes() || slotStack.getDamage() == cursorStack.getDamage())) {
        const int slotCount = slotStack.count;
        if (slotCount > 0 && slotCount + cursorStack.count <= cursorStack.getMaxCount()) {
            cursorStack.count += slotCount;
            playerInventory.setCursorStack(cursorStack);
            [[maybe_unused]] ItemStack removed = slotStack.split(slotCount);
            if (slotStack.empty()) {
                currentSlot->setStack({});
            } else {
                currentSlot->setStack(slotStack);
            }
            currentSlot->onTakeItem(playerInventory.getCursorStack());
        }
    }
    return carried;
}

void ScreenHandler::onClosed(PlayerEntity* player) {
    if (player == nullptr) {
        return;
    }
    ItemStack cursor = player->inventory.getCursorStack();
    if (!cursor.empty()) {
        player->dropItem(cursor);
        player->inventory.setCursorStack({});
    }
}
}  // namespace net::minecraft::screen
