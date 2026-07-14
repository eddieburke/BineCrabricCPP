#pragma once
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class BookshelfBlock : public Block {
public:
  static constexpr int kBlockId = 47;
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

public:
  static void registerClass();
  BookshelfBlock(int id, int textureId) : Block(id, textureId, material::Material::WOOD) {
  }
  [[nodiscard]] int getTexture(int side) const override {
    return Block::textureForSide(side, textureId, 4, 4);
  }
  [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
    return 0;
  }
};
} // namespace net::minecraft::block