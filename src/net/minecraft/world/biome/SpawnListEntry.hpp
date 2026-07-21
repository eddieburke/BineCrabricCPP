#pragma once
#include <string>
namespace net::minecraft {
struct SpawnListEntry {
 std::string entityType;
 int itemWeight = 0;
 SpawnListEntry() = default;
 SpawnListEntry(std::string entityType, int itemWeight)
     : entityType(std::move(entityType)), itemWeight(itemWeight) {
 }
};
} // namespace net::minecraft
