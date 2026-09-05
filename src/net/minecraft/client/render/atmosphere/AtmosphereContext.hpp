#pragma once
#include "net/minecraft/util/math/Types.hpp"
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
 const net::minecraft::client::option::RenderSettings& settings;
 int atmosphereTicks;
 double cameraX = 0.0;
 double cameraZ = 0.0;
 Vec3d skyColor{1.0, 1.0, 1.0};
 Vec3d cloudColor{1.0, 1.0, 1.0};
 float rainStrength = 0.0f;
 float skyLightIntensity = 0.0f;
 float starBrightness = 0.0f;
 float cloudBaseHeight = 128.0f;
 float cloudRenderHeight = 128.0f;
};
} // namespace net::minecraft::client::render::atmosphere
