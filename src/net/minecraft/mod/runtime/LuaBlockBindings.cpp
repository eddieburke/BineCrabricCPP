#include "net/minecraft/mod/runtime/LuaBlockBindings.hpp"
#include <cmath>
#include <string>
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/mod/model/ModModels.hpp"
#endif
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
using namespace net::minecraft::mod::model;
#endif
int luaRegisterBlock(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
  api.pushboolean(state, 0);
  api.pushstring(state, "minecraft.register_block expects a spec table");
  return 2;
 }
 const int tableIndex = 1;
 BlockRegistrationSpec spec;
 spec.blockId = luaIntField(state, tableIndex, "id", 0);
 spec.texturePath = luaStringField(state, tableIndex, "texture", "");
 spec.terrainTextureId = luaIntField(state, tableIndex, "texture_id", -1);
 spec.hardness = luaFloatField(state, tableIndex, "hardness", 1.0f);
 spec.resistance = luaFloatField(state, tableIndex, "resistance", 1.0f);
 spec.luminance = luaFloatField(state, tableIndex, "luminance", 0.0f);
 spec.translationKey = luaStringField(state, tableIndex, "translation_key", "");
 spec.displayName = luaStringField(state, tableIndex, "name", "");
 spec.material = luaStringField(state, tableIndex, "material", "stone");
 spec.ownerModId = mod->modId;
 spec.opaque = luaBoolField(state, tableIndex, "opaque", true);
 spec.fullCube = luaBoolField(state, tableIndex, "full_cube", true);
 spec.translucent = luaBoolField(state, tableIndex, "translucent", !spec.opaque);
 spec.collisionHeight = luaFloatField(state, tableIndex, "collision_height", 1.0f);
 spec.stackOnSame = luaBoolField(state, tableIndex, "stack_on_same", false);
 spec.requiresSolidBelow = luaBoolField(state, tableIndex, "requires_solid_below", true);
 spec.coordinateBounds = luaBoolField(state, tableIndex, "coordinate_bounds", false);
 spec.coordinateColor = luaBoolField(state, tableIndex, "coordinate_color", false);
 spec.boundsPadding = luaFloatField(state, tableIndex, "bounds_padding", 0.0625f);
 spec.boundsOffset = luaFloatField(state, tableIndex, "bounds_offset", 0.1f);
 spec.minScale = luaFloatField(state, tableIndex, "min_scale", 0.9f);
 spec.maxScale = luaFloatField(state, tableIndex, "max_scale", 1.1f);
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
  if(spec.modelRef != kLuaNoRef) {
   mod->blockModelCallbackRefs.push_back(spec.modelRef);
  }
 }
#endif
 api.settop(state, tableIndex);
 api.getfield(state, tableIndex, "item");
 if(api.type(state, -1) == kLuaTTable) {
  const int itemTable = api.gettop(state);
  spec.itemTexturePath = luaStringField(state, itemTable, "texture", "");
  spec.itemTextureId = luaIntField(state, itemTable, "texture_id", -1);
  api.settop(state, itemTable);
 }
 api.settop(state, tableIndex);
 spec.tileEntityId = luaStringField(state, tableIndex, "tile_entity", "");
 api.settop(state, tableIndex);
 std::string error;
 if(!registerBlockSpec(spec, error)) {
  if(spec.modelRef != kLuaNoRef) {
   api.unref(state, kLuaRegistryIndex, spec.modelRef);
  }
  api.pushboolean(state, 0);
  api.pushstring(state, error.c_str());
  return 2;
 }
 WorldRequiredMods::registerContentBlock(mod->modId, spec.blockId);
 api.pushboolean(state, 1);
 return 1;
}
void installBlockApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 bindModFunction(state, &mod, "_register_block", luaRegisterBlock);
}
} // namespace net::minecraft::mod::runtime
