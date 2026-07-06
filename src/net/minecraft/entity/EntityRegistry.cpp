#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/world/World.hpp"
#include <cassert>
#include <iostream>
#include <typeindex>
namespace net::minecraft::entity {
namespace {
using Factory = EntityRegistry::Factory;
std::unordered_map<std::string, Factory>& idToFactory() {
  static std::unordered_map<std::string, Factory> map;
  return map;
}
std::unordered_map<int, Factory>& rawIdToFactory() {
  static std::unordered_map<int, Factory> map;
  return map;
}
std::unordered_map<std::type_index, std::string>& typeToId() {
  static std::unordered_map<std::type_index, std::string> map;
  return map;
}
std::unordered_map<std::type_index, int>& typeToRawId() {
  static std::unordered_map<std::type_index, int> map;
  return map;
}
} // namespace
void EntityRegistry::registerType(std::type_index typeIdx, const std::string& id, int rawId, Factory factory) {
  if(idToFactory().find(id) != idToFactory().end()) {
    assert(false && "EntityRegistry: duplicate id registration");
    return;
  }
  if(rawIdToFactory().find(rawId) != rawIdToFactory().end()) {
    assert(false && "EntityRegistry: duplicate rawId registration");
    return;
  }
  if(typeToId().find(typeIdx) != typeToId().end()) {
    assert(false && "EntityRegistry: duplicate typeIndex registration");
    return;
  }
  if(typeToRawId().find(typeIdx) != typeToRawId().end()) {
    assert(false && "EntityRegistry: duplicate typeIndex rawId registration");
    return;
  }
  idToFactory()[id] = factory;
  rawIdToFactory()[rawId] = factory;
  typeToId()[typeIdx] = id;
  typeToRawId()[typeIdx] = rawId;
}
std::unique_ptr<Entity> EntityRegistry::create(const std::string& id, World* world) {
  const auto it = idToFactory().find(id);
  if(it == idToFactory().end()) {
    return nullptr;
  }
  return it->second(world);
}
std::unique_ptr<Entity> EntityRegistry::create(const int rawId, World* world) {
  const auto it = rawIdToFactory().find(rawId);
  if(it == rawIdToFactory().end()) {
    std::cout << "Skipping Entity with id " << rawId << '\n';
    return nullptr;
  }
  return it->second(world);
}
std::unique_ptr<Entity> EntityRegistry::getEntityFromNbt(const NbtCompound& nbt, World* world) {
  std::unique_ptr<Entity> entity = create(nbt.getString("id"), world);
  if(entity != nullptr) {
    entity->readNbt(nbt);
  } else {
    std::cout << "Skipping Entity with id " << nbt.getString("id") << '\n';
  }
  return entity;
}
int EntityRegistry::getRawId(const Entity& entity) {
  const auto it = typeToRawId().find(entity.runtimeType());
  if(it == typeToRawId().end()) {
    return 0;
  }
  return it->second;
}
std::string EntityRegistry::getId(const Entity& entity) {
  const auto it = typeToId().find(entity.runtimeType());
  if(it == typeToId().end()) {
    return "";
  }
  return it->second;
}
} // namespace net::minecraft::entity
