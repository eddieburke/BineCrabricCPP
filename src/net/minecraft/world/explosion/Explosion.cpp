#include "net/minecraft/world/explosion/Explosion.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft {
Explosion::Explosion(World* world, entity::Entity* source, double xIn, double yIn, double zIn, float powerIn)
    : x(xIn), y(yIn), z(zIn), source(source), power(powerIn), world_(world) {
}

void Explosion::explode() {
    if (world_ == nullptr) {
        return;
    }
    const float savedPower = power;
    constexpr int sampleCount = 16;
    for (int i = 0; i < sampleCount; ++i) {
        for (int j = 0; j < sampleCount; ++j) {
            for (int k = 0; k < sampleCount; ++k) {
                if (i != 0 && i != sampleCount - 1 && j != 0 && j != sampleCount - 1 && k != 0 &&
                    k != sampleCount - 1) {
                    continue;
                }
                double dirX = static_cast<double>(i) / static_cast<double>(sampleCount - 1) * 2.0 - 1.0;
                double dirY = static_cast<double>(j) / static_cast<double>(sampleCount - 1) * 2.0 - 1.0;
                double dirZ = static_cast<double>(k) / static_cast<double>(sampleCount - 1) * 2.0 - 1.0;
                const double dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
                dirX /= dirLen;
                dirY /= dirLen;
                dirZ /= dirLen;
                double traceX = x;
                double traceY = y;
                double traceZ = z;
                constexpr float step = 0.3f;
                float intensity = power * (0.7f + world_->random().nextFloat() * 0.6f);
                while (intensity > 0.0f) {
                    const int bx = MathHelper::floor(traceX);
                    const int by = MathHelper::floor(traceY);
                    const int bz = MathHelper::floor(traceZ);
                    const int blockId = world_->getBlockId(bx, by, bz);
                    if (blockId > 0 && blockId < block::Block::BLOCK_COUNT) {
                        block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
                        if (block != nullptr) {
                            intensity -= (block->getBlastResistance(source) + 0.3f) * step;
                        }
                    }
                    if (intensity > 0.0f) {
                        damagedBlocks.insert(Vec3i{bx, by, bz});
                    }
                    traceX += dirX * static_cast<double>(step);
                    traceY += dirY * static_cast<double>(step);
                    traceZ += dirZ * static_cast<double>(step);
                    intensity -= step * 0.75f;
                }
            }
        }
    }
    power *= 2.0f;
    const int minX = MathHelper::floor(x - static_cast<double>(power) - 1.0);
    const int maxX = MathHelper::floor(x + static_cast<double>(power) + 1.0);
    const int minY = MathHelper::floor(y - static_cast<double>(power) - 1.0);
    const int maxY = MathHelper::floor(y + static_cast<double>(power) + 1.0);
    const int minZ = MathHelper::floor(z - static_cast<double>(power) - 1.0);
    const int maxZ = MathHelper::floor(z + static_cast<double>(power) + 1.0);
    const Vec3d origin{x, y, z};
    const std::vector<Entity*> entities = world_->getEntities(source,
                                                              Box{static_cast<double>(minX),
                                                                  static_cast<double>(minY),
                                                                  static_cast<double>(minZ),
                                                                  static_cast<double>(maxX),
                                                                  static_cast<double>(maxY),
                                                                  static_cast<double>(maxZ)});
    for (entity::Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }
        const double distanceRatio = entity->getDistance(x, y, z) / static_cast<double>(power);
        if (distanceRatio > 1.0) {
            continue;
        }
        double offsetX = entity->x - x;
        double offsetY = entity->y - y;
        double offsetZ = entity->z - z;
        const double offsetLen = MathHelper::sqrt(offsetX * offsetX + offsetY * offsetY + offsetZ * offsetZ);
        offsetX /= offsetLen;
        offsetY /= offsetLen;
        offsetZ /= offsetLen;
        const double visibility = world_->getVisibilityRatio(origin, entity->boundingBox);
        const double blastExposure = (1.0 - distanceRatio) * visibility;
        entity->damage(
            source,
            static_cast<int>((blastExposure * blastExposure + blastExposure) / 2.0 * 8.0 * static_cast<double>(power) +
                             1.0));
        entity->velocityX += offsetX * blastExposure;
        entity->velocityY += offsetY * blastExposure;
        entity->velocityZ += offsetZ * blastExposure;
    }
    power = savedPower;
    if (fire) {
        std::vector<Vec3i> blocks(damagedBlocks.begin(), damagedBlocks.end());
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            const int bx = it->x;
            const int by = it->y;
            const int bz = it->z;
            const int blockId = world_->getBlockId(bx, by, bz);
            const int belowId = world_->getBlockId(bx, by - 1, bz);
            if (blockId != 0 || belowId == 0 || !block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(belowId)]) {
                continue;
            }
            if (random_.nextInt(3) != 0) {
                continue;
            }
            if (block::Block::FIRE != nullptr) {
                world_->setBlock(bx, by, bz, block::Block::FIRE->id);
            }
        }
    }
}

void Explosion::playExplosionSound(bool addParticles) {
    if (world_ == nullptr) {
        return;
    }
    world_->playSound(x,
                      y,
                      z,
                      "random.explode",
                      4.0f,
                      (1.0f + (world_->random().nextFloat() - world_->random().nextFloat()) * 0.2f) * 0.7f);
    std::vector<Vec3i> blocks(damagedBlocks.begin(), damagedBlocks.end());
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        const int bx = it->x;
        const int by = it->y;
        const int bz = it->z;
        const int blockId = world_->getBlockId(bx, by, bz);
        if (addParticles) {
            double px = static_cast<double>(bx) + world_->random().nextFloat();
            double py = static_cast<double>(by) + world_->random().nextFloat();
            double pz = static_cast<double>(bz) + world_->random().nextFloat();
            double dirX = px - x;
            double dirY = py - y;
            double dirZ = pz - z;
            const double dirLen = MathHelper::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
            if (dirLen > 0.0) {
                dirX /= dirLen;
                dirY /= dirLen;
                dirZ /= dirLen;
            }
            double speed = 0.5 / (dirLen / static_cast<double>(power) + 0.1);
            speed *= static_cast<double>(world_->random().nextFloat() * world_->random().nextFloat() + 0.3f);
            world_->addParticle(
                "explode", (px + x) / 2.0, (py + y) / 2.0, (pz + z) / 2.0, dirX * speed, dirY * speed, dirZ * speed);
            world_->addParticle("smoke", px, py, pz, dirX * speed, dirY * speed, dirZ * speed);
        }
        if (blockId <= 0 || blockId >= block::Block::BLOCK_COUNT) {
            continue;
        }
        block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            continue;
        }
        block->dropStacks(world_, bx, by, bz, world_->getBlockMeta(bx, by, bz), 0.3f);
        world_->setBlock(bx, by, bz, 0);
        block->onDestroyedByExplosion(world_, bx, by, bz);
    }
}
}  // namespace net::minecraft
