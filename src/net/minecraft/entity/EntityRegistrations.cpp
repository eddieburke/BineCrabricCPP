// Vanilla entity type registry — mirrors Java EntityRegistry static block.
// Mods: add registerVanillaEntity<YourEntity>(...) in a separate TU linked into the target.

#include "net/minecraft/entity/EntityRegistrar.hpp"

#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
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
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"

namespace {

using net::minecraft::entity::detail::registerVanillaEntity;
using net::minecraft::entity::FallingBlockEntity;
using net::minecraft::entity::ItemEntity;
using net::minecraft::entity::LivingEntity;
using net::minecraft::entity::TntEntity;
using net::minecraft::entity::decoration::painting::PaintingEntity;
using net::minecraft::entity::mob::CreeperEntity;
using net::minecraft::entity::mob::GhastEntity;
using net::minecraft::entity::mob::GiantEntity;
using net::minecraft::entity::mob::MonsterEntity;
using net::minecraft::entity::mob::PigZombieEntity;
using net::minecraft::entity::mob::SkeletonEntity;
using net::minecraft::entity::mob::SlimeEntity;
using net::minecraft::entity::mob::SpiderEntity;
using net::minecraft::entity::mob::ZombieEntity;
using net::minecraft::entity::passive::ChickenEntity;
using net::minecraft::entity::passive::CowEntity;
using net::minecraft::entity::passive::PigEntity;
using net::minecraft::entity::passive::SheepEntity;
using net::minecraft::entity::passive::SquidEntity;
using net::minecraft::entity::passive::WolfEntity;
using net::minecraft::entity::projectile::ArrowEntity;
using net::minecraft::entity::projectile::thrown::SnowballEntity;
using net::minecraft::entity::vehicle::BoatEntity;
using net::minecraft::entity::vehicle::MinecartEntity;

struct VanillaEntityRegistrations {
    VanillaEntityRegistrations()
    {
        registerVanillaEntity<ArrowEntity>("Arrow", 10);
        registerVanillaEntity<SnowballEntity>("Snowball", 11);
        registerVanillaEntity<ItemEntity>("Item", 1);
        registerVanillaEntity<PaintingEntity>("Painting", 9);
        registerVanillaEntity<LivingEntity>("Mob", 48);
        registerVanillaEntity<MonsterEntity>("Monster", 49);
        registerVanillaEntity<CreeperEntity>("Creeper", 50);
        registerVanillaEntity<SkeletonEntity>("Skeleton", 51);
        registerVanillaEntity<SpiderEntity>("Spider", 52);
        registerVanillaEntity<GiantEntity>("Giant", 53);
        registerVanillaEntity<ZombieEntity>("Zombie", 54);
        registerVanillaEntity<SlimeEntity>("Slime", 55);
        registerVanillaEntity<GhastEntity>("Ghast", 56);
        registerVanillaEntity<PigZombieEntity>("PigZombie", 57);
        registerVanillaEntity<TntEntity>("PrimedTnt", 20);
        registerVanillaEntity<FallingBlockEntity>("FallingSand", 21);
        registerVanillaEntity<MinecartEntity>("Minecart", 40);
        registerVanillaEntity<BoatEntity>("Boat", 41);
        registerVanillaEntity<PigEntity>("Pig", 90);
        registerVanillaEntity<SheepEntity>("Sheep", 91);
        registerVanillaEntity<CowEntity>("Cow", 92);
        registerVanillaEntity<ChickenEntity>("Chicken", 93);
        registerVanillaEntity<SquidEntity>("Squid", 94);
        registerVanillaEntity<WolfEntity>("Wolf", 95);
    }
} s_vanillaEntityRegistrations;

} // namespace
