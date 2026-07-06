#pragma once
#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipe.hpp"
#include <memory>
#include <string>
#include <vector>
namespace net::minecraft::recipe {
// Wrapper for Java's Object... recipe arguments: a pattern row, a String[] of
// rows, a key character, or an Item/Block/ItemStack ingredient.
class RecipeArg {
public:
  enum class Kind { Rows,
                    Row,
                    Key,
                    Item,
                    Block,
                    Stack };
  RecipeArg(std::vector<std::string> rows) : kind_(Kind::Rows), rows_(std::move(rows)) {}
  RecipeArg(const char* row) : kind_(Kind::Row), row_(row) {}
  RecipeArg(std::string row) : kind_(Kind::Row), row_(std::move(row)) {}
  RecipeArg(char key) : kind_(Kind::Key), key_(key) {}
  RecipeArg(net::minecraft::Item* item) : kind_(Kind::Item), item_(item) {}
  RecipeArg(net::minecraft::block::Block* block) : kind_(Kind::Block), block_(block) {}
  RecipeArg(ItemStack stack) : kind_(Kind::Stack), stack_(stack) {}
  [[nodiscard]] Kind kind() const {
    return kind_;
  }
  [[nodiscard]] const std::vector<std::string>& rows() const {
    return rows_;
  }
  [[nodiscard]] const std::string& row() const {
    return row_;
  }
  [[nodiscard]] char key() const {
    return key_;
  }
  [[nodiscard]] net::minecraft::Item* item() const {
    return item_;
  }
  [[nodiscard]] net::minecraft::block::Block* block() const {
    return block_;
  }
  [[nodiscard]] const ItemStack& stack() const {
    return stack_;
  }

private:
  Kind kind_;
  std::vector<std::string> rows_{};
  std::string row_{};
  char key_ = 0;
  net::minecraft::Item* item_ = nullptr;
  net::minecraft::block::Block* block_ = nullptr;
  ItemStack stack_{};
};
// Faithful port of net.minecraft.recipe.CraftingRecipeManager (beta 1.7.3).
class CraftingRecipeManager {
public:
  [[nodiscard]] static CraftingRecipeManager& getInstance();
  static void registerVanillaRecipes();
  void finishRegistration();
  void addShapedRecipe(ItemStack output, std::vector<RecipeArg> input);
  void addShapelessRecipe(ItemStack output, std::vector<RecipeArg> input);
  [[nodiscard]] ItemStack craft(const CraftingInventory& craftingInventory) const;
  [[nodiscard]] const std::vector<std::unique_ptr<CraftingRecipe>>& getRecipes() const {
    return recipes;
  }

private:
  CraftingRecipeManager();
  std::vector<std::unique_ptr<CraftingRecipe>> recipes;
};
} // namespace net::minecraft::recipe
