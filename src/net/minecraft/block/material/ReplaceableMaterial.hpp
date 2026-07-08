#pragma once
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block::material {
class ReplaceableMaterial : public Material {
   public:
    explicit ReplaceableMaterial(const MapColor& color) : Material(color) {
    }

    [[nodiscard]] bool isSolid() const override {
        return false;
    }

    [[nodiscard]] bool blocksVision() const override {
        return false;
    }

    [[nodiscard]] bool blocksMovement() const override {
        return false;
    }
};
}  // namespace net::minecraft::block::material
