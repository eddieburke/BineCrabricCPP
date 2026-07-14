#include "net/minecraft/client/particle/ParticleRegistry.hpp"
#include <cassert>
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::client::particle {
ParticleRegistry& ParticleRegistry::instance() {
  static ParticleRegistry registry;
  return registry;
}
void ParticleRegistry::registerFactory(std::string id, Factory factory) {
  assert(!factories_.contains(id) && "ParticleRegistry: duplicate particle id");
  factories_.emplace(std::move(id), std::move(factory));
}
std::unique_ptr<Particle> ParticleRegistry::create(const std::string& id, const ParticleSpawnContext& context) const {
  assert(registry::Registry::isBootstrapped() && "ParticleRegistry: bootstrap not complete");
  const auto it = factories_.find(id);
  if(it == factories_.end()) {
    return nullptr;
  }
  return it->second(context);
}
} // namespace net::minecraft::client::particle
