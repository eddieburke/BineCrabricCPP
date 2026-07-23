#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::block {
struct BricksBlockRegistrar {
 static constexpr bool kRegisters = true;
 static constexpr int kBlockId = 45;
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::BRICKS),
                                {std::string("##"), std::string("##"), '#', Item::byRawId(80)});
 }
 static void registerClass() {
  namespace mat = material;
  Block::BRICKS = (new Block(kBlockId, 7, mat::Material::STONE))
                      ->setHardness(2.0f)
                      ->setResistance(10.0f)
                      ->setTranslationKey("brick");
 }
};
MC_REGISTER_BLOCK(BricksBlockRegistrar)
} // namespace net::minecraft::block
