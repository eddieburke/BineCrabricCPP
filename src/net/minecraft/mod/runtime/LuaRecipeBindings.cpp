#include "net/minecraft/mod/runtime/LuaRecipeBindings.hpp"

#include <string>

#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRecipeRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;

namespace {
int luaRegisterShapedRecipe(lua_State* state) {
    LuaApi& api = luaApi();
    ModHost::LoadedLuaMod* mod = currentLuaMod(state);
    if (mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
        api.pushboolean(state, 0);
        api.pushstring(state, "minecraft.register_shaped_recipe expects a spec table");
        return 2;
    }
    const int tableIndex = 1;
    lua::ShapedRecipeSpec spec;
    spec.outputBlockId = luaIntField(state, tableIndex, "output_block_id", 0);
    spec.outputItemId = luaIntField(state, tableIndex, "output_item_id", 0);
    spec.outputCount = luaIntField(state, tableIndex, "output_count", 1);
    spec.ingredientItemId = luaIntField(state, tableIndex, "item_id", 0);
    const std::string keyText = luaStringField(state, tableIndex, "key", "#");
    spec.key = keyText.empty() ? '#' : keyText.front();
    api.getfield(state, tableIndex, "pattern");
    if (api.type(state, -1) == kLuaTTable) {
        const int patternTable = api.gettop(state);
        for (int i = 1; i <= 8; ++i) {
            api.rawgeti(state, patternTable, i);
            if (api.type(state, -1) == kLuaTNil) {
                api.settop(state, -2);
                break;
            }
            if (api.type(state, -1) == kLuaTString) {
                spec.pattern.push_back(luaString(state, -1, ""));
            }
            api.settop(state, -2);
        }
    }
    api.settop(state, tableIndex);
    std::string error;
    if (!lua::registerShapedRecipe(spec, error)) {
        api.pushboolean(state, 0);
        api.pushstring(state, error.c_str());
        return 2;
    }
    api.pushboolean(state, 1);
    return 1;
}
}  // namespace

void installRecipeApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
    bindModFunction(state, &mod, "_register_shaped_recipe", luaRegisterShapedRecipe);
}
}  // namespace net::minecraft::mod::runtime
