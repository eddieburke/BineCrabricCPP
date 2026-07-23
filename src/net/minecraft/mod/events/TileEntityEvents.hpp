#pragma once
#include <cstdint>
namespace net::minecraft {
class World;
namespace block::entity {
class BlockEntity;
} // namespace block::entity
} // namespace net::minecraft
namespace net::minecraft::mod {
struct PreTileEntityRenderEvent {
 const block::entity::BlockEntity* entity = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 std::string id;
 float tickDelta = 0.0f;
 bool canceled = false;
};
struct TileEntityTickEvent {
 World* world = nullptr;
 block::entity::BlockEntity* entity = nullptr;
 bool remote = false;
 bool canceled = false;
 bool animationTicked = false;
};
} // namespace net::minecraft::mod
