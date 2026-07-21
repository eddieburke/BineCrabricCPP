#include "net/minecraft/client/InteractionManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/BlockSounds.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
namespace net::minecraft::client {
InteractionManager::InteractionManager(Minecraft* minecraft) : minecraft(minecraft) {
}
void InteractionManager::setWorld(World* /*world*/) {
}
void InteractionManager::attackBlock(int x, int y, int z, int direction) {
 if(minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
  return;
 }
 minecraft->world->extinguishFire(minecraft->player, x, y, z, direction);
 breakBlock(x, y, z, direction);
}
bool InteractionManager::breakBlock(int x, int y, int z, int /*direction*/) {
 if(minecraft == nullptr || minecraft->world == nullptr) {
  return false;
 }
 World* world = minecraft->world;
 (void)minecraft->player;
 const int blockId = world->getBlockId(x, y, z);
 Block* block = blockId > 0 ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
 if(block != nullptr) {
  const int meta = world->getBlockMeta(x, y, z);
  block::sounds::playBreak(
      world, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5, block);
  world->spawnBlockBreakParticles(x, y, z, block->id, meta);
 }
 const int meta = world->getBlockMeta(x, y, z);
 const bool removed = world->setBlock(x, y, z, 0);
 if(block != nullptr && removed) {
  block->onMetadataChange(world, x, y, z, meta);
 }
 return removed;
}
void InteractionManager::processBlockBreakingAction(int x, int y, int z, int side) {
 (void)x;
 (void)y;
 (void)z;
 (void)side;
}
void InteractionManager::cancelBlockBreaking() {
}
void InteractionManager::update(float /*partialTick*/) {
}
float InteractionManager::getReachDistance() {
 return 5.0f;
}
bool InteractionManager::interactItem(PlayerEntity* player, World* world, ItemStack* item) {
 if(player == nullptr || world == nullptr || item == nullptr) {
  return false;
 }
 const std::size_t selectedSlot = static_cast<std::size_t>(player->inventory.selectedSlot);
 const int previousCount = item->count;
 ItemStack* itemStack = item->use(world, player);
 if(itemStack != item || (itemStack != nullptr && itemStack->count != previousCount)) {
  player->inventory.main[selectedSlot] = itemStack != nullptr ? *itemStack : ItemStack{};
  if(player->inventory.main[selectedSlot].empty()) {
   player->inventory.main[selectedSlot] = ItemStack{};
  }
  return true;
 }
 return false;
}
void InteractionManager::preparePlayer(PlayerEntity* /*player*/) {
}
void InteractionManager::tick() {
}
float InteractionManager::getBlockBreakingProgress(float /*partialTick*/) const {
 return 0.0f;
}
float InteractionManager::getLastBlockBreakingProgress() const {
 return 0.0f;
}
bool InteractionManager::canBeRendered() {
 return true;
}
void InteractionManager::preparePlayerRespawn(PlayerEntity* /*player*/) {
}
bool InteractionManager::interactBlock(
    PlayerEntity* player, World* world, ItemStack* item, int x, int y, int z, int side) {
 if(world == nullptr || player == nullptr) {
  return false;
 }
 mod::BlockInteractEvent event{player, world, item, x, y, z, side, true, false, false};
 event.client = minecraft;
net::minecraft::mod::lua::setModContext(world, true, player);
   net::minecraft::mod::runtime::luaHookBlockInteract(event);
   net::minecraft::mod::lua::clearModContext();
 if(event.canceled) {
  return event.handled;
 }
 item = event.stack;
 x = event.x;
 y = event.y;
 z = event.z;
 side = event.side;
 if(event.handled) {
  return true;
 }
 const int blockId = world->getBlockId(x, y, z);
 if(blockId > 0) {
  Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block != nullptr && block->onUse(world, x, y, z, player)) {
   return true;
  }
 }
 if(item == nullptr) {
  return false;
 }
 return item->useOnBlock(player, world, x, y, z, side);
}
PlayerEntity* InteractionManager::createPlayer(World* world) {
 const int dimensionId = (world != nullptr && world->dimension != nullptr) ? world->dimension->id : 0;
 return new entity::player::ClientPlayerEntity(
     minecraft, world, minecraft->session, minecraft->options, dimensionId);
}
void InteractionManager::interactEntity(PlayerEntity* player, Entity* entity) {
 if(player != nullptr && entity != nullptr) {
  mod::EntityInteractEvent event{player, entity, false, false};
  event.sneaking = player->isSneaking();
  event.stack = player->inventory.getSelectedItem();
   net::minecraft::mod::runtime::luaHookEntityInteract(event);
   if(event.canceled) {
    return;
   }
   player->interact(entity);
  }
 }
 void InteractionManager::attackEntity(PlayerEntity* player, Entity* target) {
 if(player != nullptr && target != nullptr) {
  mod::EntityInteractEvent event{player, target, true, false, false};
  event.stack = player->inventory.getSelectedItem();
   net::minecraft::mod::runtime::luaHookEntityInteract(event);
  if(event.canceled || event.handled) {
   return;
  }
  player->attack(target);
 }
 }
 ItemStack* InteractionManager::clickSlot(int /*syncId*/, int slotId, int button, bool shift, PlayerEntity* player) {
 if(player == nullptr || player->currentScreenHandler == nullptr) {
  return nullptr;
 }
 ItemStack result = player->currentScreenHandler->onSlotClick(slotId, button, shift, player);
 if(result.empty()) {
  return nullptr;
 }
 lastClickedStack_ = std::move(result);
 return &lastClickedStack_;
}
void InteractionManager::onScreenRemoved(int /*syncId*/, PlayerEntity* player) {
 if(player == nullptr || player->currentScreenHandler == nullptr) {
  return;
 }
 player->currentScreenHandler->onClosed(player);
 player->currentScreenHandler = &player->playerScreenHandler;
}
} // namespace net::minecraft::client
