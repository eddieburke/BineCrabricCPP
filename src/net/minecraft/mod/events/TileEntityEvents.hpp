#pragma once
#include <cstdint>
namespace net::minecraft {
class World;
namespace block::entity {
class BlockEntity;
} // namespace block::entity
} // namespace net::minecraft
namespace net::minecraft::mod {
struct TileEntityTickEvent {
  World* world = nullptr;
  block::entity::BlockEntity* entity = nullptr;
  bool clientWorld = false;
  bool canceled = false;
  bool animationTicked = false;
};
} // namespace net::minecraft::mod
