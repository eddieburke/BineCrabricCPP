#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include <vector>
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::block::Block;
using net::minecraft::recipe::CraftingRecipeManager;
using net::minecraft::recipe::RecipeArg;
std::vector<ShapedRecipeSpec>& pendingShapedRecipes() {
  static std::vector<ShapedRecipeSpec> value;
  return value;
}
void registerShapedRecipeImpl(const ShapedRecipeSpec& spec) {
  Block* output = Block::BLOCKS[static_cast<std::size_t>(spec.outputBlockId)];
  net::minecraft::Item* ingredient = net::minecraft::Item::byId(spec.ingredientItemId);
  if(output == nullptr || ingredient == nullptr) {
    return;
  }
  std::vector<RecipeArg> args;
  args.reserve(spec.pattern.size() + 2);
  for(const std::string& row : spec.pattern) {
    args.emplace_back(row);
  }
  args.emplace_back(spec.key);
  args.emplace_back(ingredient);
  CraftingRecipeManager::getInstance().addShapedRecipe(net::minecraft::ItemStack(output, spec.outputCount),
                                                       std::move(args));
}
void initAllShapedRecipes() {
  for(const ShapedRecipeSpec& spec : pendingShapedRecipes()) {
    registerShapedRecipeImpl(spec);
  }
}
void ensureRecipeBatchQueued() {
  static bool queued = false;
  if(queued) {
    return;
  }
  queued = true;
  registry::Registry::enqueue(mod::LifecyclePhase::CraftingRecipeRegistration, 50000, initAllShapedRecipes);
}
} // namespace
bool registerShapedRecipe(const ShapedRecipeSpec& spec, std::string& error) {
  if(spec.outputBlockId <= 0 || spec.outputBlockId >= Block::BLOCK_COUNT) {
    error = "register_shaped_recipe requires output_block_id";
    return false;
  }
  if(spec.outputCount <= 0 || spec.outputCount > 64) {
    error = "register_shaped_recipe output_count must be between 1 and 64";
    return false;
  }
  if(spec.pattern.empty() || spec.pattern.size() > 3) {
    error = "register_shaped_recipe requires pattern rows";
    return false;
  }
  const std::size_t width = spec.pattern.front().size();
  if(width == 0 || width > 3) {
    error = "register_shaped_recipe rows must contain 1 to 3 columns";
    return false;
  }
  bool usesIngredient = false;
  for(const std::string& row : spec.pattern) {
    if(row.size() != width) {
      error = "register_shaped_recipe rows must have equal widths";
      return false;
    }
    usesIngredient = usesIngredient || row.find(spec.key) != std::string::npos;
  }
  if(!usesIngredient) {
    error = "register_shaped_recipe pattern does not use the ingredient key";
    return false;
  }
  if(spec.ingredientItemId <= 0) {
    error = "register_shaped_recipe requires item_id";
    return false;
  }
  if(registry::Registry::isBootstrapped()) {
    error = "register_shaped_recipe must run while Lua mod scripts load at startup";
    return false;
  }
  pendingShapedRecipes().push_back(spec);
  ensureRecipeBatchQueued();
  return true;
}
} // namespace net::minecraft::mod::lua
