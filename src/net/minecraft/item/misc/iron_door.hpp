#pragma once

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/DoorItem.hpp"

namespace net::minecraft::item {

class IronDoorItem : public DoorItem {
public:
    static constexpr int ID = 330;
    IronDoorItem() : DoorItem(74, block::material::Material::METAL) {
        setTexturePosition(12, 2)->setTranslationKey("doorIron");
    }
};

} // namespace net::minecraft::item
