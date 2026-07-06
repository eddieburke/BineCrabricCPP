#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
namespace net::minecraft::entity {
// Faithful port of net.minecraft.entity.EntityRegistry.
class EntityRegistry {
public:
  using Factory = std::function<std::unique_ptr<Entity>(World*)>;
  // Full registration: populates all four lookup maps (id, rawId, typeIndex→id, typeIndex→rawId).
  static void registerType(std::type_index typeIdx, const std::string& id, int rawId, Factory factory);
  [[nodiscard]] static std::unique_ptr<Entity> create(const std::string& id, World* world);
  [[nodiscard]] static std::unique_ptr<Entity> create(int rawId, World* world);
  [[nodiscard]] static std::unique_ptr<Entity> getEntityFromNbt(const NbtCompound& nbt, World* world);
  [[nodiscard]] static int getRawId(const Entity& entity);
  [[nodiscard]] static std::string getId(const Entity& entity);
};
} // namespace net::minecraft::entity
