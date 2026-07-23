#pragma once
#include <string>
namespace net::minecraft {
class ItemStack;
class World;
namespace entity {
class Entity;
namespace player {
class PlayerEntity;
}
} // namespace entity
namespace client {
class Minecraft;
}
} // namespace net::minecraft
namespace net::minecraft::mod {
struct BlockInteractEvent {
 entity::player::PlayerEntity* player = nullptr;
 World* world = nullptr;
 ItemStack* stack = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 int side = 0;
 bool rightClick = true;
 bool canceled = false;
 bool handled = false;
 client::Minecraft* client = nullptr;
};
struct EntityInteractEvent {
 entity::player::PlayerEntity* player = nullptr;
 entity::Entity* target = nullptr;
 bool attack = false;
 bool canceled = false;
 bool handled = false;
 bool sneaking = false;
 ItemStack* stack = nullptr;
};
struct EntityTeleportEvent {
 entity::Entity* entity = nullptr;
 World* world = nullptr;
 double fromX = 0.0;
 double fromY = 0.0;
 double fromZ = 0.0;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 float yaw = 0.0f;
 float pitch = 0.0f;
 bool canceled = false;
 client::Minecraft* client = nullptr;
};
struct AttackDamageEvent {
 entity::player::PlayerEntity* player = nullptr;
 entity::Entity* target = nullptr;
 int damage = 0;
 bool critical = false;
 bool canceled = false;
 float fallDistance = 0.0f;
 bool onGround = false;
 double targetX = 0.0;
 double targetY = 0.0;
 double targetZ = 0.0;
};
struct PlayerTravelEvent {
 entity::player::PlayerEntity* player = nullptr;
 float sideways = 0.0f;
 float forward = 0.0f;
 float speedMultiplier = 1.0f;
};
struct CraftingTakeEvent {
 entity::player::PlayerEntity* player = nullptr;
 ItemStack* stack = nullptr;
 bool canceled = false;
};
struct FurnaceOutputTakeEvent {
 entity::player::PlayerEntity* player = nullptr;
 ItemStack* stack = nullptr;
 bool canceled = false;
};
struct EntitySpawnEvent {
 entity::Entity* entity = nullptr;
 int entityId = 0;
 std::string entityType;
};
struct EntityRemoveEvent {
 entity::Entity* entity = nullptr;
 int entityId = 0;
 std::string entityType;
};
struct EntityTickEvent {
 World* world = nullptr;
 entity::Entity* entity = nullptr;
 bool remote = false;
 bool canceled = false;
};
} // namespace net::minecraft::mod
