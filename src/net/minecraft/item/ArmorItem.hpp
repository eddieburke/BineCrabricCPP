#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class ArmorItem : public Item {
public:
protected:
    ArmorItem(int rawId, int type, int textureIndex, int equipmentSlot)
        : Item(rawId, RegistrationMode::Deferred),
          type(type),
          equipmentSlot(equipmentSlot),
          maxProtection(PROTECTION_BY_SLOT[equipmentSlot]),
          textureIndex(textureIndex)
    {
        setMaxCount(1);
        setMaxDamage(DURABILITY_BY_SLOT[equipmentSlot] * 3 << type);
    }

    static constexpr int PROTECTION_BY_SLOT[4] = {3, 8, 6, 3};
    static constexpr int DURABILITY_BY_SLOT[4] = {11, 16, 15, 13};

public:
    [[nodiscard]] int getMaxProtection() const { return maxProtection; }
    [[nodiscard]] int getTextureIndex() const { return textureIndex; }
    [[nodiscard]] int getEquipmentSlot() const { return equipmentSlot; }

protected:
    int type = 0;
    int equipmentSlot = 0;
    int maxProtection = 0;
    int textureIndex = 0;
};

} // namespace net::minecraft::item
