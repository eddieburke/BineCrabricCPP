#pragma once

#include "net/minecraft/item/ItemStack.hpp"

#include <vector>

namespace net::minecraft::screen {

class ScreenHandler;

class ScreenHandlerListener {
public:
    virtual ~ScreenHandlerListener() = default;

    virtual void onContentsUpdate(ScreenHandler* handler, const std::vector<ItemStack>& stacks)
    {
        (void)handler;
        (void)stacks;
    }

    virtual void onSlotUpdate(ScreenHandler* handler, int slot, const ItemStack& stack)
    {
        (void)handler;
        (void)slot;
        (void)stack;
    }

    virtual void onPropertyUpdate(ScreenHandler* handler, int id, int value)
    {
        (void)handler;
        (void)id;
        (void)value;
    }
};

} // namespace net::minecraft::screen
