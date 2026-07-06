#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/FlyingEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/MobEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/multiplayer/OtherPlayerEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/mob/PigZombieEntity.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/mob/SlimeEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/mob/ZombieEntity.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include <unordered_map>
namespace net::minecraft {
namespace {
using client::multiplayer::MultiplayerClientPlayerEntity;
using client::multiplayer::OtherPlayerEntity;
using entity::Entity;
using entity::FallingBlockEntity;
using entity::FlyingEntity;
using entity::ItemEntity;
using entity::LightningEntity;
using entity::LivingEntity;
using entity::MobEntity;
using entity::TntEntity;
using entity::WaterCreatureEntity;
using entity::decoration::painting::PaintingEntity;
using entity::mob::CreeperEntity;
using entity::mob::GhastEntity;
using entity::mob::GiantEntity;
using entity::mob::MonsterEntity;
using entity::mob::PigZombieEntity;
using entity::mob::SkeletonEntity;
using entity::mob::SlimeEntity;
using entity::mob::SpiderEntity;
using entity::mob::ZombieEntity;
using entity::passive::ChickenEntity;
using entity::passive::CowEntity;
using entity::passive::PigEntity;
using entity::passive::SheepEntity;
using entity::passive::SquidEntity;
using entity::passive::WolfEntity;
using entity::player::PlayerEntity;
using entity::player::ServerPlayerEntity;
using entity::projectile::ArrowEntity;
using entity::projectile::FireballEntity;
using entity::projectile::FishingBobberEntity;
using entity::projectile::thrown::EggEntity;
using entity::projectile::thrown::SnowballEntity;
using entity::vehicle::BoatEntity;
using entity::vehicle::MinecartEntity;
template <typename Child, typename Parent>
void linkParent(std::unordered_map<std::type_index, std::type_index>& map) {
  map.emplace(std::type_index(typeid(Child)), std::type_index(typeid(Parent)));
}
const std::unordered_map<std::type_index, std::type_index>& parentMap() {
  static const std::unordered_map<std::type_index, std::type_index> map = [] {
    std::unordered_map<std::type_index, std::type_index> parents;
    linkParent<PlayerEntity, LivingEntity>(parents);
    linkParent<ServerPlayerEntity, PlayerEntity>(parents);
    linkParent<OtherPlayerEntity, PlayerEntity>(parents);
    // The MP local player is a MultiplayerClientPlayerEntity (subclass of
    // ClientPlayerEntity). Without this link the renderer dispatcher finds no renderer
    // for its exact type, so the player model never draws in MP (the inventory preview
    // and any third-person view). SP works only because its player is a plain
    // ClientPlayerEntity, which the dispatcher aliases explicitly.
    linkParent<MultiplayerClientPlayerEntity, PlayerEntity>(parents);
    linkParent<PigZombieEntity, ZombieEntity>(parents);
    linkParent<ZombieEntity, MonsterEntity>(parents);
    linkParent<CreeperEntity, MonsterEntity>(parents);
    linkParent<SkeletonEntity, MonsterEntity>(parents);
    linkParent<SpiderEntity, MonsterEntity>(parents);
    linkParent<GhastEntity, FlyingEntity>(parents);
    linkParent<FlyingEntity, LivingEntity>(parents);
    linkParent<GiantEntity, MonsterEntity>(parents);
    linkParent<SlimeEntity, LivingEntity>(parents);
    linkParent<MonsterEntity, MobEntity>(parents);
    linkParent<PigEntity, MobEntity>(parents);
    linkParent<SheepEntity, MobEntity>(parents);
    linkParent<CowEntity, MobEntity>(parents);
    linkParent<ChickenEntity, MobEntity>(parents);
    linkParent<SquidEntity, WaterCreatureEntity>(parents);
    linkParent<WaterCreatureEntity, MobEntity>(parents);
    linkParent<WolfEntity, MobEntity>(parents);
    linkParent<MobEntity, LivingEntity>(parents);
    linkParent<LivingEntity, Entity>(parents);
    linkParent<ArrowEntity, Entity>(parents);
    linkParent<SnowballEntity, Entity>(parents);
    linkParent<EggEntity, Entity>(parents);
    linkParent<FireballEntity, Entity>(parents);
    linkParent<FishingBobberEntity, Entity>(parents);
    linkParent<ItemEntity, Entity>(parents);
    linkParent<TntEntity, Entity>(parents);
    linkParent<FallingBlockEntity, Entity>(parents);
    linkParent<MinecartEntity, Entity>(parents);
    linkParent<BoatEntity, Entity>(parents);
    linkParent<PaintingEntity, Entity>(parents);
    linkParent<LightningEntity, Entity>(parents);
    return parents;
  }();
  return map;
}
} // namespace
std::optional<std::type_index> entitySupertype(std::type_index type) {
  const auto it = parentMap().find(type);
  if(it == parentMap().end()) {
    return std::nullopt;
  }
  return it->second;
}
} // namespace net::minecraft
