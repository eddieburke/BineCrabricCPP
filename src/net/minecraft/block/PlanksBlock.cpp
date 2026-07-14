#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
struct PlanksBlockRegistrar {
  static constexpr bool kRegisters = true;
  static constexpr int kBlockId = 5;
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Block::PLANKS, 4), {std::string("#"), '#', Block::LOG});
  }
  static void registerClass() {
    namespace mat = material;
    Block::PLANKS = (new Block(kBlockId, 4, mat::Material::WOOD))
                        ->setHardness(2.0f)
                        ->setResistance(5.0f)
                        ->setSoundGroup(&kWoodSound)
                        ->setTranslationKey("wood")
                        ->ignoreMetaUpdates();
  }
};
MC_REGISTER_BLOCK(PlanksBlockRegistrar)
} // namespace net::minecraft::block
