#pragma once

#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block::material {

class FluidMaterial : public Material {
public:
    explicit FluidMaterial(const MapColor& color)
        : Material(color)
    {
        setReplaceable();
        setDestroyPistonBehavior();
    }

    [[nodiscard]] bool isFluid() const override { return true; }
    [[nodiscard]] bool blocksMovement() const override { return false; }
    [[nodiscard]] bool isSolid() const override { return false; }
};

} // namespace net::minecraft::block::material
