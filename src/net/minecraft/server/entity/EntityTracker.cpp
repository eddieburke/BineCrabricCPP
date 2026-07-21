#include "net/minecraft/server/entity/EntityTracker.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/SpawnableEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
namespace net::minecraft::server {
namespace {
namespace mcentity = ::net::minecraft::entity;
using mcentity::player::ServerPlayerEntity;
} // namespace
EntityTracker::EntityTracker(MinecraftServer* server, int dimensionId) : server_(server), dimensionId_(dimensionId) {
 viewDistance_ = server != nullptr ? server->playerManager.getBlockViewDistance() : 144;
}
std::vector<ServerPlayerEntity*> EntityTracker::dimensionPlayers() const {
 std::vector<ServerPlayerEntity*> players;
 if(server_ == nullptr) {
  return players;
 }
 for(ServerPlayerEntity* player : server_->playerManager.players) {
  if(player != nullptr && player->dimensionId == dimensionId_) {
   players.push_back(player);
  }
 }
 return players;
}
void EntityTracker::onEntityAdded(mcentity::Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 if(auto* serverPlayer = dynamic_cast<ServerPlayerEntity*>(entity)) {
  startTracking(entity, 512, 2);
  for(const auto& entry : entries_) {
   if(entry->currentTrackedEntity != serverPlayer) {
    entry->updateListener(serverPlayer);
   }
  }
  return;
 }
 if(dynamic_cast<mcentity::projectile::FishingBobberEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 5, true);
 } else if(dynamic_cast<mcentity::projectile::ArrowEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 20, false);
 } else if(dynamic_cast<mcentity::projectile::FireballEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 10, false);
 } else if(dynamic_cast<mcentity::projectile::thrown::SnowballEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 10, true);
 } else if(dynamic_cast<mcentity::projectile::thrown::EggEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 10, true);
 } else if(dynamic_cast<mcentity::ItemEntity*>(entity) != nullptr) {
  startTracking(entity, 64, 20, true);
 } else if(dynamic_cast<mcentity::vehicle::MinecartEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 5, true);
 } else if(dynamic_cast<mcentity::vehicle::BoatEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 5, true);
 } else if(dynamic_cast<mcentity::passive::SquidEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 3, true);
 } else if(dynamic_cast<mcentity::SpawnableEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 3);
 } else if(dynamic_cast<mcentity::TntEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 10, true);
 } else if(dynamic_cast<mcentity::FallingBlockEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 20, true);
 } else if(dynamic_cast<mcentity::decoration::painting::PaintingEntity*>(entity) != nullptr) {
  startTracking(entity, 160, INT_MAX, false);
 } else if(dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(entity) != nullptr) {
  startTracking(entity, 160, 3, true);
 }
}
void EntityTracker::startTracking(mcentity::Entity* entity, int trackedDistance, int trackingFrequency) {
 startTracking(entity, trackedDistance, trackingFrequency, false);
}
void EntityTracker::startTracking(mcentity::Entity* entity,
                                  int trackedDistance,
                                  int trackingFrequency,
                                  bool alwaysUpdateVelocity) {
 if(entity == nullptr) {
  return;
 }
 if(trackedDistance > viewDistance_) {
  trackedDistance = viewDistance_;
 }
 if(entriesById_.get(entity->id) != nullptr) {
  throw std::logic_error("Entity is already tracked!");
 }
 auto entry =
     std::make_unique<entity::EntityTrackerEntry>(entity, trackedDistance, trackingFrequency, alwaysUpdateVelocity);
 entriesById_.put(entity->id, entry.get());
 entry->updateListeners(dimensionPlayers());
 entries_.push_back(std::move(entry));
}
void EntityTracker::onEntityRemoved(mcentity::Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 if(auto* serverPlayer = dynamic_cast<ServerPlayerEntity*>(entity)) {
  for(const auto& entry : entries_) {
   entry->notifyEntityRemoved(serverPlayer);
  }
 }
 entity::EntityTrackerEntry* entry = entriesById_.get(entity->id);
 if(entry != nullptr) {
  entriesById_.remove(entity->id);
  entry->notifyEntityRemoved();
  entries_.erase(std::remove_if(entries_.begin(),
                                entries_.end(),
                                [entry](const std::unique_ptr<entity::EntityTrackerEntry>& owned) {
                                 return owned.get() == entry;
                                }),
                 entries_.end());
 }
}
void EntityTracker::tick() {
 const std::vector<ServerPlayerEntity*> players = dimensionPlayers();
 std::vector<ServerPlayerEntity*> movedPlayers;
 for(const auto& entry : entries_) {
  entry->notifyNewLocation(players);
  if(entry->newPlayerDataUpdated) {
   if(auto* serverPlayer = dynamic_cast<ServerPlayerEntity*>(entry->currentTrackedEntity)) {
    movedPlayers.push_back(serverPlayer);
   }
  }
 }
 for(ServerPlayerEntity* movedPlayer : movedPlayers) {
  for(const auto& entry : entries_) {
   if(entry->currentTrackedEntity != movedPlayer) {
    entry->updateListener(movedPlayer);
   }
  }
 }
}
void EntityTracker::removeListener(ServerPlayerEntity* player) {
 for(const auto& entry : entries_) {
  entry->removeListener(player);
 }
}
} // namespace net::minecraft::server
