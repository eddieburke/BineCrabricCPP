#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
namespace net::minecraft {
class World;
class PortalForcer {
public:
  void moveToPortal(World* world, Entity* entity);
  bool teleportToValidPortal(World* world, Entity* entity);
  bool createPortal(World* world, Entity* entity);

private:
  JavaRandom random_;
};
} // namespace net::minecraft
