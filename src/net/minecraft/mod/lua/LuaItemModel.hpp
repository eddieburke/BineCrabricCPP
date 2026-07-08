#pragma once
#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include <string>
struct lua_State;
namespace net::minecraft::client::render {
class Tessellator;
}
namespace net::minecraft::mod::lua {
struct LuaItemModelSpec {
  enum class Type {
    Flat = 0,
    BoxList,
    Manual,
  };
  Type type = Type::Flat;
  std::vector<ModelBox> boxes;
};
// Registration input lives in LuaItemRegistry; the model layer only needs to
// name it for the manual-draw and instantiation entry points below.
struct ItemRegistrationSpec;
// Builds the concrete Item from a validated spec. Called by the item registry
// during the batched ItemRegistration lifecycle phase.
void instantiateLuaModItem(const ItemRegistrationSpec& spec);
bool drawLuaItemModel(client::render::Tessellator& tessellator, const ItemStack& stack, float brightness);
// When enabled, every non-block item renders as an extruded sprite slab via
// drawLuaItemModel instead of the flat GUI sprite. This is the easy hand/UI
// model override; the thrown/ground physics is handled separately by Lua.
[[nodiscard]] bool itemModelHandOverrideActive();
void setItemModelHandOverride(bool enabled);
bool invokeManualItemModelDraw(const ItemRegistrationSpec& spec, float brightness);
bool emitManualItemModelQuad(const ManualBlockVertex* vertices, int textureId, float red, float green, float blue,
                             float alpha);
} // namespace net::minecraft::mod::lua
