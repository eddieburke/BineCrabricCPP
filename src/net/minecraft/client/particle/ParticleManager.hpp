#pragma once
#include <algorithm>
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/particle/BlockParticle.hpp"
#include "net/minecraft/client/particle/Particle.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::particle {
class ParticleManager {
 public:
 ParticleManager() = default;
 ParticleManager(World* world, texture::TextureManager* textureManager)
     : world_(world), textureManager_(textureManager) {
 }
 void addParticle(std::unique_ptr<Particle> particle) {
  if(particle == nullptr) {
   return;
  }
  const int group = particle->getGroup();
  auto& bucket = particles_[static_cast<std::size_t>(group)];
  if(bucket.size() >= 4000) {
   bucket.erase(bucket.begin());
  }
  bucket.push_back(std::move(particle));
 }
 void addParticle(Particle* particle) {
  addParticle(std::unique_ptr<Particle>(particle));
 }
 void removeDeadParticles() {
  for(auto& bucket : particles_) {
   const std::size_t tickCount = bucket.size();
   for(std::size_t i = 0; i < tickCount && i < bucket.size(); ++i) {
    if(bucket[i] != nullptr) {
     bucket[i]->tick();
    }
   }
   std::erase_if(bucket, [](const std::unique_ptr<Particle>& particle) {
    return particle == nullptr || particle->dead;
   });
  }
 }
 void render(Entity* entity, float partialTicks, ParticleSheet sheet) {
  const render::RenderType& type = sheet == ParticleSheet::Opaque        ? render::RenderType::particlesOpaque()
                                   : sheet == ParticleSheet::Translucent ? render::RenderType::particlesTranslucent()
                                                                         : render::RenderType::particles();
  const render::RenderPassScope pass(type);
  // Particles must share the exact render origin AND orientation used by
  // terrain and entities (the frame camera), not the passed-in entity's
  // rotation. The frame camera honors mod camera overrides (e.g. a
  // tripod/handheld camera facing a different direction than the player),
  // so billboarding off entity->yaw/pitch here desyncs particle facing from
  // the actual rendered view whenever those rotations differ.
  syncCameraOffset();
  // Particles emit camera-relative quads, so the pose is identity: they are
  // already in pass-base space, exactly like terrain.
  render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
  const render::FrameRenderCamera& frame = render::core::cameraFrame();
  const float yawCos = frame.viewRightX;
  const float yawSin = frame.viewRightZ;
  const float widthOffset = frame.viewUpX;
  const float heightOffset = frame.viewUpZ;
  const float verticalSize = frame.viewUpY;
  render::Tessellator& tessellator = render::Tessellator::INSTANCE;
  for(int group = 0; group < 3; ++group) {
   const char* texture = group == 0 ? "/particles.png" : (group == 1 ? "/terrain.png" : "/gui/items.png");
   int defaultTextureId = -1;
   int boundTextureId = -1;
   for(const auto& particle : particles_[static_cast<std::size_t>(group)]) {
    if(particle->renderSheet() != sheet) {
     continue;
    }
    if(defaultTextureId < 0) {
     defaultTextureId = textureManager_->getTextureId(texture);
     boundTextureId = defaultTextureId;
     textureManager_->bindTexture(boundTextureId);
     tessellator.startQuads();
    }
    if(group == 1) {
     const int particleTextureId = particle->boundTextureGl(*textureManager_);
     const int nextTextureId = particleTextureId >= 0 ? particleTextureId : defaultTextureId;
     if(nextTextureId != boundTextureId) {
      tessellator.draw();
      boundTextureId = nextTextureId;
      textureManager_->bindTexture(boundTextureId);
      tessellator.startQuads();
     }
    }
    particle->applyLight(tessellator);
    particle->render(tessellator, partialTicks, yawCos, verticalSize, yawSin, widthOffset, heightOffset);
   }
   if(defaultTextureId >= 0) {
    tessellator.draw();
   }
  }
  for(const auto& particle : particles_[3]) {
   if(particle->renderSheet() != sheet) {
    continue;
   }
   particle->applyLight(tessellator);
   particle->render(tessellator, partialTicks, yawCos, verticalSize, yawSin, widthOffset, heightOffset);
  }
 }
 void addBlockBreakParticles(int x, int y, int z, int blockId, int blockMeta) {
  if(blockId == 0) {
   return;
  }
  Block* block = Block::BLOCKS[blockId];
  constexpr int grid = 4;
  for(int i = 0; i < grid; ++i) {
   for(int j = 0; j < grid; ++j) {
    for(int k = 0; k < grid; ++k) {
     // jitter within cell so particles don't spawn on a perfect lattice
     const double px = static_cast<double>(x) +
                       (static_cast<double>(i) + random_.nextDouble()) / static_cast<double>(grid);
     const double py = static_cast<double>(y) +
                       (static_cast<double>(j) + random_.nextDouble()) / static_cast<double>(grid);
     const double pz = static_cast<double>(z) +
                       (static_cast<double>(k) + random_.nextDouble()) / static_cast<double>(grid);
     // scale outward velocity so particles burst away from the block
     const double vx = (px - static_cast<double>(x) - 0.5) * 3.0;
     const double vy = (py - static_cast<double>(y) - 0.5) * 3.0;
     const double vz = (pz - static_cast<double>(z) - 0.5) * 3.0;
     auto particle = std::make_unique<BlockParticle>(world_, px, py, pz, vx, vy, vz, block, blockMeta);
     particle->color(x, y, z);
     addParticle(std::move(particle));
    }
   }
  }
 }
 void addBlockBreakingParticles(int x, int y, int z, int side) {
  const int blockId = world_->getBlockId(x, y, z);
  if(blockId == 0) {
   return;
  }
  Block* block = Block::BLOCKS[blockId];
  JavaRandom& random = random_;
  constexpr float inset = 0.1f;
  double px = static_cast<double>(x) +
              random.nextDouble() * (block->maxX - block->minX - static_cast<double>(inset) * 2.0) +
              static_cast<double>(inset) + block->minX;
  double py = static_cast<double>(y) +
              random.nextDouble() * (block->maxY - block->minY - static_cast<double>(inset) * 2.0) +
              static_cast<double>(inset) + block->minY;
  double pz = static_cast<double>(z) +
              random.nextDouble() * (block->maxZ - block->minZ - static_cast<double>(inset) * 2.0) +
              static_cast<double>(inset) + block->minZ;
  if(side == 0) {
   py = static_cast<double>(y) + block->minY - static_cast<double>(inset);
  } else if(side == 1) {
   py = static_cast<double>(y) + block->maxY + static_cast<double>(inset);
  } else if(side == 2) {
   pz = static_cast<double>(z) + block->minZ - static_cast<double>(inset);
  } else if(side == 3) {
   pz = static_cast<double>(z) + block->maxZ + static_cast<double>(inset);
  } else if(side == 4) {
   px = static_cast<double>(x) + block->minX - static_cast<double>(inset);
  } else if(side == 5) {
   px = static_cast<double>(x) + block->maxX + static_cast<double>(inset);
  }
  auto particle = std::make_unique<BlockParticle>(
      world_, px, py, pz, 0.0, 0.0, 0.0, block, world_->getBlockMeta(x, y, z));
  particle->color(x, y, z)->multiplyVelocity(0.2f)->setScale(0.6f);
  addParticle(std::move(particle));
 }
 void setWorld(World* world) {
  world_ = world;
  for(auto& bucket : particles_) {
   bucket.clear();
  }
 }
 void setTextureManager(texture::TextureManager* textureManager) {
  textureManager_ = textureManager;
 }
 [[nodiscard]] std::string toString() const {
  const std::size_t count = particles_[0].size() + particles_[1].size() + particles_[2].size();
  return std::to_string(count);
 }

 private:
  // Anchor particle rendering to the camera EYE so its origin matches the
  // terrain/entity render origin (WorldRenderer::sectionOrigin). Particles emit
  // camera-relative quads with an identity pose; the camera matrix comes from
  // the draw camera state like every other world draw.
  static void syncCameraOffset() {
   if(render::core::drawCameraStateValid()) {
    const double* eye = render::core::drawCameraPosition();
    Particle::xOffset = eye[0];
    Particle::yOffset = eye[1];
    Particle::zOffset = eye[2];
    return;
   }
   const render::FrameRenderCamera& frame = render::core::cameraFrame();
   Particle::xOffset = frame.eyeX;
   Particle::yOffset = frame.eyeY;
   Particle::zOffset = frame.eyeZ;
  }
 World* world_ = nullptr;
 texture::TextureManager* textureManager_ = nullptr;
 std::array<std::vector<std::unique_ptr<Particle>>, 4> particles_{};
 // Java: private Random random = new Random(); kept separate from world.random
 // so spawning block-break particles never perturbs the world RNG stream.
 JavaRandom random_{};
};
} // namespace net::minecraft::client::particle
