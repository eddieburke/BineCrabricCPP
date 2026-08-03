#include <string>
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/mod/model/ModModels.hpp"
#endif
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
using namespace net::minecraft::mod::model;
#endif
int luaRegisterBlock(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod == nullptr || !args.table(1)) {
  return args.fail("minecraft.register_block expects a spec table");
 }
 const int tableIndex = 1;
 BlockRegistrationSpec spec;
 spec.blockId = luaIntField(state, tableIndex, "id", 0);
 spec.texturePath = luaStringField(state, tableIndex, "texture", "");
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
  const int modelType = api.type(state, -1);
 if(modelType == kLuaTNumber) {
  spec.bakedModel = luaIntField(state, tableIndex, "model", 0);
 }
 api.settop(state, tableIndex);
 if(spec.bakedModel == 0) {
  return args.fail(modelType == kLuaTFunction
                       ? "register_block: model must be a handle from minecraft.model.load; "
                         "draw-callback models are no longer supported"
                       : "register_block: model is required and must be a handle from "
                         "minecraft.model.load (check that the load succeeded — it returns nil "
                         "plus an error message on failure)");
 }
#else
 api.settop(state, tableIndex);
#endif
  spec.itemTexturePath = luaStringField(state, tableIndex, "item_texture", "");
 std::string error;
 if(!registerBlockSpec(spec, error)) {
  return args.fail(error);
 }
 WorldRequiredMods::registerContentBlock(mod->modId, spec.blockId);
 api.pushboolean(state, 1);
 return 1;
}
void installBlockApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 bindModFunction(state, &mod, "_register_block", luaRegisterBlock);
}
} // namespace net::minecraft::mod::runtime
