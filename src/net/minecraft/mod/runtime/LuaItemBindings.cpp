#include "net/minecraft/mod/runtime/LuaItemBindings.hpp"

#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#endif
#include <cmath>
#include <string>

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
namespace {
bool readItemBox(lua_State* state, int tableIndex, ModelBox& box) {
    LuaApi& api = luaApi();
    if (api.type(state, tableIndex) != kLuaTTable) {
        return false;
    }
    box.minX = luaFloatField(state, tableIndex, "min_x", luaFloatAt(state, tableIndex, 1, box.minX));
    box.minY = luaFloatField(state, tableIndex, "min_y", luaFloatAt(state, tableIndex, 2, box.minY));
    box.minZ = luaFloatField(state, tableIndex, "min_z", luaFloatAt(state, tableIndex, 3, box.minZ));
    box.maxX = luaFloatField(state, tableIndex, "max_x", luaFloatAt(state, tableIndex, 1, box.maxX));
    box.maxY = luaFloatField(state, tableIndex, "max_y", luaFloatAt(state, tableIndex, 2, box.maxY));
    box.maxZ = luaFloatField(state, tableIndex, "max_z", luaFloatAt(state, tableIndex, 3, box.maxZ));
    if (api.getfield(state, tableIndex, "min") == kLuaTTable) {
        const int minIndex = api.gettop(state);
        box.minX = luaFloatAt(state, minIndex, 1, box.minX);
        box.minY = luaFloatAt(state, minIndex, 2, box.minY);
        box.minZ = luaFloatAt(state, minIndex, 3, box.minZ);
    }
    api.settop(state, tableIndex);
    if (api.getfield(state, tableIndex, "max") == kLuaTTable) {
        const int maxIndex = api.gettop(state);
        box.maxX = luaFloatAt(state, maxIndex, 1, box.maxX);
        box.maxY = luaFloatAt(state, maxIndex, 2, box.maxY);
        box.maxZ = luaFloatAt(state, maxIndex, 3, box.maxZ);
    }
    api.settop(state, tableIndex);
    return std::isfinite(box.minX) && std::isfinite(box.minY) && std::isfinite(box.minZ) && std::isfinite(box.maxX) &&
           std::isfinite(box.maxY) && std::isfinite(box.maxZ) && box.minX >= 0.0f && box.minY >= 0.0f &&
           box.minZ >= 0.0f && box.maxX <= 1.0f && box.maxY <= 1.0f && box.maxZ <= 1.0f && box.minX < box.maxX &&
           box.minY < box.maxY && box.minZ < box.maxZ;
}

bool parseItemModel(
    lua_State* state, int tableIndex, ItemRegistrationSpec& spec, int& manualDrawRef, std::string& error) {
    LuaApi& api = luaApi();
    manualDrawRef = kLuaNoRef;
    std::string type = toLowerCopy(luaStringField(state, tableIndex, "type", ""));
    if (type.empty()) {
        api.getfield(state, tableIndex, "draw");
        if (api.type(state, -1) == kLuaTFunction) {
            type = "manual";
            api.settop(state, tableIndex);
        } else {
            api.settop(state, tableIndex);
            api.getfield(state, tableIndex, "boxes");
            if (api.type(state, -1) == kLuaTTable) {
                type = "box_list";
            }
            api.settop(state, tableIndex);
        }
    }
    if (type.empty() || type == "flat" || type == "simple") {
        spec.model.type = LuaItemModelSpec::Type::Flat;
        return true;
    }
    if (type == "box_list" || type == "boxes") {
        spec.model.type = LuaItemModelSpec::Type::BoxList;
        api.getfield(state, tableIndex, "boxes");
        if (api.type(state, -1) != kLuaTTable) {
            api.settop(state, tableIndex);
            error = "box_list item model requires boxes array";
            return false;
        }
        const int boxesTable = api.gettop(state);
        for (int i = 1; i <= 32; ++i) {
            api.rawgeti(state, boxesTable, i);
            if (api.type(state, -1) == kLuaTNil) {
                api.settop(state, -2);
                break;
            }
            if (api.type(state, -1) == kLuaTTable) {
                ModelBox box;
                if (readItemBox(state, api.gettop(state), box)) {
                    spec.model.boxes.push_back(box);
                }
            }
            api.settop(state, -2);
        }
        api.settop(state, tableIndex);
        if (spec.model.boxes.empty()) {
            error = "box_list item model requires at least one box";
            return false;
        }
        return true;
    }
    if (type == "manual" || type == "tessellated" || type == "custom") {
        spec.model.type = LuaItemModelSpec::Type::Manual;
        api.getfield(state, tableIndex, "draw");
        if (api.type(state, -1) != kLuaTFunction) {
            api.settop(state, tableIndex);
            error = "manual item model requires a draw function";
            return false;
        }
        manualDrawRef = api.ref(state, kLuaRegistryIndex);
        api.settop(state, tableIndex);
        if (manualDrawRef == kLuaNoRef) {
            error = "failed to retain manual item model callback";
            return false;
        }
        return true;
    }
    error = "unknown item model type: " + type;
    return false;
}
}  // namespace

int luaRegisterItem(lua_State* state) {
    LuaApi& api = luaApi();
    ModHost::LoadedLuaMod* mod = currentLuaMod(state);
    if (mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
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
    int manualDrawRef = kLuaNoRef;
    api.getfield(state, tableIndex, "model");
    if (api.type(state, -1) == kLuaTTable) {
        const int modelIndex = api.gettop(state);
        std::string modelError;
        if (!parseItemModel(state, modelIndex, spec, manualDrawRef, modelError)) {
            api.settop(state, tableIndex);
            api.pushboolean(state, 0);
            api.pushstring(state, modelError.c_str());
            return 2;
        }
    }
    api.settop(state, tableIndex);
    spec.manualDrawRef = manualDrawRef;
    std::string error;
    if (!registerItemSpec(spec, error)) {
        if (manualDrawRef != kLuaNoRef) {
            api.unref(state, kLuaRegistryIndex, manualDrawRef);
        }
        api.pushboolean(state, 0);
        api.pushstring(state, error.c_str());
        return 2;
    }
    if (manualDrawRef != kLuaNoRef) {
        mod->itemModelCallbackRefs.push_back(manualDrawRef);
    }
    api.pushboolean(state, 1);
    return 1;
}

void installItemApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
    bindModFunction(state, &mod, "_register_item", luaRegisterItem);
}
#else
void installItemApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
    (void) state;
    (void) mod;
}
#endif
}  // namespace net::minecraft::mod::runtime
