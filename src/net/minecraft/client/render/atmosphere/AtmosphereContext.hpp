#pragma once

#include "net/minecraft/client/render/ViewDistance.hpp"

namespace net::minecraft {
class World;
}

namespace net::minecraft::entity {
class Entity;
class LivingEntity;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::client::render::atmosphere {

struct AtmosphereContext {
    net::minecraft::client::Minecraft* client;
    net::minecraft::World* world;
    net::minecraft::client::texture::TextureManager* textureManager;
    net::minecraft::entity::Entity* camera;
    const net::minecraft::entity::LivingEntity* livingCamera;
    const net::minecraft::client::option::GameOptions& options;
    int atmosphereTicks;
    net::minecraft::client::render::ViewDistance viewDistance;
};

} // namespace net::minecraft::client::render::atmosphere
