#pragma once
#include "net/minecraft/entity/SpawnableEntity.hpp"

namespace net::minecraft::entity {
class Monster : public SpawnableEntity {
   public:
    ~Monster() override = default;
};
}  // namespace net::minecraft::entity
