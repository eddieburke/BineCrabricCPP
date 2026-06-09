#include "net/minecraft/entity/EntityRegistry.hpp"

#include "net/minecraft/entity/Entity.hpp"
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
#include "net/minecraft/world/World.hpp"

#include <iostream>
#include <typeindex>

namespace net::minecraft::entity {
namespace {

using Factory = EntityRegistry::Factory;

std::unordered_map<std::string, Factory>& idToFactory()
{
    static std::unordered_map<std::string, Factory> map;
    return map;
}

std::unordered_map<int, Factory>& rawIdToFactory()
{
    static std::unordered_map<int, Factory> map;
    return map;
}

std::unordered_map<std::type_index, std::string>& typeToId()
{
    static std::unordered_map<std::type_index, std::string> map;
    return map;
}

std::unordered_map<std::type_index, int>& typeToRawId()
{
    static std::unordered_map<std::type_index, int> map;
    return map;
}

template <typename T>
void registerEntity(const std::string& id, int rawId)
{
    Factory factory = [](World* world) -> std::unique_ptr<Entity> {
        return std::make_unique<T>(world);
    };
    idToFactory()[id] = factory;
    rawIdToFactory()[rawId] = factory;
    typeToId()[std::type_index(typeid(T))] = id;
    typeToRawId()[std::type_index(typeid(T))] = rawId;
}

template <typename T>
void registerEntityById(const std::string& id)
{
    Factory factory = [](World* world) -> std::unique_ptr<Entity> {
        return std::make_unique<T>(world);
    };
    idToFactory()[id] = factory;
    typeToId()[std::type_index(typeid(T))] = id;
}

} // namespace

void EntityRegistry::registerType(const std::string& id, int rawId, Factory factory)
{
    idToFactory()[id] = factory;
    rawIdToFactory()[rawId] = factory;
}

std::unique_ptr<Entity> EntityRegistry::create(const std::string& id, World* world)
{
    const auto it = idToFactory().find(id);
    if (it == idToFactory().end()) {
        return nullptr;
    }
    return it->second(world);
}

std::unique_ptr<Entity> EntityRegistry::create(const int rawId, World* world)
{
    const auto it = rawIdToFactory().find(rawId);
    if (it == rawIdToFactory().end()) {
        std::cout << "Skipping Entity with id " << rawId << '\n';
        return nullptr;
    }
    return it->second(world);
}

std::unique_ptr<Entity> EntityRegistry::getEntityFromNbt(const NbtCompound& nbt, World* world)
{
    std::unique_ptr<Entity> entity = create(nbt.getString("id"), world);
    if (entity != nullptr) {
        entity->readNbt(nbt);
    } else {
        std::cout << "Skipping Entity with id " << nbt.getString("id") << '\n';
    }
    return entity;
}

int EntityRegistry::getRawId(const Entity& entity)
{
    const auto it = typeToRawId().find(entity.runtimeType());
    if (it == typeToRawId().end()) {
        return 0;
    }
    return it->second;
}

std::string EntityRegistry::getId(const Entity& entity)
{
    const auto it = typeToId().find(entity.runtimeType());
    if (it == typeToId().end()) {
        return "";
    }
    return it->second;
}

void EntityRegistry::bootstrap()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    registerEntity<entity::projectile::ArrowEntity>("Arrow", 10);
    registerEntity<entity::projectile::thrown::SnowballEntity>("Snowball", 11);
    registerEntity<ItemEntity>("Item", 1);
    registerEntity<entity::decoration::painting::PaintingEntity>("Painting", 9);
    registerEntity<LivingEntity>("Mob", 48);
    registerEntity<entity::mob::MonsterEntity>("Monster", 49);
    registerEntity<entity::mob::CreeperEntity>("Creeper", 50);
    registerEntity<entity::mob::SkeletonEntity>("Skeleton", 51);
    registerEntity<entity::mob::SpiderEntity>("Spider", 52);
    registerEntity<entity::mob::GiantEntity>("Giant", 53);
    registerEntity<entity::mob::ZombieEntity>("Zombie", 54);
    registerEntity<entity::mob::SlimeEntity>("Slime", 55);
    registerEntity<entity::mob::GhastEntity>("Ghast", 56);
    registerEntity<entity::mob::PigZombieEntity>("PigZombie", 57);
    registerEntity<entity::passive::PigEntity>("Pig", 90);
    registerEntity<entity::passive::SheepEntity>("Sheep", 91);
    registerEntity<entity::passive::CowEntity>("Cow", 92);
    registerEntity<entity::passive::ChickenEntity>("Chicken", 93);
    registerEntity<entity::passive::SquidEntity>("Squid", 94);
    registerEntity<net::minecraft::entity::passive::WolfEntity>("Wolf", 95);
    registerEntity<TntEntity>("PrimedTnt", 20);
    registerEntity<FallingBlockEntity>("FallingSand", 21);
    registerEntity<entity::vehicle::MinecartEntity>("Minecart", 40);
    registerEntity<entity::vehicle::BoatEntity>("Boat", 41);
}

} // namespace net::minecraft::entity
