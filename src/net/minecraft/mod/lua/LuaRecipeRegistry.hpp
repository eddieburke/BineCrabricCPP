#pragma once
#include <string>
#include <vector>
namespace net::minecraft::mod::lua {
struct ShapedRecipeSpec {
 int outputBlockId = 0;
 int outputItemId = 0;
 int outputCount = 1;
 std::vector<std::string> pattern;
 char key = '#';
 int ingredientItemId = 0;
};
bool registerShapedRecipe(const ShapedRecipeSpec& spec, std::string& error);
} // namespace net::minecraft::mod::lua
