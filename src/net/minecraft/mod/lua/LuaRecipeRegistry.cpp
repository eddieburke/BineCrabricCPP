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
 if(output.empty()) {
  return;
 }
 const bool hasMultiIngredients = !spec.extraIngredients.empty();
 const int pairCount = hasMultiIngredients ? static_cast<int>(spec.extraIngredients.size()) : 1;
 std::vector<RecipeArg> args;
 args.reserve(spec.pattern.size() + static_cast<std::size_t>(pairCount) * 2);
 for(const std::string& row : spec.pattern) {
  args.emplace_back(row);
 }
 auto addIngredient = [&](char key, int itemId) {
  net::minecraft::Item* item = net::minecraft::Item::byId(itemId);
  if(item != nullptr) {
   args.emplace_back(key);
   args.emplace_back(item);
  }
 };
 if(hasMultiIngredients) {
  for(const auto& [key, itemId] : spec.extraIngredients) {
   addIngredient(key, itemId);
  }
 } else {
  addIngredient(spec.key, spec.ingredientItemId);
 }
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
  const bool hasMultiIngredients = !spec.extraIngredients.empty();
  bool usesIngredient = !hasMultiIngredients;
  for(const std::string& row : spec.pattern) {
   if(row.size() != width) {
    error = "shaped recipe rows must have equal widths";
    return false;
   }
   if(!hasMultiIngredients) {
    usesIngredient = usesIngredient || row.find(spec.key) != std::string::npos;
   }
  }
  if(!hasMultiIngredients) {
   if(!usesIngredient) {
    error = "shaped recipe pattern does not use the ingredient key";
    return false;
   }
   if(spec.ingredientItemId <= 0) {
    error = "shaped recipe requires item_id";
    return false;
   }
  } else {
   for(const auto& [key, id] : spec.extraIngredients) {
    if(id <= 0) {
     error = "shaped recipe ingredient item_id must be positive";
     return false;
    }
    bool found = false;
    for(const std::string& row : spec.pattern) {
     found = found || row.find(key) != std::string::npos;
    }
    if(!found) {
     error = std::string("shaped recipe key '") + key + "' not found in pattern";
     return false;
    }
   }
  }
 return true;
}
} // namespace
bool registerShapedRecipe(const ShapedRecipeSpec& spec, std::string& error) {
 if(!validateShapedRecipeSpec(spec, error)) {
  return false;
 }
 if(registry::Registry::isBootstrapped()) {
  if(shapedRecipeOutput(spec).empty()) {
   error = "shaped recipe output is unknown";
   return false;
  }
  const bool hasMulti = !spec.extraIngredients.empty();
  if(!hasMulti && net::minecraft::Item::byId(spec.ingredientItemId) == nullptr) {
   error = "shaped recipe ingredient is unknown";
   return false;
  }
  if(hasMulti) {
   for(const auto& [k, id] : spec.extraIngredients) {
    if(net::minecraft::Item::byId(id) == nullptr) {
     error = std::string("shaped recipe ingredient '") + k + "' (id " + std::to_string(id) + ") is unknown";
     return false;
    }
   }
  }
  registerShapedRecipeImpl(spec);
  return true;
 }
 pendingShapedRecipes().push_back(spec);
 ensureRecipeBatchQueued();
 return true;
}
} // namespace net::minecraft::mod::lua
