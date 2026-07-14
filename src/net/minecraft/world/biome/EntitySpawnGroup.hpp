#pragma once
#include <string>
namespace net::minecraft {
struct EntitySpawnGroup {
  std::string entityType;
  int amount = 0;
  EntitySpawnGroup() = default;
  EntitySpawnGroup(std::string entityType, int amount) : entityType(std::move(entityType)), amount(amount) {
  }
};
} // namespace net::minecraft
