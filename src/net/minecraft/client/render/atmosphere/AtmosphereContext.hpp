#pragma once
namespace net::minecraft {
class World;
}
namespace net::minecraft::entity {
class Entity;
class LivingEntity;
} // namespace net::minecraft::entity
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::client::option {
struct RenderSettings;
}
namespace net::minecraft::client::render::atmosphere {
struct AtmosphereContext {
 net::minecraft::client::Minecraft* client;
 net::minecraft::World* world;
 net::minecraft::client::texture::TextureManager* textureManager;
 net::minecraft::entity::Entity* camera;
 const net::minecraft::entity::LivingEntity* livingCamera;
 const net::minecraft::client::option::RenderSettings& settings;
 int atmosphereTicks;
};
} // namespace net::minecraft::client::render::atmosphere
