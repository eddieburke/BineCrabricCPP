#include "net/minecraft/mod/runtime/LuaBlockBindings.hpp"

#include <cmath>
#include <string>

#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"

namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;

namespace {
bool readVec3Field(lua_State* state, int tableIndex, const char* key, float& x, float& y, float& z) {
    LuaApi& api = luaApi();
    api.getfield(state, tableIndex, key);
    if (api.type(state, -1) != kLuaTTable) {
        api.settop(state, -2);
        return false;
    }
    const int vecTable = api.gettop(state);
    x = luaFloatField(state, vecTable, "x", luaFloatAt(state, vecTable, 1, x));
    y = luaFloatField(state, vecTable, "y", luaFloatAt(state, vecTable, 2, y));
    z = luaFloatField(state, vecTable, "z", luaFloatAt(state, vecTable, 3, z));
    api.settop(state, -2);
    return true;
}

bool readModelBox(lua_State* state, int tableIndex, ModelBox& box) {
    float minX = box.minX;
    float minY = box.minY;
    float minZ = box.minZ;
    float maxX = box.maxX;
    float maxY = box.maxY;
    float maxZ = box.maxZ;
    if (!readVec3Field(state, tableIndex, "min", minX, minY, minZ)) {
        return false;
    }
    if (!readVec3Field(state, tableIndex, "max", maxX, maxY, maxZ)) {
        return false;
    }
    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(minZ) || !std::isfinite(maxX) ||
        !std::isfinite(maxY) || !std::isfinite(maxZ) || minX < 0.0f || minY < 0.0f || minZ < 0.0f || maxX > 1.0f ||
        maxY > 1.0f || maxZ > 1.0f || minX >= maxX || minY >= maxY || minZ >= maxZ) {
        return false;
    }
    box.minX = minX;
    box.minY = minY;
    box.minZ = minZ;
    box.maxX = maxX;
    box.maxY = maxY;
    box.maxZ = maxZ;
    return true;
}

[[nodiscard]] ConnectionRule connectionRuleFromName(std::string name) {
    name = toLowerCopy(std::move(name));
    if (name == "same") {
        return ConnectionRule::Same;
    }
    if (name == "opaque") {
        return ConnectionRule::Opaque;
    }
    if (name == "glass") {
        return ConnectionRule::Glass;
    }
    if (name == "fence") {
        return ConnectionRule::Fence;
    }
    return ConnectionRule::Opaque;
}

bool parseBlockModel(lua_State* state, int tableIndex, LuaBlockModelSpec& model, std::string& error) {
    LuaApi& api = luaApi();
    const std::string type = toLowerCopy(luaStringField(state, tableIndex, "type", "full_cube"));
    model.opaque = luaBoolField(state, tableIndex, "opaque", model.opaque);
    model.fullCube = luaBoolField(state, tableIndex, "full_cube", model.fullCube);
    model.collisionHeight = luaFloatField(state, tableIndex, "collision_height", model.collisionHeight);
    model.stackOnSame = luaBoolField(state, tableIndex, "stack_on_same", model.stackOnSame);
    model.requiresSolidBelow = luaBoolField(state, tableIndex, "requires_solid_below", model.requiresSolidBelow);
    model.coordinateBounds = luaBoolField(state,
                                          tableIndex,
                                          "coordinate_bounds",
                                          luaBoolField(state, tableIndex, "varied_bounds", model.coordinateBounds));
    model.coordinateColor = luaBoolField(
        state, tableIndex, "coordinate_color", luaBoolField(state, tableIndex, "coord_color", model.coordinateColor));
    model.boundsPadding = luaFloatField(state, tableIndex, "bounds_padding", model.boundsPadding);
    model.boundsOffset = luaFloatField(state, tableIndex, "bounds_offset", model.boundsOffset);
    model.minScale = luaFloatField(state, tableIndex, "min_scale", model.minScale);
    model.maxScale = luaFloatField(state, tableIndex, "max_scale", model.maxScale);
    const std::string colorMode = toLowerCopy(luaStringField(state, tableIndex, "color", ""));
    if (colorMode == "coordinate" || colorMode == "coords" || colorMode == "position") {
        model.coordinateColor = true;
    }
    if (type == "full_cube" || type == "simple" || type.empty()) {
        model.type = LuaBlockModelSpec::Type::FullCube;
        return true;
    }
    if (type == "manual" || type == "custom" || type == "tessellated") {
        model.type = LuaBlockModelSpec::Type::Manual;
        model.opaque = luaBoolField(state, tableIndex, "opaque", false);
        model.fullCube = luaBoolField(state, tableIndex, "full_cube", false);
        return true;
    }
    if (type == "box_list" || type == "connected_bars") {
        model.type =
            type == "connected_bars" ? LuaBlockModelSpec::Type::ConnectedBars : LuaBlockModelSpec::Type::BoxList;
        model.opaque = luaBoolField(state, tableIndex, "opaque", type == "connected_bars" ? false : model.opaque);
        model.fullCube = luaBoolField(state, tableIndex, "full_cube", false);
        api.getfield(state, tableIndex, "connect");
        if (api.type(state, -1) == kLuaTTable) {
            const int connectTable = api.gettop(state);
            for (int i = 1; i <= 8; ++i) {
                api.rawgeti(state, connectTable, i);
                if (api.type(state, -1) == kLuaTString) {
                    model.connectRules.push_back(connectionRuleFromName(luaString(state, -1, "")));
                }
                api.settop(state, -2);
            }
        }
        api.settop(state, tableIndex);
        if (type == "connected_bars") {
            ModelBox core;
            core.alwaysDraw = true;
            api.getfield(state, tableIndex, "core");
            if (api.type(state, -1) != kLuaTTable || !readModelBox(state, api.gettop(state), core)) {
                api.settop(state, tableIndex);
                error = "connected_bars model requires core box with min/max";
                return false;
            }
            api.settop(state, tableIndex);
            model.boxes.push_back(core);

            const struct {
                const char* key;
                int north;
                int south;
                int east;
                int west;
            } arms[] = {
                {"north", 1, 0, 0, 0},
                {"south", 0, 1, 0, 0},
                {"east", 0, 0, 1, 0},
                {"west", 0, 0, 0, 1},
            };

            for (const auto& arm : arms) {
                api.getfield(state, tableIndex, arm.key);
                if (api.type(state, -1) == kLuaTTable) {
                    ModelBox box;
                    if (readModelBox(state, api.gettop(state), box)) {
                        box.alwaysDraw = false;
                        box.connectNorth = arm.north;
                        box.connectSouth = arm.south;
                        box.connectEast = arm.east;
                        box.connectWest = arm.west;
                        model.boxes.push_back(box);
                    }
                }
                api.settop(state, tableIndex);
            }
            if (model.connectRules.empty()) {
                model.connectRules = {
                    ConnectionRule::Same, ConnectionRule::Opaque, ConnectionRule::Glass, ConnectionRule::Fence};
            }
            return true;
        }
        api.getfield(state, tableIndex, "boxes");
        if (api.type(state, -1) != kLuaTTable) {
            api.settop(state, tableIndex);
            error = "box_list model requires boxes array";
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
                box.alwaysDraw = luaBoolField(state, api.gettop(state), "always", true);
                if (readModelBox(state, api.gettop(state), box)) {
                    model.boxes.push_back(box);
                }
            }
            api.settop(state, -2);
        }
        api.settop(state, tableIndex);
        if (model.boxes.empty()) {
            error = "box_list model requires at least one box";
            return false;
        }
        return true;
    }
    error = "unknown model type: " + type;
    return false;
}

int luaRegisterBlock(lua_State* state) {
    LuaApi& api = luaApi();
    ModHost::LoadedLuaMod* mod = currentLuaMod(state);
    if (mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
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
    api.getfield(state, tableIndex, "model");
    if (api.type(state, -1) == kLuaTTable) {
        const int modelIndex = api.gettop(state);
        std::string modelError;
        if (!parseBlockModel(state, modelIndex, spec.model, modelError)) {
            api.settop(state, tableIndex);
            api.pushboolean(state, 0);
            api.pushstring(state, modelError.c_str());
            return 2;
        }
        if (spec.model.type == LuaBlockModelSpec::Type::Manual) {
            api.getfield(state, modelIndex, "draw");
            if (api.type(state, -1) == kLuaTFunction) {
                spec.manualDrawRef = api.ref(state, kLuaRegistryIndex);
                if (spec.manualDrawRef != kLuaNoRef) {
                    mod->blockModelCallbackRefs.push_back(spec.manualDrawRef);
                }
            } else {
                api.settop(state, -2);
            }
            api.getfield(state, modelIndex, "inventory");
            if (api.type(state, -1) != kLuaTFunction) {
                api.settop(state, -2);
                api.getfield(state, modelIndex, "inventory_draw");
            }
            if (api.type(state, -1) == kLuaTFunction) {
                spec.manualInventoryRef = api.ref(state, kLuaRegistryIndex);
                if (spec.manualInventoryRef != kLuaNoRef) {
                    mod->blockModelCallbackRefs.push_back(spec.manualInventoryRef);
                }
            } else {
                api.settop(state, -2);
            }
        }
    }
    api.getfield(state, tableIndex, "item");
    if (api.type(state, -1) == kLuaTTable) {
        const int itemTable = api.gettop(state);
        spec.itemTexturePath = luaStringField(state, itemTable, "texture", "");
        spec.itemTextureId = luaIntField(state, itemTable, "texture_id", -1);
        api.settop(state, itemTable);
    }
    api.settop(state, tableIndex);
    std::string error;
    if (!registerBlockSpec(spec, error)) {
        api.pushboolean(state, 0);
        api.pushstring(state, error.c_str());
        return 2;
    }
    WorldRequiredMods::registerContentBlock(mod->modId, spec.blockId);
    api.pushboolean(state, 1);
    return 1;
}
}  // namespace

void installBlockApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
    bindModFunction(state, &mod, "_register_block", luaRegisterBlock);
}
}  // namespace net::minecraft::mod::runtime
