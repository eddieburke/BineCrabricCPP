#pragma once

#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block::material {

class AirMaterial : public Material {
public:
    explicit AirMaterial(const MapColor& color)
        : Material(color)
    {
        setReplaceable();
    }

    [[nodiscard]] bool isSolid() const override { return false; }
    [[nodiscard]] bool blocksVision() const override { return false; }
    [[nodiscard]] bool blocksMovement() const override { return false; }
};

} // namespace net::minecraft::block::material
