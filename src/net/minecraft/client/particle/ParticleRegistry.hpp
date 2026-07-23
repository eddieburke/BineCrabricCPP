#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
namespace net::minecraft {
class World;
namespace client::texture {
class TextureManager;
}
namespace client::particle {
class Particle;
struct ParticleSpawnContext {
 World* world = nullptr;
 texture::TextureManager* textureManager = nullptr;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double velocityX = 0.0;
 double velocityY = 0.0;
 double velocityZ = 0.0;
};
class ParticleRegistry {
 public:
 using Factory = std::function<std::unique_ptr<Particle>(const ParticleSpawnContext&)>;
 static ParticleRegistry& instance();
 void registerFactory(std::string id, Factory factory);
 [[nodiscard]] std::unique_ptr<Particle> create(const std::string& id, const ParticleSpawnContext& context) const;

 private:
 std::unordered_map<std::string, Factory> factories_;
};
} // namespace client::particle
} // namespace net::minecraft
