#pragma once
#include <string>
#include <utility>
#include <vector>
namespace net::minecraft::mod::lua {
struct ShapedRecipeSpec {
 int outputBlockId = 0;
 int outputItemId = 0;
 int outputCount = 1;
 std::vector<std::string> pattern;
 char key = '#';
 int ingredientItemId = 0;
 std::vector<std::pair<char, int>> extraIngredients;
 // Owning mod (for live re-registration idempotency) and whether the recipe has
 // already been handed to CraftingRecipeManager this session.
 std::string ownerModId;
 bool instantiated = false;
};
bool registerShapedRecipe(const ShapedRecipeSpec& spec, std::string& error);
} // namespace net::minecraft::mod::lua
