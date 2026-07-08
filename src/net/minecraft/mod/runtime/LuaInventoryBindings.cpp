#include "net/minecraft/mod/runtime/LuaInventoryBindings.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/inventory/InventoryApi.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#endif
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
#ifdef MINECRAFT_NATIVE_EXPORTS
namespace {
using PlayerInventory = entity::player::PlayerInventory;
[[nodiscard]] entity::player::PlayerEntity* luaPlayer() {
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr || client->player == nullptr) {
    return nullptr;
  }
  return static_cast<entity::player::PlayerEntity*>(client->player);
}
[[nodiscard]] ItemStack readStack(lua_State* state, int index) {
  LuaApi& api = luaApi();
  if(api.type(state, index) != kLuaTTable) {
    return {};
  }
  const int itemId = luaIntField(state, index, "id", luaIntField(state, index, "item_id", 0));
  const int count = luaIntField(state, index, "count", 1);
  const int damage = luaIntField(state, index, "damage", 0);
  if(itemId <= 0 || count <= 0) {
    return {};
  }
  return ItemStack(itemId, count, damage);
}
void pushStack(lua_State* state, const ItemStack& stack) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 6);
  setField(state, "id", stack.itemId);
  setField(state, "item_id", stack.itemId);
  setField(state, "count", stack.count);
  setField(state, "damage", stack.damage);
  if(!stack.empty()) {
    setField(state, "max_damage", stack.getMaxDamage());
    setField(state, "damageable", stack.isDamageable());
    setField(state, "stackable", stack.isStackable());
    setField(state, "has_subtypes", stack.hasSubtypes());
    setField(state, "max_count", stack.getMaxCount());
  }
}
[[nodiscard]] ItemStack* slotStack(PlayerInventory& inventory, int slot) {
  if(slot < 0) {
    return nullptr;
  }
  if(slot < static_cast<int>(PlayerInventory::mainSize)) {
    return &inventory.main[static_cast<std::size_t>(slot)];
  }
  const int armorSlot = slot - static_cast<int>(PlayerInventory::mainSize);
  if(armorSlot >= 0 && armorSlot < static_cast<int>(PlayerInventory::armorSize)) {
    return &inventory.armor[static_cast<std::size_t>(armorSlot)];
  }
  return nullptr;
}
int luaInventorySlotCount(lua_State* state) {
  luaApi().pushinteger(state, static_cast<long long>(PlayerInventory::mainSize + PlayerInventory::armorSize));
  return 1;
}
int luaInventoryMainSize(lua_State* state) {
  luaApi().pushinteger(state, static_cast<long long>(PlayerInventory::mainSize));
  return 1;
}
int luaInventoryGet(lua_State* state) {
  LuaApi& api = luaApi();
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    api.pushnil(state);
    return 1;
  }
  int isNumber = 0;
  const int slot = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0) {
    api.pushnil(state);
    return 1;
  }
  ItemStack* stack = slotStack(player->inventory, slot);
  if(stack == nullptr) {
    api.pushnil(state);
    return 1;
  }
  pushStack(state, *stack);
  return 1;
}
int luaInventorySet(lua_State* state) {
  LuaApi& api = luaApi();
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int slot = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
  ItemStack* stack = slotStack(player->inventory, slot);
  if(stack == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  *stack = readStack(state, 2);
  player->inventory.markDirty();
  api.pushboolean(state, 1);
  return 1;
}
int luaInventoryCursorGet(lua_State* state) {
  LuaApi& api = luaApi();
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    api.pushnil(state);
    return 1;
  }
  pushStack(state, player->inventory.getCursorStack());
  return 1;
}
int luaInventoryCursorSet(lua_State* state) {
  LuaApi& api = luaApi();
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  player->inventory.setCursorStack(readStack(state, 1));
  api.pushboolean(state, 1);
  return 1;
}
int luaInventoryGive(lua_State* state) {
  LuaApi& api = luaApi();
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    api.pushboolean(state, 0);
    return 1;
  }
  inventory::giveToPlayer(*player, readStack(state, 1));
  api.pushboolean(state, 1);
  return 1;
}
int luaInventoryOffer(lua_State* state) {
  entity::player::PlayerEntity* player = luaPlayer();
  if(player == nullptr) {
    pushStack(state, readStack(state, 1));
    return 1;
  }
  const ItemStack remainder = inventory::offerToPlayer(*player, readStack(state, 1));
  pushStack(state, remainder);
  return 1;
}
int luaItemsDescribe(lua_State* state) {
  LuaApi& api = luaApi();
  int isNumber = 0;
  const int itemId = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0 || itemId <= 0) {
    api.pushnil(state);
    return 1;
  }
  const ItemStack probe(itemId, 1);
  if(probe.empty()) {
    api.pushnil(state);
    return 1;
  }
  api.createtable(state, 0, 6);
  setField(state, "id", itemId);
  setField(state, "max_damage", probe.getMaxDamage());
  setField(state, "damageable", probe.isDamageable());
  setField(state, "stackable", probe.isStackable());
  setField(state, "has_subtypes", probe.hasSubtypes());
  setField(state, "max_count", probe.getMaxCount());
  return 1;
}
} // namespace
#endif
void installInventoryApi(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  const int root = api.gettop(state);
  pushFunctionTable(state, {
                               {"slot_count", luaInventorySlotCount},
                               {"main_size", luaInventoryMainSize},
                               {"get", luaInventoryGet},
                               {"set", luaInventorySet},
                               {"cursor_get", luaInventoryCursorGet},
                               {"cursor_set", luaInventoryCursorSet},
                               {"give", luaInventoryGive},
                               {"offer", luaInventoryOffer},
                           });
  api.setfield(state, root, "inventory");
  api.getfield(state, root, "items");
  if(api.type(state, -1) != kLuaTTable) {
    api.settop(state, -2);
    pushFunctionTable(state, {{"describe", luaItemsDescribe}});
    api.setfield(state, root, "items");
  } else {
    api.pushcclosure(state, luaItemsDescribe, 0);
    api.setfield(state, -2, "describe");
    api.settop(state, root);
  }
#else
  (void)state;
#endif
}
} // namespace net::minecraft::mod::runtime
