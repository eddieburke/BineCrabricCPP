#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class Packet;
}

namespace net::minecraft::item {

class NetworkSyncedItem : public Item {
public:
    explicit NetworkSyncedItem(int rawId) : Item(rawId) {}

    [[nodiscard]] virtual bool isNetworkSynced() const { return true; }
    [[nodiscard]] virtual Packet* getUpdatePacket(ItemStack* /*stack*/, World* /*world*/, PlayerEntity* /*player*/) { return nullptr; }
};

} // namespace net::minecraft::item
