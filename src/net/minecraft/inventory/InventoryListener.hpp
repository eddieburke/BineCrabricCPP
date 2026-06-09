#pragma once

#include "net/minecraft/inventory/SimpleInventory.hpp"

namespace net::minecraft {

class InventoryListener {
public:
    virtual ~InventoryListener() = default;
    virtual void onInventoryChanged(SimpleInventory* inventory) = 0;
};

} // namespace net::minecraft
