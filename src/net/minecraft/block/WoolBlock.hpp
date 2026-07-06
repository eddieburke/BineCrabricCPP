#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class WoolBlock : public Block {
public:
  static constexpr int kBlockId = 35;
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

public:
  static void registerClass();
  static void registerBlockItems();
  WoolBlock() : Block(35, 64, material::Material::WOOL) {}
  [[nodiscard]] int getTexture(int /*side*/, int meta) const override {
    if(meta == 0) {
      return textureId;
    }
    meta = ~(meta & 0xF);
    return 113 + ((meta & 8) >> 3) + (meta & 7) * 16;
  }
  [[nodiscard]] static int getBlockMeta(int itemMeta) {
    return ~itemMeta & 0xF;
  }
  [[nodiscard]] static int getItemMeta(int blockMeta) {
    return ~blockMeta & 0xF;
  }

protected:
  [[nodiscard]] int getDroppedItemMeta(int blockMeta) const override {
    return blockMeta;
  }
};
} // namespace net::minecraft::block
