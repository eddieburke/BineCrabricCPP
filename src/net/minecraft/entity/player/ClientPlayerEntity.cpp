#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/PlayerTextures.hpp"
#include "net/minecraft/client/gui/screen/ingame/CraftingScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/DispenserScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/DoubleChestScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/FurnaceScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/inventory/DoubleInventory.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/CraftingScreenHandler.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::entity::player {
ClientPlayerEntity::ClientPlayerEntity(client::Minecraft* minecraft,
                                       World* world,
                                       const client::util::Session& session,
                                       client::option::GameOptions& /*options*/,
                                       int dimensionIdIn)
    : PlayerEntity(world), minecraft_(minecraft) {
 dimensionId = dimensionIdIn;
 msauth::applySessionTextures(*this, session);
 name = session.username.empty() ? "Player" : session.username;
}
void ClientPlayerEntity::move(double dx, double dy, double dz) {
 PlayerEntity::move(dx, dy, dz);
}
void ClientPlayerEntity::tickLiving() {
 PlayerEntity::tickLiving();
 const client::input::MovementState& movement = client::input::InputSystem::instance().movement();
 sidewaysSpeed = movement.sideways;
 forwardSpeed = movement.forward;
 jumping = movement.jumping;
}
void ClientPlayerEntity::tickMovement() {
 if(minecraft_ != nullptr && minecraft_->stats != nullptr &&
    !minecraft_->stats->hasStat(achievement::Achievements::OPEN_INVENTORY.statId())) {
  minecraft_->toast.setTutorial(achievement::Achievements::OPEN_INVENTORY.statId());
 }
 touchingPortal = false;
 const client::input::MovementState& movement = client::input::InputSystem::instance().movement();
 setSneaking(movement.sneaking && !sleeping);
 if(movement.sneaking && cameraOffset < 0.2f) {
  cameraOffset = 0.2f;
 }
 pushOutOfBlock(
     x - static_cast<double>(width) * 0.35, boundingBox.minY + 0.5, z + static_cast<double>(width) * 0.35);
 pushOutOfBlock(
     x - static_cast<double>(width) * 0.35, boundingBox.minY + 0.5, z - static_cast<double>(width) * 0.35);
 pushOutOfBlock(
     x + static_cast<double>(width) * 0.35, boundingBox.minY + 0.5, z - static_cast<double>(width) * 0.35);
 pushOutOfBlock(
     x + static_cast<double>(width) * 0.35, boundingBox.minY + 0.5, z + static_cast<double>(width) * 0.35);
 PlayerEntity::tickMovement();
 lastScreenDistortion = screenDistortion;
 if(inTeleportationState) {
  if(world != nullptr && !world->isRemote() && vehicle != nullptr) {
   setVehicle(nullptr);
  }
  if(minecraft_ != nullptr) {
   minecraft_->setScreen(nullptr);
  }
  if(screenDistortion == 0.0f) {
   if(minecraft_ != nullptr) {
    minecraft_->audio.play("portal.trigger", 1.0f, random.nextFloat() * 0.4f + 0.8f);
   }
  }
  screenDistortion += 0.0125f;
  if(screenDistortion >= 1.0f) {
   screenDistortion = 1.0f;
   if(world != nullptr && !world->isRemote() && minecraft_ != nullptr) {
    portalCooldown = 10;
    minecraft_->audio.play("portal.travel", 1.0f, random.nextFloat() * 0.4f + 0.8f);
    minecraft_->changeDimension();
   }
   inTeleportationState = false;
  }
 } else if(screenDistortion > 0.0f) {
  screenDistortion -= 0.05f;
  if(screenDistortion < 0.0f) {
   screenDistortion = 0.0f;
  }
 }
 if(!touchingPortal) {
  inTeleportationState = false;
 }
 if(portalCooldown > 0) {
  --portalCooldown;
 }
}
void ClientPlayerEntity::writeNbt(NbtCompound& nbt) const {
 PlayerEntity::writeNbt(nbt);
 nbt.putInt("Score", score);
}
void ClientPlayerEntity::readNbt(const NbtCompound& nbt) {
 PlayerEntity::readNbt(nbt);
 score = nbt.getInt("Score");
}
void ClientPlayerEntity::closeHandledScreen() {
 PlayerEntity::closeHandledScreen();
 openChestDoubleInventory_.reset();
 if(minecraft_ != nullptr) {
  minecraft_->setScreen(nullptr);
 }
}
void ClientPlayerEntity::openChestScreen(Inventory* inventoryIn) {
 if(minecraft_ == nullptr || inventoryIn == nullptr) {
  return;
 }
 minecraft_->setScreen(std::make_unique<client::gui::screen::ingame::DoubleChestScreen>(&inventory, inventoryIn));
}
void ClientPlayerEntity::openChestScreen(int xIn, int yIn, int zIn) {
 if(minecraft_ == nullptr || world == nullptr) {
  return;
 }
 auto* chest = dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn, yIn, zIn));
 if(chest == nullptr) {
  return;
 }
 Inventory* chestInventory = chest;
 openChestDoubleInventory_.reset();
 const int chestId = Block::CHEST != nullptr ? Block::CHEST->id : 54;
 if(world->getBlockId(xIn - 1, yIn, zIn) == chestId) {
  openChestDoubleInventory_ = std::make_unique<DoubleInventory>(
      "Large chest",
      dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn - 1, yIn, zIn)),
      chest);
  chestInventory = openChestDoubleInventory_.get();
 } else if(world->getBlockId(xIn + 1, yIn, zIn) == chestId) {
  openChestDoubleInventory_ = std::make_unique<DoubleInventory>(
      "Large chest",
      chest,
      dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn + 1, yIn, zIn)));
  chestInventory = openChestDoubleInventory_.get();
 } else if(world->getBlockId(xIn, yIn, zIn - 1) == chestId) {
  openChestDoubleInventory_ = std::make_unique<DoubleInventory>(
      "Large chest",
      dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn, yIn, zIn - 1)),
      chest);
  chestInventory = openChestDoubleInventory_.get();
 } else if(world->getBlockId(xIn, yIn, zIn + 1) == chestId) {
  openChestDoubleInventory_ = std::make_unique<DoubleInventory>(
      "Large chest",
      chest,
      dynamic_cast<block::entity::ChestBlockEntity*>(world->getBlockEntity(xIn, yIn, zIn + 1)));
  chestInventory = openChestDoubleInventory_.get();
 }
 minecraft_->setScreen(std::make_unique<client::gui::screen::ingame::DoubleChestScreen>(&inventory, chestInventory));
}
void ClientPlayerEntity::openFurnaceScreen(block::entity::FurnaceBlockEntity* furnaceIn) {
 if(minecraft_ == nullptr || furnaceIn == nullptr) {
  return;
 }
 minecraft_->setScreen(std::make_unique<client::gui::screen::ingame::FurnaceScreen>(&inventory, furnaceIn));
}
void ClientPlayerEntity::openDispenserScreen(block::entity::DispenserBlockEntity* dispenserIn) {
 if(minecraft_ == nullptr || dispenserIn == nullptr) {
  return;
 }
 minecraft_->setScreen(std::make_unique<client::gui::screen::ingame::DispenserScreen>(&inventory, dispenserIn));
}
void ClientPlayerEntity::openCraftingScreen(int xIn, int yIn, int zIn) {
 if(minecraft_ == nullptr) {
  return;
 }
 minecraft_->setScreen(std::make_unique<client::gui::screen::ingame::CraftingScreen>(&inventory, xIn, yIn, zIn));
}
void ClientPlayerEntity::sendChatMessage(const std::string& message) {
 (void)message;
}
bool ClientPlayerEntity::isSneaking() const {
 return client::input::InputSystem::instance().movement().sneaking && !sleeping;
}
bool ClientPlayerEntity::shouldSuffocate(int blockX, int blockY, int blockZ) const {
 return world != nullptr && world->shouldSuffocate(blockX, blockY, blockZ);
}
bool ClientPlayerEntity::pushOutOfBlock(double px, double py, double pz) {
 if(world == nullptr) {
  return false;
 }
 const int blockX = MathHelper::floor(px);
 const int blockY = MathHelper::floor(py);
 const int blockZ = MathHelper::floor(pz);
 const double localX = px - static_cast<double>(blockX);
 const double localZ = pz - static_cast<double>(blockZ);
 if(shouldSuffocate(blockX, blockY, blockZ) || shouldSuffocate(blockX, blockY + 1, blockZ)) {
  const bool westFree =
      !shouldSuffocate(blockX - 1, blockY, blockZ) && !shouldSuffocate(blockX - 1, blockY + 1, blockZ);
  const bool eastFree =
      !shouldSuffocate(blockX + 1, blockY, blockZ) && !shouldSuffocate(blockX + 1, blockY + 1, blockZ);
  const bool northFree =
      !shouldSuffocate(blockX, blockY, blockZ - 1) && !shouldSuffocate(blockX, blockY + 1, blockZ - 1);
  const bool southFree =
      !shouldSuffocate(blockX, blockY, blockZ + 1) && !shouldSuffocate(blockX, blockY + 1, blockZ + 1);
  int direction = -1;
  double bestDistance = 9999.0;
  if(westFree && localX < bestDistance) {
   bestDistance = localX;
   direction = 0;
  }
  if(eastFree && 1.0 - localX < bestDistance) {
   bestDistance = 1.0 - localX;
   direction = 1;
  }
  if(northFree && localZ < bestDistance) {
   bestDistance = localZ;
   direction = 4;
  }
  if(southFree && 1.0 - localZ < bestDistance) {
   bestDistance = 1.0 - localZ;
   direction = 5;
  }
  constexpr float push = 0.1f;
  if(direction == 0) {
   velocityX = -push;
  }
  if(direction == 1) {
   velocityX = push;
  }
  if(direction == 4) {
   velocityZ = -push;
  }
  if(direction == 5) {
   velocityZ = push;
  }
 }
 return false;
}
void ClientPlayerEntity::respawn() {
 if(minecraft_ != nullptr) {
  minecraft_->respawnPlayer(false, 0);
 }
}
void ClientPlayerEntity::increaseStat(int stat, int amount) {
 if(minecraft_ == nullptr || minecraft_->stats == nullptr) {
  return;
 }
 if(achievement::Achievements::isAchievementStatId(stat)) {
  if(!minecraft_->stats->hasParentAchievement(stat)) {
   return;
  }
  if(!minecraft_->stats->hasStat(stat)) {
   minecraft_->toast.set(stat);
  }
  minecraft_->stats->incrementById(stat, amount);
  return;
 }
 if(stat::StatId* statId = stat::Stats::getStatById(stat)) {
  minecraft_->stats->increment(*statId, amount);
  return;
 }
 minecraft_->stats->incrementById(stat, amount);
}
} // namespace net::minecraft::entity::player
