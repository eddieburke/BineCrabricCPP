#include "net/minecraft/mod/runtime/LuaItemBindings.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/mod/model/ModModels.hpp"
#endif
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <cmath>
#include <string>
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
using namespace net::minecraft::mod::model;
#endif
int luaRegisterItem(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
  api.pushboolean(state, 0);
  api.pushstring(state, "minecraft.register_item expects a spec table");
  return 2;
 }
 const int tableIndex = 1;
 ItemRegistrationSpec spec;
 spec.itemId = luaIntField(state, tableIndex, "id", 0);
 spec.texturePath = luaStringField(state, tableIndex, "texture", "");
 spec.itemsTextureId = luaIntField(state, tableIndex, "texture_id", -1);
 spec.maxCount = luaIntField(state, tableIndex, "max_count", 64);
 spec.maxDamage = luaIntField(state, tableIndex, "max_damage", 0);
 spec.translationKey = luaStringField(state, tableIndex, "translation_key", "");
 spec.displayName = luaStringField(state, tableIndex, "name", "");
 spec.ownerModId = mod->modId;
 api.getfield(state, tableIndex, "model");
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(api.type(state, -1) == kLuaTNumber) {
  spec.bakedModel = luaIntField(state, tableIndex, "model", 0);
 } else if(api.type(state, -1) == kLuaTFunction) {
  const int modelIndex = api.gettop(state);
  std::string modelError;
  if(!parseModelCallback(state, modelIndex, spec.modelRef, modelError)) {
   api.settop(state, tableIndex);
   api.pushboolean(state, 0);
   api.pushstring(state, modelError.c_str());
   return 2;
  }
 }
#endif
 api.settop(state, tableIndex);
 std::string error;
 if(!registerItemSpec(spec, error)) {
  if(spec.modelRef != kLuaNoRef) {
   api.unref(state, kLuaRegistryIndex, spec.modelRef);
  }
  api.pushboolean(state, 0);
  api.pushstring(state, error.c_str());
  return 2;
 }
 if(spec.modelRef != kLuaNoRef) {
  mod->itemModelCallbackRefs.push_back(spec.modelRef);
 }
 api.pushboolean(state, 1);
 return 1;
}
void installItemApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 bindModFunction(state, &mod, "_register_item", luaRegisterItem);
}
} // namespace net::minecraft::mod::runtime
