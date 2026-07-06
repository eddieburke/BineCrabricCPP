#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include <memory>
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/item/FoodItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
namespace net::minecraft::entity::passive {
WolfEntity::WolfEntity(World* world) : AnimalEntity(world) {
  initDataTracker();
  texture = "/mob/wolf.png";
  setBoundingBoxSpacing(0.8f, 0.8f);
  movementSpeed = 1.1f;
  health = 8;
}
std::string WolfEntity::getRandomSound() {
  if(isAngry()) {
    return "mob.wolf.growl";
  }
  if(random.nextInt(3) == 0) {
    if(isTamed() && dataTracker.getInt(18) < 10) {
      return "mob.wolf.whine";
    }
    return "mob.wolf.panting";
  }
  return "mob.wolf.bark";
}
void WolfEntity::tick() {
  AnimalEntity::tick();
  lastBegAnimationProcess = begAnimationProgress;
  begAnimationProgress = begging ? begAnimationProgress + (1.0f - begAnimationProgress) * 0.4f
                                 : begAnimationProgress + (0.0f - begAnimationProgress) * 0.4f;
  if(begging) {
    lookTimer = 10;
  }
  if(isWet()) {
    furWet = true;
    shakingWaterOff = false;
    shakeProgress = 0.0f;
    lastShakeProgress = 0.0f;
  } else if((furWet || shakingWaterOff) && shakingWaterOff) {
    if(shakeProgress == 0.0f && world != nullptr) {
      world->playSound(this, "mob.wolf.shake", getSoundVolume(),
                       (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
    }
    lastShakeProgress = shakeProgress;
    shakeProgress += 0.05f;
    if(lastShakeProgress >= 2.0f) {
      furWet = false;
      shakingWaterOff = false;
      lastShakeProgress = 0.0f;
      shakeProgress = 0.0f;
    }
    if(shakeProgress > 0.4f && world != nullptr) {
      const float baseY = static_cast<float>(boundingBox.minY);
      const int particleCount = static_cast<int>(MathHelper::sin((shakeProgress - 0.4f) * kPiF) * 7.0f);
      for(int i = 0; i < particleCount; ++i) {
        const float offsetX = (random.nextFloat() * 2.0f - 1.0f) * width * 0.5f;
        const float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * width * 0.5f;
        world->addParticle("splash", x + static_cast<double>(offsetX), static_cast<double>(baseY) + 0.8f,
                           z + static_cast<double>(offsetZ), velocityX, velocityY, velocityZ);
      }
    }
  }
}
void WolfEntity::tickMovement() {
  AnimalEntity::tickMovement();
  begging = false;
  if(hasLookTarget() && !hasPath() && !isAngry()) {
    if(auto* player = dynamic_cast<player::PlayerEntity*>(getLookTarget())) {
      ItemStack* itemStack = player->inventory.getSelectedItem();
      if(itemStack != nullptr) {
        if(!isTamed() && Item::byRawId(96) != nullptr && itemStack->itemId == Item::byRawId(96)->id) {
          begging = true;
        } else if(isTamed() && itemStack->itemId >= 0 && itemStack->itemId < Item::ITEM_COUNT) {
          if(auto* food =
                 dynamic_cast<item::FoodItem*>(Item::ITEMS[static_cast<std::size_t>(itemStack->itemId)])) {
            begging = food->isMeat();
          }
        }
      }
    }
  }
  if(!interpolateOnly && furWet && !shakingWaterOff && !hasPath() && onGround) {
    shakingWaterOff = true;
    shakeProgress = 0.0f;
    lastShakeProgress = 0.0f;
    if(world != nullptr) {
      world->broadcastEntityEvent(this, 8);
    }
  }
}
bool WolfEntity::isMovementBlocked() const {
  return isInSittingPose() || shakingWaterOff;
}
Entity* WolfEntity::getTargetInRange() {
  if(isAngry() && world != nullptr) {
    return world->getClosestPlayer(this, 16.0);
  }
  return nullptr;
}
void WolfEntity::attack(Entity* other, float distance) {
  if(distance > 2.0f && distance < 6.0f && random.nextInt(10) == 0) {
    if(onGround && other != nullptr) {
      const double deltaX = other->x - x;
      const double deltaZ = other->z - z;
      const float flatDistance = MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaZ * deltaZ));
      if(flatDistance > 1.0e-4f) {
        velocityX = deltaX / static_cast<double>(flatDistance) * 0.5 * 0.8 + velocityX * 0.2;
        velocityZ = deltaZ / static_cast<double>(flatDistance) * 0.5 * 0.8 + velocityZ * 0.2;
        velocityY = 0.4;
      }
    }
  } else if(distance < 1.5f && other != nullptr && other->boundingBox.maxY > boundingBox.minY &&
            other->boundingBox.minY < boundingBox.maxY) {
    attackCooldown = 20;
    other->damage(this, isTamed() ? 4 : 2);
  }
}
void WolfEntity::tickLiving() {
  AnimalEntity::tickLiving();
  if(!movementBlocked && !hasPath() && isTamed() && vehicle == nullptr && world != nullptr) {
    PlayerEntity* owner = world->getPlayer(getOwnerName());
    if(owner != nullptr) {
      followOwner(getDistance(*owner));
    } else if(!isSubmergedInWater()) {
      setSitting(true);
    }
  } else if(target == nullptr && !hasPath() && !isTamed() && world != nullptr && world->random().nextInt(100) == 0) {
    const Box searchBox = Box(x, y, z, x + 1.0, y + 1.0, z + 1.0).expand(16.0, 4.0, 16.0);
    const std::vector<Entity*> nearby = world->getEntities(this, searchBox);
    std::vector<SheepEntity*> sheep;
    sheep.reserve(nearby.size());
    for(Entity* entity : nearby) {
      if(auto* candidate = dynamic_cast<SheepEntity*>(entity)) {
        sheep.push_back(candidate);
      }
    }
    if(!sheep.empty()) {
      target = sheep[static_cast<std::size_t>(world->random().nextInt(static_cast<int>(sheep.size())))];
    }
  }
  if(isSubmergedInWater()) {
    setSitting(false);
  }
  if(world != nullptr && !world->isRemote()) {
    dataTracker.set(18, health);
  }
}
void WolfEntity::followOwner(float distance) {
  if(world == nullptr) {
    return;
  }
  PlayerEntity* owner = world->getPlayer(getOwnerName());
  if(owner == nullptr) {
    return;
  }
  if(distance > 5.0f) {
    ai::pathing::Path candidate = world->findPath(this, owner, 16.0f);
    if(candidate.length == 0 && distance > 12.0f) {
      const int baseX = MathHelper::floor(owner->x) - 2;
      const int baseZ = MathHelper::floor(owner->z) - 2;
      const int baseY = MathHelper::floor(owner->boundingBox.minY);
      for(int i = 0; i <= 4; ++i) {
        for(int j = 0; j <= 4; ++j) {
          if((i >= 1 && j >= 1 && i <= 3 && j <= 3) ||
             !world->shouldSuffocate(baseX + i, baseY - 1, baseZ + j) ||
             world->shouldSuffocate(baseX + i, baseY, baseZ + j) ||
             world->shouldSuffocate(baseX + i, baseY + 1, baseZ + j)) {
            continue;
          }
          setPositionAndAnglesKeepPrevAngles(static_cast<double>(baseX + i) + 0.5, static_cast<double>(baseY),
                                             static_cast<double>(baseZ + j) + 0.5, yaw, pitch);
          return;
        }
      }
    } else if(candidate.length > 0) {
      setPath(std::make_unique<ai::pathing::Path>(std::move(candidate)));
    } else {
      path.reset();
    }
  }
}
bool WolfEntity::damage(Entity* damageSource, int amount) {
  setSitting(false);
  Entity* attacker = damageSource;
  if(attacker != nullptr && dynamic_cast<PlayerEntity*>(attacker) == nullptr &&
     dynamic_cast<projectile::ArrowEntity*>(attacker) == nullptr) {
    amount = (amount + 1) / 2;
  }
  if(!AnimalEntity::damage(attacker, amount)) {
    return false;
  }
  if(!isTamed() && !isAngry()) {
    if(dynamic_cast<PlayerEntity*>(attacker) != nullptr) {
      setAngry(true);
      target = attacker;
    }
    if(auto* arrow = dynamic_cast<projectile::ArrowEntity*>(attacker);
       arrow != nullptr && arrow->owner != nullptr) {
      attacker = arrow->owner;
    }
    if(dynamic_cast<LivingEntity*>(attacker) != nullptr && world != nullptr) {
      const Box searchBox = Box(x, y, z, x + 1.0, y + 1.0, z + 1.0).expand(16.0, 4.0, 16.0);
      const std::vector<Entity*> nearby = world->getEntities(this, searchBox);
      for(Entity* entity : nearby) {
        auto* wolf = dynamic_cast<WolfEntity*>(entity);
        if(wolf == nullptr || wolf->isTamed() || wolf->target != nullptr) {
          continue;
        }
        wolf->target = attacker;
        if(dynamic_cast<PlayerEntity*>(attacker) != nullptr) {
          wolf->setAngry(true);
        }
      }
    }
  } else if(attacker != this && attacker != nullptr) {
    if(isTamed() && dynamic_cast<PlayerEntity*>(attacker) != nullptr &&
       namesEqualIgnoreCase(dynamic_cast<PlayerEntity*>(attacker)->name, getOwnerName())) {
      return true;
    }
    target = attacker;
  }
  return true;
}
bool WolfEntity::interact(player::PlayerEntity* player) {
  if(player == nullptr || world == nullptr) {
    return false;
  }
  ItemStack* itemStack = player->inventory.getSelectedItem();
  if(!isTamed()) {
    if(itemStack != nullptr && Item::byRawId(96) != nullptr && itemStack->itemId == Item::byRawId(96)->id &&
       !isAngry()) {
      --itemStack->count;
      if(itemStack->count <= 0) {
        player->inventory.setStack(static_cast<std::size_t>(player->inventory.selectedSlot), {});
      }
      if(!world->isRemote()) {
        if(random.nextInt(3) == 0) {
          setTamed(true);
          path.reset();
          setSitting(true);
          health = 20;
          dataTracker.set(18, health);
          setOwnerName(player->name);
          showFeedParticles(true);
          world->broadcastEntityEvent(this, 7);
        } else {
          showFeedParticles(false);
          world->broadcastEntityEvent(this, 6);
        }
      }
      return true;
    }
  } else {
    if(itemStack != nullptr && itemStack->itemId >= 0 && itemStack->itemId < Item::ITEM_COUNT) {
      if(auto* food = dynamic_cast<item::FoodItem*>(Item::ITEMS[static_cast<std::size_t>(itemStack->itemId)])) {
        if(food->isMeat() && dataTracker.getInt(18) < 20) {
          --itemStack->count;
          if(itemStack->count <= 0) {
            player->inventory.setStack(static_cast<std::size_t>(player->inventory.selectedSlot), {});
          }
          const int healAmount = Item::byRawId(63) != nullptr
                                     ? dynamic_cast<item::FoodItem*>(Item::byRawId(63))->getHealthRestored()
                                     : 3;
          heal(healAmount);
          return true;
        }
      }
    }
    if(namesEqualIgnoreCase(player->name, getOwnerName())) {
      if(!world->isRemote()) {
        setSitting(!isInSittingPose());
        jumping = false;
        path.reset();
      }
      return true;
    }
  }
  return false;
}
void WolfEntity::writeNbt(NbtCompound& nbt) const {
  LivingEntity::writeNbt(nbt);
  nbt.putBoolean("Angry", isAngry());
  nbt.putBoolean("Sitting", isInSittingPose());
  if(getOwnerName().empty()) {
    nbt.putString("Owner", "");
  } else {
    nbt.putString("Owner", getOwnerName());
  }
}
void WolfEntity::readNbt(const NbtCompound& nbt) {
  LivingEntity::readNbt(nbt);
  setAngry(nbt.getBoolean("Angry"));
  setSitting(nbt.getBoolean("Sitting"));
  const std::string owner = nbt.getString("Owner");
  if(!owner.empty()) {
    setOwnerName(owner);
    setTamed(true);
  }
}
void WolfEntity::processServerEntityStatus(std::int8_t status) {
  if(status == 7) {
    showFeedParticles(true);
    return;
  }
  if(status == 6) {
    showFeedParticles(false);
    return;
  }
  if(status == 8) {
    shakingWaterOff = true;
    shakeProgress = 0.0f;
    lastShakeProgress = 0.0f;
    return;
  }
  LivingEntity::processServerEntityStatus(status);
}
void WolfEntity::showFeedParticles(bool hearts) {
  if(world == nullptr) {
    return;
  }
  const char* particle = hearts ? "heart" : "smoke";
  for(int i = 0; i < 7; ++i) {
    const double offsetX = random.nextGaussian() * 0.02;
    const double offsetY = random.nextGaussian() * 0.02;
    const double offsetZ = random.nextGaussian() * 0.02;
    world->addParticle(particle, x + static_cast<double>(random.nextFloat() * width * 2.0f - width),
                       y + 0.5 + static_cast<double>(random.nextFloat() * height),
                       z + static_cast<double>(random.nextFloat() * width * 2.0f - width), offsetX, offsetY,
                       offsetZ);
  }
}
bool WolfEntity::namesEqualIgnoreCase(const std::string& left, const std::string& right) {
  if(left.size() != right.size()) {
    return false;
  }
  for(std::size_t i = 0; i < left.size(); ++i) {
    if(std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i]))) {
      return false;
    }
  }
  return true;
}
int WolfEntity::getMaxLookPitchChange() const {
  return isInSittingPose() ? 20 : LivingEntity::getMaxLookPitchChange();
}
int WolfEntity::getDroppedItemId() const {
  return -1;
}
MC_REGISTER_ENTITY(WolfEntity)
} // namespace net::minecraft::entity::passive
