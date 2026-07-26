#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/mod/model/ModModels.hpp"
#endif
#include <string>
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
using namespace net::minecraft::mod::model;
#endif
int luaRegisterItem(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || !args.table(1)) {
  return args.fail("minecraft.register_item expects a spec table");
 }
 const int tableIndex = 1;
 ItemRegistrationSpec spec;
 spec.itemId = luaIntField(state, tableIndex, "id", 0);
 spec.texturePath = luaStringField(state, tableIndex, "texture", "");
 spec.maxCount = luaIntField(state, tableIndex, "max_count", 64);
 spec.maxDamage = luaIntField(state, tableIndex, "max_damage", 0);
 spec.translationKey = luaStringField(state, tableIndex, "translation_key", "");
 spec.displayName = luaStringField(state, tableIndex, "name", "");
 spec.ownerModId = mod->modId;
 api.getfield(state, tableIndex, "model");
#ifdef MINECRAFT_NATIVE_EXPORTS
 // Items still make model optional (plain sprite items have none), but when
 // present it must be a baked handle from minecraft.model.load/build — see
 // register_block's identical rule in LuaBlockBindings.cpp.
 const int modelType = api.type(state, -1);
 if(modelType == kLuaTNumber) {
  spec.bakedModel = luaIntField(state, tableIndex, "model", 0);
 } else if(modelType == kLuaTFunction) {
  api.settop(state, tableIndex);
  return args.fail("register_item: model must be a handle from minecraft.model.load; "
                    "draw-callback models are no longer supported");
 }
#endif
 api.settop(state, tableIndex);
 std::string error;
 if(!registerItemSpec(spec, error)) {
  return args.fail(error);
 }
 WorldRequiredMods::registerContentItem(mod->modId, spec.itemId);
 api.pushboolean(state, 1);
 return 1;
}
void installItemApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 bindModFunction(state, &mod, "_register_item", luaRegisterItem);
}
} // namespace net::minecraft::mod::runtime
