#pragma once
#include "net/minecraft/util/hit/HitResultType.hpp"

namespace net::minecraft {
class World;

namespace client {
class Minecraft;
}

namespace entity {
class Entity;
class LivingEntity;

namespace player {
class PlayerEntity;
}
}  // namespace entity
}  // namespace net::minecraft

namespace net::minecraft::client::gui::screen {
class Screen;
}

namespace net::minecraft::mod {
struct RaycastEvent {
    client::Minecraft* client = nullptr;
    entity::player::PlayerEntity* player = nullptr;
    World* world = nullptr;
    bool hasHit = false;
    HitResultType type = HitResultType::BLOCK;
    int blockX = 0;
    int blockY = 0;
    int blockZ = 0;
    int side = 0;
    int blockId = 0;
    double hitX = 0.0;
    double hitY = 0.0;
    double hitZ = 0.0;
    entity::Entity* entity = nullptr;
};

struct ClientTickEvent {
    client::Minecraft* client = nullptr;
    entity::player::PlayerEntity* player = nullptr;
    World* world = nullptr;
    bool paused = false;
    bool before = true;
    bool afterWorld = false;
};

struct KeyPressEvent {
    int key = 0;
    bool pressed = false;
    bool repeat = false;
    bool handled = false;
};

struct MouseButtonEvent {
    int button = 0;
    bool pressed = false;
    bool handled = false;
};

struct ScreenGuiEvent {
    client::gui::screen::Screen* screen = nullptr;
    bool tickPhase = true;
    float tickDelta = 0.0f;
    int mouseX = 0;
    int mouseY = 0;
};
}  // namespace net::minecraft::mod
