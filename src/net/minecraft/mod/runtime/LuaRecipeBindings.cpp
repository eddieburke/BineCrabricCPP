#include <string>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRecipeRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
int luaRegisterShapedRecipe(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || !args.table(1)) {
  return args.fail("minecraft.register_shaped_recipe expects a spec table");
 }
 const int tableIndex = 1;
 lua::ShapedRecipeSpec spec;
 spec.outputBlockId = luaIntField(state, tableIndex, "output_block_id", 0);
 spec.outputItemId = luaIntField(state, tableIndex, "output_item_id", 0);
 spec.outputCount = luaIntField(state, tableIndex, "output_count", 1);
 spec.ingredientItemId = luaIntField(state, tableIndex, "item_id", 0);
 const std::string keyText = luaStringField(state, tableIndex, "key", "#");
 spec.key = keyText.empty() ? '#' : keyText.front();
 api.getfield(state, tableIndex, "ingredients");
 if(api.type(state, -1) == kLuaTTable) {
  api.pushnil(state);
  while(api.next(state, -2) != 0) {
   if(api.type(state, -2) == kLuaTString && api.type(state, -1) == kLuaTNumber) {
    const std::string k = luaString(state, -2, "");
    const int v = static_cast<int>(api.tointegerx(state, -1, nullptr));
    if(!k.empty() && v > 0) {
     spec.extraIngredients.emplace_back(k.front(), v);
    }
   }
   api.settop(state, -2);
  }
 }
 api.settop(state, tableIndex);
 api.getfield(state, tableIndex, "pattern");
 if(api.type(state, -1) == kLuaTTable) {
  const int patternTable = api.gettop(state);
  for(int i = 1; i <= 8; ++i) {
   api.rawgeti(state, patternTable, i);
   if(api.type(state, -1) == kLuaTNil) {
    api.settop(state, -2);
    break;
   }
   if(api.type(state, -1) == kLuaTString) {
    spec.pattern.push_back(luaString(state, -1, ""));
   }
   api.settop(state, -2);
  }
 }
  api.settop(state, tableIndex);
  spec.ownerModId = mod != nullptr ? mod->modId : std::string();
  std::string error;
  if(!lua::registerShapedRecipe(spec, error)) {
   return args.fail(error);
  }
 api.pushboolean(state, 1);
 return 1;
}
} // namespace
void installRecipeApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 bindModFunction(state, &mod, "_register_shaped_recipe", luaRegisterShapedRecipe);
}
} // namespace net::minecraft::mod::runtime
