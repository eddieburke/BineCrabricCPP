#pragma once

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/DoorItem.hpp"

namespace net::minecraft::item {

class WoodenDoorItem : public DoorItem {
public:
    static constexpr int ID = 324;
    WoodenDoorItem() : DoorItem(68, block::material::Material::WOOD) {
        setTexturePosition(11, 2)->setTranslationKey("doorWood");
    }
};

} // namespace net::minecraft::item
