#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/glowstone_dust.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class GlowstoneBlock : public Block {
public:
  static constexpr int kBlockId = 89;
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

public:
  static void registerClass();
  GlowstoneBlock(int id, int textureId, Material& material) : Block(id, textureId, material) {}
  [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override {
    return 2 + random.nextInt(3);
  }
  [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override {
    return Item::byRawId(92) != nullptr ? Item::byRawId(92)->id : 348;
  }
};
} // namespace net::minecraft::block