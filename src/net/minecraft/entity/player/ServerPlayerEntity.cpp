#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/inventory/DoubleInventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/NetworkSyncedItem.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/EntityPackets.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/screen/CraftingScreenHandler.hpp"
#include "net/minecraft/screen/DispenserScreenHandler.hpp"
#include "net/minecraft/screen/FurnaceScreenHandler.hpp"
#include "net/minecraft/screen/GenericContainerScreenHandler.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::player {
ServerPlayerEntity::ServerPlayerEntity(server::MinecraftServer* serverIn,
                                       World* world,
                                       const std::string& nameIn,
                                       server::network::ServerPlayerInteractionManager* interactionManagerIn)
    : PlayerEntity(world), interactionManager(world != nullptr ? world : nullptr) {
 if(world != nullptr) {
  const Vec3i spawn = world->getSpawnPos();
  int spawnX = spawn.x;
  int spawnZ = spawn.z;
  int spawnY = spawn.y;
  if(world->dimension == nullptr || !world->dimension->hasCeiling) {
   spawnX += random.nextInt(20) - 10;
   spawnZ += random.nextInt(20) - 10;
   spawnY = world->getSpawnPositionValidityY(spawnX, spawnZ);
  }
  setPositionAndAnglesKeepPrevAngles(static_cast<double>(spawnX) + 0.5,
                                     static_cast<double>(spawnY),
                                     static_cast<double>(spawnZ) + 0.5,
                                     0.0f,
                                     0.0f);
 }
 server = serverIn;
 stepHeight = 0.0f;
 name = nameIn;
 standingEyeHeight = 0.0f;
 if(interactionManagerIn != nullptr) {
  interactionManager = *interactionManagerIn;
 }
 interactionManager.world = world;
 interactionManager.player = this;
 lastX = x;
 lastZ = z;
}
ItemStack ServerPlayerEntity::getEquipment(int slot) const {
 if(slot == 0) {
  const ItemStack* selected = inventory.getSelectedItem();
  return selected != nullptr ? *selected : ItemStack{};
 }
 if(slot >= 1 && slot <= static_cast<int>(inventory.armor.size())) {
  return inventory.armor[static_cast<std::size_t>(slot - 1)];
 }
 return {};
}
void ServerPlayerEntity::tick() {
 interactionManager.update();
 if(joinInvulnerabilityTicks > 0) {
  --joinInvulnerabilityTicks;
 }
 if(currentScreenHandler != nullptr) {
  currentScreenHandler->sendContentUpdates();
 }
 for(int slot = 0; slot < 5; ++slot) {
  const ItemStack itemStack = getEquipment(slot);
  if(itemStack == equipment_[static_cast<std::size_t>(slot)]) {
   continue;
  }
  if(server != nullptr) {
   EntityEquipmentUpdateS2CPacket packet;
   packet.id = id;
   packet.slot = slot;
   packet.itemRawId = itemStack.empty() ? -1 : itemStack.itemId;
   packet.itemDamage = itemStack.empty() ? 0 : itemStack.damage;
   server->getEntityTracker(dimensionId).sendToListeners(this, packet);
  }
  equipment_[static_cast<std::size_t>(slot)] = itemStack;
 }
}
void ServerPlayerEntity::playerTick(bool shouldSendChunkUpdates) {
 PlayerEntity::tick();
 for(std::size_t slot = 0; slot < inventory.size(); ++slot) {
  ItemStack itemStack = inventory.getStack(slot);
  if(itemStack.empty() || itemStack.getItem() == nullptr) {
   continue;
  }
  auto* networkSyncedItem = dynamic_cast<item::NetworkSyncedItem*>(itemStack.getItem());
  if(networkSyncedItem == nullptr || networkHandler == nullptr ||
     networkHandler->getBlockDataSendQueueSize() > 2) {
   continue;
  }
  std::unique_ptr<Packet> updatePacket(networkSyncedItem->getUpdatePacket(&itemStack, world, this));
  if(updatePacket == nullptr) {
   continue;
  }
  networkHandler->sendPacket(std::move(updatePacket));
 }
 (void)shouldSendChunkUpdates;
 if(inTeleportationState) {
  if(server != nullptr && server->allowNether) {
   if(currentScreenHandler != &playerScreenHandler) {
    closeHandledScreen();
   }
   if(vehicle != nullptr) {
    setVehicle(vehicle);
   } else {
    changeDimensionCooldown += 0.0125f;
    if(changeDimensionCooldown >= 1.0f) {
     changeDimensionCooldown = 1.0f;
     portalCooldown = 10;
     server->playerManager.changePlayerDimension(this);
    }
   }
   inTeleportationState = false;
  }
 } else {
  if(changeDimensionCooldown > 0.0f) {
   changeDimensionCooldown -= 0.05f;
  }
  if(changeDimensionCooldown < 0.0f) {
   changeDimensionCooldown = 0.0f;
  }
 }
 if(portalCooldown > 0) {
  --portalCooldown;
 }
 if(health != lastHealthScore) {
  if(networkHandler != nullptr) {
   HealthUpdateS2CPacket packet;
   packet.health = health;
   networkHandler->sendPacket(packet);
  }
  lastHealthScore = health;
 }
}
void ServerPlayerEntity::setWorld(World* worldIn) {
 Entity::setWorld(worldIn);
 if(auto* serverWorld = dynamic_cast<ServerWorld*>(worldIn); serverWorld != nullptr) {
  interactionManager = server::network::ServerPlayerInteractionManager(serverWorld);
  interactionManager.player = this;
 }
}
void ServerPlayerEntity::initScreenHandler() {
 if(currentScreenHandler != nullptr) {
  currentScreenHandler->addListener(this);
 }
}
void ServerPlayerEntity::incrementScreenHandlerSyncId() {
 screenHandlerSyncId_ = screenHandlerSyncId_ % 100 + 1;
}
void ServerPlayerEntity::openChestScreen(Inventory* inventoryIn) {
 if(inventoryIn == nullptr || networkHandler == nullptr) {
  return;
 }
 ownedScreenInventory_.reset();
 incrementScreenHandlerSyncId();
 OpenScreenS2CPacket packet;
 packet.syncId = screenHandlerSyncId_;
 packet.screenHandlerId = 0;
 packet.name = inventoryIn->getName();
 packet.inventorySize = static_cast<int>(inventoryIn->size());
 networkHandler->sendPacket(packet);
 ownedScreenHandler_ = std::make_unique<screen::GenericContainerScreenHandler>(&inventory, inventoryIn);
 currentScreenHandler = ownedScreenHandler_.get();
 currentScreenHandler->syncId = screenHandlerSyncId_;
 initScreenHandler();
}
void ServerPlayerEntity::openChestScreen(int xIn, int yIn, int zIn) {
 if(world == nullptr || networkHandler == nullptr) {
  return;
 }
 auto* chest = dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn, yIn, zIn));
 if(chest == nullptr) {
  return;
 }
 Inventory* inventoryIn = chest;
 ownedScreenInventory_.reset();
 const int chestId = Block::CHEST != nullptr ? Block::CHEST->id : 54;
 if(world->getBlockId(xIn - 1, yIn, zIn) == chestId) {
  ownedScreenInventory_ = std::make_unique<DoubleInventory>(
      "Large chest", world->getBlockEntity(xIn - 1, yIn, zIn) != nullptr ? dynamic_cast<Inventory*>(world->getBlockEntity(xIn - 1, yIn, zIn)) : nullptr,
      chest);
 } else if(world->getBlockId(xIn + 1, yIn, zIn) == chestId) {
  ownedScreenInventory_ = std::make_unique<DoubleInventory>(
      "Large chest", chest, dynamic_cast<Inventory*>(world->getBlockEntity(xIn + 1, yIn, zIn)));
 } else if(world->getBlockId(xIn, yIn, zIn - 1) == chestId) {
  ownedScreenInventory_ = std::make_unique<DoubleInventory>(
      "Large chest", dynamic_cast<Inventory*>(world->getBlockEntity(xIn, yIn, zIn - 1)), chest);
 } else if(world->getBlockId(xIn, yIn, zIn + 1) == chestId) {
  ownedScreenInventory_ = std::make_unique<DoubleInventory>(
      "Large chest", chest, dynamic_cast<Inventory*>(world->getBlockEntity(xIn, yIn, zIn + 1)));
 }
 if(ownedScreenInventory_ != nullptr) {
  inventoryIn = ownedScreenInventory_.get();
 }
 incrementScreenHandlerSyncId();
 OpenScreenS2CPacket packet;
 packet.syncId = screenHandlerSyncId_;
 packet.screenHandlerId = 0;
 packet.name = inventoryIn->getName();
 packet.inventorySize = static_cast<int>(inventoryIn->size());
 networkHandler->sendPacket(packet);
 ownedScreenHandler_ = std::make_unique<screen::GenericContainerScreenHandler>(&inventory, inventoryIn);
 currentScreenHandler = ownedScreenHandler_.get();
 currentScreenHandler->syncId = screenHandlerSyncId_;
 initScreenHandler();
}
void ServerPlayerEntity::openFurnaceScreen(block::entity::FurnaceBlockEntity* furnaceIn) {
 if(furnaceIn == nullptr || networkHandler == nullptr) {
  return;
 }
 ownedScreenInventory_.reset();
 incrementScreenHandlerSyncId();
 OpenScreenS2CPacket packet;
 packet.syncId = screenHandlerSyncId_;
 packet.screenHandlerId = 2;
 packet.name = furnaceIn->getName();
 packet.inventorySize = static_cast<int>(furnaceIn->size());
 networkHandler->sendPacket(packet);
 ownedScreenHandler_ = std::make_unique<screen::FurnaceScreenHandler>(&inventory, furnaceIn);
 currentScreenHandler = ownedScreenHandler_.get();
 currentScreenHandler->syncId = screenHandlerSyncId_;
 initScreenHandler();
}
void ServerPlayerEntity::openDispenserScreen(block::entity::DispenserBlockEntity* dispenserIn) {
 if(dispenserIn == nullptr || networkHandler == nullptr) {
  return;
 }
 ownedScreenInventory_.reset();
 incrementScreenHandlerSyncId();
 OpenScreenS2CPacket packet;
 packet.syncId = screenHandlerSyncId_;
 packet.screenHandlerId = 3;
 packet.name = dispenserIn->getName();
 packet.inventorySize = static_cast<int>(dispenserIn->size());
 networkHandler->sendPacket(packet);
 ownedScreenHandler_ = std::make_unique<screen::DispenserScreenHandler>(&inventory, dispenserIn);
 currentScreenHandler = ownedScreenHandler_.get();
 currentScreenHandler->syncId = screenHandlerSyncId_;
 initScreenHandler();
}
void ServerPlayerEntity::openCraftingScreen(int xIn, int yIn, int zIn) {
 if(networkHandler == nullptr) {
  return;
 }
 ownedScreenInventory_.reset();
 incrementScreenHandlerSyncId();
 OpenScreenS2CPacket packet;
 packet.syncId = screenHandlerSyncId_;
 packet.screenHandlerId = 1;
 packet.name = "Crafting";
 packet.inventorySize = 9;
 networkHandler->sendPacket(packet);
 ownedScreenHandler_ = std::make_unique<screen::CraftingScreenHandler>(&inventory, xIn, yIn, zIn);
 currentScreenHandler = ownedScreenHandler_.get();
 currentScreenHandler->syncId = screenHandlerSyncId_;
 initScreenHandler();
}
void ServerPlayerEntity::clearWakePosition() {
}
void ServerPlayerEntity::markHealthDirty() {
 lastHealthScore = -99999999;
}
void ServerPlayerEntity::onContentsUpdate(screen::ScreenHandler& screenHandler) {
 onContentsUpdate(screenHandler, screenHandler.getStacks());
}
void ServerPlayerEntity::onContentsUpdate(screen::ScreenHandler& screenHandler, const std::vector<ItemStack>& stacks) {
 if(networkHandler == nullptr) {
  return;
 }
 InventoryS2CPacket inventoryPacket;
 inventoryPacket.syncId = screenHandler.syncId;
 inventoryPacket.contents = stacks;
 networkHandler->sendPacket(inventoryPacket);
 ScreenHandlerSlotUpdateS2CPacket cursorPacket;
 cursorPacket.syncId = -1;
 cursorPacket.slot = -1;
 cursorPacket.stack = inventory.getCursorStack();
 networkHandler->sendPacket(cursorPacket);
}
void ServerPlayerEntity::onSlotUpdate(screen::ScreenHandler& handler, int slot, const ItemStack& stack) {
 if(skipPacketSlotUpdates || networkHandler == nullptr) {
  return;
 }
 ScreenHandlerSlotUpdateS2CPacket packet;
 packet.syncId = handler.syncId;
 packet.slot = slot;
 packet.stack = stack;
 networkHandler->sendPacket(packet);
}
void ServerPlayerEntity::onPropertyUpdate(screen::ScreenHandler& handler, int property, int value) {
 if(networkHandler == nullptr) {
  return;
 }
 ScreenHandlerPropertyUpdateS2CPacket packet;
 packet.syncId = handler.syncId;
 packet.propertyId = property;
 packet.value = value;
 networkHandler->sendPacket(packet);
}
void ServerPlayerEntity::onDisconnect() {
 if(vehicle != nullptr) {
  setVehicle(vehicle);
 }
 if(passenger != nullptr) {
  passenger->setVehicle(this);
 }
 if(isSleeping()) {
  wakeUp(true, false, false);
 }
}
void ServerPlayerEntity::setVehicle(Entity* entity) {
 // Java ServerPlayerEntity#setVehicle: after the base attach/detach, tell this
 // player's own client about the new (dis)mount and re-sync their position.
 // Without this the mount only exists server-side and right-click never seats
 // the local player. setVehicle is virtual so PlayerEntity* call sites (e.g.
 // MinecartEntity::interact) dispatch here for real server players.
 Entity::setVehicle(entity);
 if(networkHandler != nullptr) {
  EntityVehicleSetS2CPacket packet;
  packet.id = id;
  packet.vehicleId = vehicle != nullptr ? vehicle->id : -1;
  networkHandler->sendPacket(packet);
  networkHandler->teleport(x, y, z, yaw, pitch);
 }
}
void ServerPlayerEntity::onHandledScreenClosed() {
 if(currentScreenHandler != nullptr) {
  currentScreenHandler->onClosed(this);
 }
 currentScreenHandler = &playerScreenHandler;
 ownedScreenHandler_.reset();
 ownedScreenInventory_.reset();
}
void ServerPlayerEntity::updateCursorStack() {
 if(skipPacketSlotUpdates || networkHandler == nullptr) {
  return;
 }
 ScreenHandlerSlotUpdateS2CPacket packet;
 packet.syncId = -1;
 packet.slot = -1;
 packet.stack = inventory.getCursorStack();
 networkHandler->sendPacket(packet);
}
void ServerPlayerEntity::closeHandledScreen() {
 if(networkHandler != nullptr && currentScreenHandler != nullptr) {
  CloseScreenS2CPacket packet;
  packet.syncId = currentScreenHandler->syncId;
  networkHandler->sendPacket(packet);
 }
 onHandledScreenClosed();
}
void ServerPlayerEntity::onKilledBy(Entity* adversary) {
 (void)adversary;
 inventory.dropInventory();
}
bool ServerPlayerEntity::damage(Entity* damageSource, int amount) {
 if(joinInvulnerabilityTicks > 0) {
  return false;
 }
 if(server != nullptr && !server->pvpEnabled) {
  if(dynamic_cast<PlayerEntity*>(damageSource) != nullptr) {
   return false;
  }
  if(auto* arrow = dynamic_cast<entity::projectile::ArrowEntity*>(damageSource)) {
   if(arrow->owner != nullptr && dynamic_cast<PlayerEntity*>(arrow->owner) != nullptr) {
    return false;
   }
  }
 }
 return PlayerEntity::damage(damageSource, amount);
}
bool ServerPlayerEntity::isPvpEnabled() const {
 return server != nullptr && server->pvpEnabled;
}
void ServerPlayerEntity::respawn() {
 if(world == nullptr) {
  return;
 }
 markDead();
 world->remove(this);
 dead = false;
 health = maxHealth;
 deathTime = 0;
 fireTicks = 0;
 if(spawnPos.has_value()) {
  setPositionAndAnglesKeepPrevAngles(static_cast<double>(spawnPos->x) + 0.5,
                                     static_cast<double>(spawnPos->y) + 1.0,
                                     static_cast<double>(spawnPos->z) + 0.5,
                                     yaw,
                                     pitch);
 } else {
  const Vec3i spawn = world->getSpawnPos();
  setPositionAndAnglesKeepPrevAngles(static_cast<double>(spawn.x) + 0.5,
                                     static_cast<double>(spawn.y) + 1.0,
                                     static_cast<double>(spawn.z) + 0.5,
                                     yaw,
                                     pitch);
 }
 velocityX = 0.0;
 velocityY = 0.0;
 velocityZ = 0.0;
 world->spawnEntity(this);
}
SleepAttemptResult ServerPlayerEntity::trySleep(int xIn, int yIn, int zIn) {
 const SleepAttemptResult result = PlayerEntity::trySleep(xIn, yIn, zIn);
 if(result == SleepAttemptResult::Ok && server != nullptr) {
  PlayerSleepUpdateS2CPacket packet;
  packet.id = id;
  packet.status = 0;
  packet.x = xIn;
  packet.y = yIn;
  packet.z = zIn;
  server->getEntityTracker(dimensionId).sendToListeners(this, packet);
  if(networkHandler != nullptr) {
   networkHandler->teleport(x, y, z, yaw, pitch);
   networkHandler->sendPacket(packet);
  }
 }
 return result;
}
void ServerPlayerEntity::wakeUp(bool resetSleepTimer, bool updateSleepingPlayers, bool setSpawnPosFlag) {
 if(isSleeping() && server != nullptr) {
  EntityAnimationPacket packet;
  packet.id = id;
  packet.animationId = 3;
  server->getEntityTracker(dimensionId).sendToAround(this, packet);
 }
 PlayerEntity::wakeUp(resetSleepTimer, updateSleepingPlayers, setSpawnPosFlag);
 if(networkHandler != nullptr) {
  networkHandler->teleport(x, y, z, yaw, pitch);
 }
}
void ServerPlayerEntity::swingHand() {
 if(handSwinging) {
  return;
 }
 handSwingTicks = -1;
 handSwinging = true;
 if(server != nullptr) {
  EntityAnimationPacket packet;
  packet.id = id;
  packet.animationId = 1;
  server->getEntityTracker(dimensionId).sendToListeners(this, packet);
 }
}
void ServerPlayerEntity::resetEyeHeight() {
 standingEyeHeight = 0.0f;
}
void ServerPlayerEntity::increaseStat(int stat, int amount) {
 if(amount <= 0 || statPacketSender == nullptr || stat::Stats::isLocalOnly(stat)) {
  return;
 }
 int remaining = amount;
 while(remaining > 100) {
  statPacketSender(stat, 100);
  remaining -= 100;
 }
 statPacketSender(stat, remaining);
}
void ServerPlayerEntity::sendMessage(const std::string& message) {
 if(networkHandler != nullptr) {
  ChatMessagePacket packet;
  packet.chatMessage = message;
  networkHandler->sendPacket(packet);
 }
}
void ServerPlayerEntity::updateInput(
    float sidewaysSpeedIn, float forwardSpeedIn, bool jumpingIn, bool sneakingIn, float pitchIn, float yawIn) {
 sidewaysSpeed = sidewaysSpeedIn;
 forwardSpeed = forwardSpeedIn;
 jumping = jumpingIn;
 setSneaking(sneakingIn);
 pitch = pitchIn;
 yaw = yawIn;
}
void ServerPlayerEntity::handleFall(double heightDifference, bool onGroundIn) {
 fall(heightDifference, onGroundIn);
}
float ServerPlayerEntity::getEyeHeight() const {
 return 1.62f;
}
} // namespace net::minecraft::entity::player
