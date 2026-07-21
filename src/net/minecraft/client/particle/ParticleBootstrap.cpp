#include "net/minecraft/client/particle/ExplosionParticle.hpp"
#include "net/minecraft/client/particle/FireSmokeParticle.hpp"
#include "net/minecraft/client/particle/FlameParticle.hpp"
#include "net/minecraft/client/particle/FootstepParticle.hpp"
#include "net/minecraft/client/particle/HeartParticle.hpp"
#include "net/minecraft/client/particle/ItemParticle.hpp"
#include "net/minecraft/client/particle/LavaEmberParticle.hpp"
#include "net/minecraft/client/particle/NoteParticle.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/particle/ParticleRegistry.hpp"
#include "net/minecraft/client/particle/PortalParticle.hpp"
#include "net/minecraft/client/particle/RainSplashParticle.hpp"
#include "net/minecraft/client/particle/RedDustParticle.hpp"
#include "net/minecraft/client/particle/SnowParticle.hpp"
#include "net/minecraft/client/particle/WaterBubbleParticle.hpp"
#include "net/minecraft/client/particle/WaterSplashParticle.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::client::particle {
namespace {
void registerVanillaParticles() {
 ParticleRegistry& registry = ParticleRegistry::instance();
 registry.registerFactory("bubble", [](const ParticleSpawnContext& c) {
  return std::make_unique<WaterBubbleParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("smoke", [](const ParticleSpawnContext& c) {
  return std::make_unique<FireSmokeParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("explode", [](const ParticleSpawnContext& c) {
  return std::make_unique<ExplosionParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("flame", [](const ParticleSpawnContext& c) {
  return std::make_unique<FlameParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("splash", [](const ParticleSpawnContext& c) {
  return std::make_unique<WaterSplashParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("rainsplash", [](const ParticleSpawnContext& c) {
  return std::make_unique<RainSplashParticle>(c.world, c.x, c.y, c.z);
 });
 registry.registerFactory("largesmoke", [](const ParticleSpawnContext& c) {
  return std::make_unique<FireSmokeParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ, 2.5f);
 });
 registry.registerFactory("reddust", [](const ParticleSpawnContext& c) {
  return std::make_unique<RedDustParticle>(c.world,
                                           c.x,
                                           c.y,
                                           c.z,
                                           static_cast<float>(c.velocityX),
                                           static_cast<float>(c.velocityY),
                                           static_cast<float>(c.velocityZ));
 });
 registry.registerFactory("note", [](const ParticleSpawnContext& c) {
  return std::make_unique<NoteParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("lava", [](const ParticleSpawnContext& c) {
  return std::make_unique<LavaEmberParticle>(c.world, c.x, c.y, c.z);
 });
 registry.registerFactory("heart", [](const ParticleSpawnContext& c) {
  return std::make_unique<HeartParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("portal", [](const ParticleSpawnContext& c) {
  return std::make_unique<PortalParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("footstep", [](const ParticleSpawnContext& c) {
  return std::make_unique<FootstepParticle>(c.textureManager, c.world, c.x, c.y, c.z);
 });
 registry.registerFactory("snowballpoof", [](const ParticleSpawnContext& c) {
  return std::make_unique<ItemParticle>(c.world, c.x, c.y, c.z, Item::byRawId(76));
 });
 registry.registerFactory("snowshovel", [](const ParticleSpawnContext& c) {
  return std::make_unique<SnowParticle>(c.world, c.x, c.y, c.z, c.velocityX, c.velocityY, c.velocityZ);
 });
 registry.registerFactory("slime", [](const ParticleSpawnContext& c) {
  return std::make_unique<ItemParticle>(c.world, c.x, c.y, c.z, Item::byRawId(85));
 });
}
struct VanillaParticleBootstrap {
 static void registerClass() {
  registerVanillaParticles();
 }
};
static registry::RegisterPhase<VanillaParticleBootstrap> s_particles(mod::LifecyclePhase::Init, 0);
}
} // namespace net::minecraft::client::particle
