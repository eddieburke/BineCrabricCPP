#include "net/minecraft/mod/lua/LuaRecipeRegistry.hpp"
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::block::Block;
using net::minecraft::recipe::CraftingRecipeManager;
using net::minecraft::recipe::RecipeArg;
std::vector<ShapedRecipeSpec>& pendingShapedRecipes() {
  static std::vector<ShapedRecipeSpec> value;
  return value;
}
[[nodiscard]] net::minecraft::ItemStack shapedRecipeOutput(const ShapedRecipeSpec& spec) {
  if(spec.outputItemId > 0) {
    net::minecraft::Item* item = net::minecraft::Item::byId(spec.outputItemId);
    if(item == nullptr) {
      return {};
    }
    return net::minecraft::ItemStack(item, spec.outputCount);
  }
  Block* block = Block::BLOCKS[static_cast<std::size_t>(spec.outputBlockId)];
  if(block == nullptr) {
    return {};
  }
  return net::minecraft::ItemStack(block, spec.outputCount);
}
void registerShapedRecipeImpl(const ShapedRecipeSpec& spec) {
  const net::minecraft::ItemStack output = shapedRecipeOutput(spec);
  net::minecraft::Item* ingredient = net::minecraft::Item::byId(spec.ingredientItemId);
  if(output.empty() || ingredient == nullptr) {
    return;
  }
  std::vector<RecipeArg> args;
  args.reserve(spec.pattern.size() + 2);
  for(const std::string& row : spec.pattern) {
    args.emplace_back(row);
  }
  args.emplace_back(spec.key);
  args.emplace_back(ingredient);
  CraftingRecipeManager::getInstance().addShapedRecipe(output, std::move(args));
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
  registry::Registry::enqueue(mod::LifecyclePhase::PostInit, 50000, initAllShapedRecipes);
}
[[nodiscard]] bool validateShapedRecipeSpec(const ShapedRecipeSpec& spec, std::string& error) {
  const bool hasBlockOutput = spec.outputBlockId > 0 && spec.outputBlockId < Block::BLOCK_COUNT;
  const bool hasItemOutput = spec.outputItemId > 0;
  if(!hasBlockOutput && !hasItemOutput) {
    error = "shaped recipe requires output_block_id or output_item_id";
    return false;
  }
  if(hasBlockOutput && hasItemOutput) {
    error = "shaped recipe accepts only one of output_block_id or output_item_id";
    return false;
  }
  if(spec.outputCount <= 0 || spec.outputCount > 64) {
    error = "shaped recipe output_count must be between 1 and 64";
    return false;
  }
  if(spec.pattern.empty() || spec.pattern.size() > 3) {
    error = "shaped recipe requires pattern rows";
    return false;
  }
  const std::size_t width = spec.pattern.front().size();
  if(width == 0 || width > 3) {
    error = "shaped recipe rows must contain 1 to 3 columns";
    return false;
  }
  bool usesIngredient = false;
  for(const std::string& row : spec.pattern) {
    if(row.size() != width) {
      error = "shaped recipe rows must have equal widths";
      return false;
    }
    usesIngredient = usesIngredient || row.find(spec.key) != std::string::npos;
  }
  if(!usesIngredient) {
    error = "shaped recipe pattern does not use the ingredient key";
    return false;
  }
  if(spec.ingredientItemId <= 0) {
    error = "shaped recipe requires item_id";
    return false;
  }
  return true;
}
} // namespace
bool registerShapedRecipe(const ShapedRecipeSpec& spec, std::string& error) {
  if(!validateShapedRecipeSpec(spec, error)) {
    return false;
  }
  if(registry::Registry::isBootstrapped()) {
    if(shapedRecipeOutput(spec).empty() || net::minecraft::Item::byId(spec.ingredientItemId) == nullptr) {
      error = "shaped recipe output or ingredient is unknown";
      return false;
    }
    registerShapedRecipeImpl(spec);
    return true;
  }
  pendingShapedRecipes().push_back(spec);
  ensureRecipeBatchQueued();
  return true;
}
} // namespace net::minecraft::mod::lua
