#pragma once
#include "net/minecraft/block/MapColor.hpp"
namespace net::minecraft::block::material {
// Mirrors net.minecraft.block.material.Material (beta 1.7.3 MCP).
class Material {
public:
  explicit Material(const MapColor& mapColor) : mapColor(mapColor) {}
  virtual ~Material() = default;
  [[nodiscard]] virtual bool isFluid() const {
    return false;
  }
  [[nodiscard]] virtual bool isSolid() const {
    return true;
  }
  [[nodiscard]] virtual bool blocksVision() const {
    return true;
  }
  [[nodiscard]] virtual bool blocksMovement() const {
    return true;
  }
  [[nodiscard]] bool isBurnable() const {
    return burnable_;
  }
  [[nodiscard]] bool isReplaceable() const {
    return replaceable_;
  }
  [[nodiscard]] bool suffocates() const {
    return !transparent_ && blocksMovement();
  }
  [[nodiscard]] bool isHandHarvestable() const {
    return handHarvestable_;
  }
  [[nodiscard]] int getPistonBehavior() const {
    return pistonBehavior_;
  }
  Material& setReplaceable() {
    replaceable_ = true;
    return *this;
  }
  const MapColor& mapColor;
  // Java: public static final Material * (subclass instances stored in Material.cpp).
  static Material& AIR;
  static Material SOLID_ORGANIC;
  static Material SOIL;
  static Material WOOD;
  static Material STONE;
  static Material METAL;
  static Material& WATER;
  static Material& LAVA;
  static Material LEAVES;
  static Material& PLANT;
  static Material SPONGE;
  static Material WOOL;
  static Material& FIRE;
  static Material SAND;
  static Material& PISTON_BREAKABLE;
  static Material GLASS;
  static Material TNT;
  static Material UNUSED;
  static Material ICE;
  static Material& SNOW_LAYER;
  static Material SNOW_BLOCK;
  static Material CACTUS;
  static Material CLAY;
  static Material PUMPKIN;
  static Material& NETHER_PORTAL;
  static Material CAKE;
  static Material COBWEB;
  static Material PISTON;

protected:
  Material& setTransparent() {
    transparent_ = true;
    return *this;
  }
  Material& setHandHarvestable() {
    handHarvestable_ = false;
    return *this;
  }
  Material& setBurnable() {
    burnable_ = true;
    return *this;
  }
  Material& setDestroyPistonBehavior() {
    pistonBehavior_ = 1;
    return *this;
  }
  Material& setUnpushablePistonBehavior() {
    pistonBehavior_ = 2;
    return *this;
  }

private:
  bool burnable_ = false;
  bool replaceable_ = false;
  bool transparent_ = false;
  bool handHarvestable_ = true;
  int pistonBehavior_ = 0;
};
} // namespace net::minecraft::block::material
