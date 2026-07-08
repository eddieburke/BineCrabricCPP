#include "net/minecraft/world/dimension/PortalForcer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft {
void PortalForcer::moveToPortal(World* world, Entity* entity) {
    if (world == nullptr || entity == nullptr) {
        return;
    }
    if (teleportToValidPortal(world, entity)) {
        return;
    }
    createPortal(world, entity);
    teleportToValidPortal(world, entity);
}

bool PortalForcer::teleportToValidPortal(World* world, Entity* entity) {
    if (world == nullptr || entity == nullptr || Block::NETHER_PORTAL == nullptr) {
        return false;
    }
    constexpr int searchRadius = 128;
    double bestDistance = -1.0;
    int bestX = 0;
    int bestY = 0;
    int bestZ = 0;
    const int entityX = MathHelper::floor(entity->x);
    const int entityZ = MathHelper::floor(entity->z);
    for (int x = entityX - searchRadius; x <= entityX + searchRadius; ++x) {
        const double dx = static_cast<double>(x) + 0.5 - entity->x;
        for (int z = entityZ - searchRadius; z <= entityZ + searchRadius; ++z) {
            const double dz = static_cast<double>(z) + 0.5 - entity->z;
            for (int y = 127; y >= 0; --y) {
                if (world->getBlockId(x, y, z) != Block::NETHER_PORTAL->id) {
                    continue;
                }
                int portalY = y;
                while (portalY > 0 && world->getBlockId(x, portalY - 1, z) == Block::NETHER_PORTAL->id) {
                    --portalY;
                }
                const double dy = static_cast<double>(portalY) + 0.5 - entity->y;
                const double distance = dx * dx + dy * dy + dz * dz;
                if (bestDistance >= 0.0 && distance >= bestDistance) {
                    continue;
                }
                bestDistance = distance;
                bestX = x;
                bestY = portalY;
                bestZ = z;
            }
        }
    }
    if (bestDistance < 0.0) {
        return false;
    }
    double targetX = static_cast<double>(bestX) + 0.5;
    double targetY = static_cast<double>(bestY) + 0.5;
    double targetZ = static_cast<double>(bestZ) + 0.5;
    if (world->getBlockId(bestX - 1, bestY, bestZ) == Block::NETHER_PORTAL->id) {
        targetX -= 0.5;
    }
    if (world->getBlockId(bestX + 1, bestY, bestZ) == Block::NETHER_PORTAL->id) {
        targetX += 0.5;
    }
    if (world->getBlockId(bestX, bestY, bestZ - 1) == Block::NETHER_PORTAL->id) {
        targetZ -= 0.5;
    }
    if (world->getBlockId(bestX, bestY, bestZ + 1) == Block::NETHER_PORTAL->id) {
        targetZ += 0.5;
    }
    entity->teleport(targetX, targetY, targetZ, entity->yaw, 0.0f);
    entity->velocityX = 0.0;
    entity->velocityY = 0.0;
    entity->velocityZ = 0.0;
    return true;
}

bool PortalForcer::createPortal(World* world, Entity* entity) {
    if (world == nullptr || entity == nullptr || Block::NETHER_PORTAL == nullptr || Block::OBSIDIAN == nullptr) {
        return false;
    }
    constexpr int searchRadius = 16;
    double bestDistance = -1.0;
    int bestX = MathHelper::floor(entity->x);
    int bestY = MathHelper::floor(entity->y);
    int bestZ = MathHelper::floor(entity->z);
    const int randomRotation = random_.nextInt(4);
    int bestRotation = 0;
    const int entityX = MathHelper::floor(entity->x);
    const int entityZ = MathHelper::floor(entity->z);
    for (int x = entityX - searchRadius; x <= entityX + searchRadius; ++x) {
        const double dx = static_cast<double>(x) + 0.5 - entity->x;
        for (int z = entityZ - searchRadius; z <= entityZ + searchRadius; ++z) {
            const double dz = static_cast<double>(z) + 0.5 - entity->z;
            for (int y = 127; y >= 0; --y) {
                if (!world->isAir(x, y, z)) {
                    continue;
                }
                int baseY = y;
                while (baseY > 0 && world->isAir(x, baseY - 1, z)) {
                    --baseY;
                }
                for (int rotation = randomRotation; rotation < randomRotation + 4; ++rotation) {
                    int axisA = rotation % 2;
                    int axisB = 1 - axisA;
                    if (rotation % 4 >= 2) {
                        axisA = -axisA;
                        axisB = -axisB;
                    }
                    bool valid = true;
                    for (int depth = 0; depth < 3 && valid; ++depth) {
                        for (int width = 0; width < 4; ++width) {
                            for (int height = -1; height < 4; ++height) {
                                const int px = x + (width - 1) * axisA + depth * axisB;
                                const int py = baseY + height;
                                const int pz = z + (width - 1) * axisB - depth * axisA;
                                if (height < 0) {
                                    if (!world->getMaterial(px, py, pz).isSolid()) {
                                        valid = false;
                                        break;
                                    }
                                } else if (!world->isAir(px, py, pz)) {
                                    valid = false;
                                    break;
                                }
                            }
                            if (!valid) {
                                break;
                            }
                        }
                    }
                    if (!valid) {
                        continue;
                    }
                    const double dy = static_cast<double>(baseY) + 0.5 - entity->y;
                    const double distance = dx * dx + dy * dy + dz * dz;
                    if (bestDistance >= 0.0 && distance >= bestDistance) {
                        continue;
                    }
                    bestDistance = distance;
                    bestX = x;
                    bestY = baseY;
                    bestZ = z;
                    bestRotation = rotation % 4;
                }
            }
        }
    }
    if (bestDistance < 0.0) {
        for (int x = entityX - searchRadius; x <= entityX + searchRadius; ++x) {
            const double dx = static_cast<double>(x) + 0.5 - entity->x;
            for (int z = entityZ - searchRadius; z <= entityZ + searchRadius; ++z) {
                const double dz = static_cast<double>(z) + 0.5 - entity->z;
                for (int y = 127; y >= 0; --y) {
                    if (!world->isAir(x, y, z)) {
                        continue;
                    }
                    int baseY = y;
                    while (baseY > 0 && world->isAir(x, baseY - 1, z)) {
                        --baseY;
                    }
                    for (int rotation = randomRotation; rotation < randomRotation + 2; ++rotation) {
                        int axisA = rotation % 2;
                        int axisB = 1 - axisA;
                        bool valid = true;
                        for (int width = 0; width < 4; ++width) {
                            for (int height = -1; height < 4; ++height) {
                                const int px = x + (width - 1) * axisA;
                                const int py = baseY + height;
                                const int pz = z + (width - 1) * axisB;
                                if (height < 0) {
                                    if (!world->getMaterial(px, py, pz).isSolid()) {
                                        valid = false;
                                        break;
                                    }
                                } else if (!world->isAir(px, py, pz)) {
                                    valid = false;
                                    break;
                                }
                            }
                            if (!valid) {
                                break;
                            }
                        }
                        if (!valid) {
                            continue;
                        }
                        const double dy = static_cast<double>(baseY) + 0.5 - entity->y;
                        const double distance = dx * dx + dy * dy + dz * dz;
                        if (bestDistance >= 0.0 && distance >= bestDistance) {
                            continue;
                        }
                        bestDistance = distance;
                        bestX = x;
                        bestY = baseY;
                        bestZ = z;
                        bestRotation = rotation % 2;
                    }
                }
            }
        }
    }
    const int rotation = bestRotation;
    int portalX = bestX;
    int portalY = bestY;
    int portalZ = bestZ;
    int axisA = rotation % 2;
    int axisB = 1 - axisA;
    if (rotation % 4 >= 2) {
        axisA = -axisA;
        axisB = -axisB;
    }
    if (bestDistance < 0.0) {
        if (portalY < 70) {
            portalY = 70;
        }
        if (portalY > 118) {
            portalY = 118;
        }
        bestY = portalY;
        for (int offset = -1; offset <= 1; ++offset) {
            for (int depth = 1; depth < 3; ++depth) {
                for (int height = -1; height < 3; ++height) {
                    const int px = portalX + (depth - 1) * axisA + offset * axisB;
                    const int py = bestY + height;
                    const int pz = portalZ + (depth - 1) * axisB - offset * axisA;
                    const int blockId = height < 0 ? Block::OBSIDIAN->id : 0;
                    world->setBlock(px, py, pz, blockId);
                }
            }
        }
    }
    for (int attempt = 0; attempt < 4; ++attempt) {
        world->pauseTicking = true;
        for (int width = 0; width < 4; ++width) {
            for (int height = -1; height < 4; ++height) {
                const int px = portalX + (width - 1) * axisA;
                const int py = bestY + height;
                const int pz = portalZ + (width - 1) * axisB;
                const bool frame = width == 0 || width == 3 || height == -1 || height == 3;
                world->setBlock(px, py, pz, frame ? Block::OBSIDIAN->id : Block::NETHER_PORTAL->id);
            }
        }
        world->pauseTicking = false;
        for (int width = 0; width < 4; ++width) {
            for (int height = -1; height < 4; ++height) {
                const int px = portalX + (width - 1) * axisA;
                const int py = bestY + height;
                const int pz = portalZ + (width - 1) * axisB;
                world->notifyNeighbors(px, py, pz, world->getBlockId(px, py, pz));
            }
        }
    }
    return true;
}
}  // namespace net::minecraft
