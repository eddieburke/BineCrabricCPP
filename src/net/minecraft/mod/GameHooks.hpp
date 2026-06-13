#pragma once

#include "net/minecraft/mod/HookBus.hpp"

namespace net::minecraft {
class ItemStack;
class World;
namespace block {
class Block;
}
namespace entity {
class Entity;
class LivingEntity;
namespace player {
class PlayerEntity;
}
}
}

namespace net::minecraft::mod {

struct WorldTickEvent {
    World* world = nullptr;
    bool clientWorld = false;
    bool before = true;
};

struct WorldTimeEvent {
    World* world = nullptr;
    long long oldTime = 0;
    long long newTime = 0;
    bool canceled = false;
};

struct WeatherCycleEvent {
    World* world = nullptr;
    bool clientWorld = false;
    bool canceled = false;
};

struct LightningStrikeEvent {
    World* world = nullptr;
    int x = 0;
    int y = 0;
    int z = 0;
    bool canceled = false;
};

struct SnowIcePlacementEvent {
    World* world = nullptr;
    int x = 0;
    int y = 0;
    int z = 0;
    bool placeSnow = false;
    bool placeIce = false;
};

struct RandomBlockTickEvent {
    World* world = nullptr;
    block::Block* block = nullptr;
    int x = 0;
    int y = 0;
    int z = 0;
    int blockId = 0;
    bool canceled = false;
};

struct ScheduledBlockTickEvent {
    World* world = nullptr;
    block::Block* block = nullptr;
    int x = 0;
    int y = 0;
    int z = 0;
    int blockId = 0;
    bool instant = false;
    bool canceled = false;
};

struct ScheduleBlockUpdateEvent {
    World* world = nullptr;
    int x = 0;
    int y = 0;
    int z = 0;
    int blockId = 0;
    int tickRate = 0;
    bool canceled = false;
};

struct TickRateEvent {
    float targetTps = 20.0f;
    float tpsScale = 1.0f;
};

struct CameraSetupEvent {
    entity::LivingEntity* camera = nullptr;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float tickDelta = 0.0f;
};

struct FovEvent {
    entity::LivingEntity* camera = nullptr;
    float tickDelta = 0.0f;
    float fov = 70.0f;
};

struct SkyRenderEvent {
    World* world = nullptr;
    entity::Entity* camera = nullptr;
    float tickDelta = 0.0f;
    bool canceled = false;
};

struct FirstPersonHandRenderEvent {
    entity::LivingEntity* camera = nullptr;
    float tickDelta = 0.0f;
    int eye = 0;
    bool canceled = false;
};

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
};

struct EntityInteractEvent {
    entity::player::PlayerEntity* player = nullptr;
    entity::Entity* target = nullptr;
    bool attack = false;
    bool canceled = false;
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

struct ChunkDecorateEvent {
    World* world = nullptr;
    int chunkX = 0;
    int chunkZ = 0;
    bool before = true;
    bool canceled = false;
};

} // namespace net::minecraft::mod
