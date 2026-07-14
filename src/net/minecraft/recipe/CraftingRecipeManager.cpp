#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/recipe/ShapedRecipe.hpp"
#include "net/minecraft/recipe/ShapelessRecipe.hpp"
namespace net::minecraft::recipe {
namespace {
void sortRecipes(std::vector<std::unique_ptr<CraftingRecipe>>& recipes) {
  std::stable_sort(recipes.begin(),
                   recipes.end(),
                   [](const std::unique_ptr<CraftingRecipe>& left, const std::unique_ptr<CraftingRecipe>& right) {
                     const bool leftShapeless = dynamic_cast<const ShapelessRecipe*>(left.get()) != nullptr;
                     const bool rightShapeless = dynamic_cast<const ShapelessRecipe*>(right.get()) != nullptr;
                     const bool leftShaped = dynamic_cast<const ShapedRecipe*>(left.get()) != nullptr;
                     const bool rightShaped = dynamic_cast<const ShapedRecipe*>(right.get()) != nullptr;
                     if(leftShapeless && rightShaped) {
                       return false;
                     }
                     if(rightShapeless && leftShaped) {
                       return true;
                     }
                     if(right->getSize() < left->getSize()) {
                       return false;
                     }
                     if(right->getSize() > left->getSize()) {
                       return true;
                     }
                     return false;
                   });
}
} // namespace
CraftingRecipeManager& CraftingRecipeManager::getInstance() {
  static CraftingRecipeManager instance;
  return instance;
}
CraftingRecipeManager::CraftingRecipeManager() = default;
void CraftingRecipeManager::finishRegistration() {
  sortRecipes(recipes);
}
void CraftingRecipeManager::addShapedRecipe(ItemStack output, std::vector<RecipeArg> input) {
  std::string pattern;
  int width = 0;
  int height = 0;
  std::size_t index = 0;
  if(index < input.size() && input[index].kind() == RecipeArg::Kind::Rows) {
    for(const std::string& row : input[index].rows()) {
      ++height;
      width = static_cast<int>(row.length());
      pattern += row;
    }
    ++index;
  } else {
    while(index < input.size() && input[index].kind() == RecipeArg::Kind::Row) {
      const std::string& row = input[index].row();
      ++height;
      width = static_cast<int>(row.length());
      pattern += row;
      ++index;
    }
  }
  std::unordered_map<char, ItemStack> ingredients;
  while(index + 1 < input.size() && input[index].kind() == RecipeArg::Kind::Key) {
    const char key = input[index].key();
    const RecipeArg& value = input[index + 1];
    ItemStack stack;
    if(value.kind() == RecipeArg::Kind::Item && value.item() != nullptr) {
      stack = ItemStack(value.item(), 1, 0);
    } else if(value.kind() == RecipeArg::Kind::Block && value.block() != nullptr) {
      stack = ItemStack(value.block(), 1, -1);
    } else if(value.kind() == RecipeArg::Kind::Stack) {
      stack = value.stack().copy();
    }
    ingredients[key] = stack;
    index += 2;
  }
  std::vector<ItemStack> grid(static_cast<std::size_t>(width * height));
  for(int i = 0; i < width * height; ++i) {
    const char key = pattern[static_cast<std::size_t>(i)];
    const auto it = ingredients.find(key);
    grid[static_cast<std::size_t>(i)] = it != ingredients.end() ? it->second.copy() : ItemStack{};
  }
  recipes.push_back(std::make_unique<ShapedRecipe>(width, height, std::move(grid), output));
  if(mod::ModLifecycle::frozen()) {
    sortRecipes(recipes);
  }
}
void CraftingRecipeManager::addShapelessRecipe(ItemStack output, std::vector<RecipeArg> input) {
  std::vector<ItemStack> ingredients;
  ingredients.reserve(input.size());
  for(const RecipeArg& arg : input) {
    if(arg.kind() == RecipeArg::Kind::Stack) {
      ingredients.push_back(arg.stack().copy());
    } else if(arg.kind() == RecipeArg::Kind::Item && arg.item() != nullptr) {
      ingredients.push_back(ItemStack(arg.item()));
    } else if(arg.kind() == RecipeArg::Kind::Block && arg.block() != nullptr) {
      ingredients.push_back(ItemStack(arg.block()));
    } else {
      throw std::runtime_error("Invalid shapeless recipy!");
    }
  }
  recipes.push_back(std::make_unique<ShapelessRecipe>(output, std::move(ingredients)));
  if(mod::ModLifecycle::frozen()) {
    sortRecipes(recipes);
  }
}
ItemStack CraftingRecipeManager::craft(const CraftingInventory& craftingInventory) const {
  for(const std::unique_ptr<CraftingRecipe>& recipe : recipes) {
    if(recipe != nullptr && recipe->matches(craftingInventory)) {
      return recipe->craft(craftingInventory);
    }
  }
  return {};
}
} // namespace net::minecraft::recipe
