#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>
#include <memory>
#include <optional>

namespace net::minecraft::entity::projectile {
namespace {

[[nodiscard]] Entity* findEntityOnPath(Entity& projectile, LivingEntity* owner, int inAirTime, const Vec3d& start,
    const Vec3d& end)
{
    if (projectile.world == nullptr) {
        return nullptr;
    }
    const Box searchBox = projectile.boundingBox.stretch(
        end.x - start.x, end.y - start.y, end.z - start.z).expand(1.0);
    const std::vector<Entity*> candidates = projectile.world->getEntities(&projectile, searchBox);
    Entity* closest = nullptr;
    double closestDist = -1.0;
    for (Entity* candidate : candidates) {
        if (candidate == nullptr || !candidate->isCollidable()) {
            continue;
        }
        if (candidate == owner && inAirTime < 5) {
            continue;
        }
        const Box hitBox = candidate->boundingBox.expand(0.3);
        const std::optional<HitResult> entityHit = Block::raycastLocalBounds(
            hitBox.minX,
            hitBox.minY,
            hitBox.minZ,
            hitBox.maxX,
            hitBox.maxY,
            hitBox.maxZ,
            0,
            0,
            0,
            start,
            end);
        if (!entityHit.has_value()) {
            continue;
        }
        const double dx = entityHit->pos.x - start.x;
        const double dy = entityHit->pos.y - start.y;
        const double dz = entityHit->pos.z - start.z;
        const double dist = dx * dx + dy * dy + dz * dz;
        if (closest == nullptr || dist < closestDist) {
            closest = candidate;
            closestDist = dist;
        }
    }
    return closest;
}

void updateProjectileRotation(Entity& entity)
{
    const float horizontal = MathHelper::sqrt(
        static_cast<float>(entity.velocityX * entity.velocityX + entity.velocityZ * entity.velocityZ));
    entity.yaw = static_cast<float>(std::atan2(entity.velocityX, entity.velocityZ) * 180.0 / 3.141592653589793);
    entity.pitch = static_cast<float>(std::atan2(entity.velocityY, static_cast<double>(horizontal)) * 180.0 / 3.141592653589793);
    while (entity.pitch - entity.prevPitch < -180.0f) {
        entity.prevPitch -= 360.0f;
    }
    while (entity.pitch - entity.prevPitch >= 180.0f) {
        entity.prevPitch += 360.0f;
    }
    while (entity.yaw - entity.prevYaw < -180.0f) {
        entity.prevYaw -= 360.0f;
    }
    while (entity.yaw - entity.prevYaw >= 180.0f) {
        entity.prevYaw += 360.0f;
    }
    entity.pitch = entity.prevPitch + (entity.pitch - entity.prevPitch) * 0.2f;
    entity.yaw = entity.prevYaw + (entity.yaw - entity.prevYaw) * 0.2f;
}

void applyProjectileDrag(Entity& entity)
{
    float drag = 0.99f;
    constexpr float gravity = 0.03f;
    if (entity.isSubmergedInWater()) {
        if (entity.world != nullptr) {
            for (int i = 0; i < 4; ++i) {
                constexpr float offset = 0.25f;
                entity.world->addParticle(
                    "bubble",
                    entity.x - entity.velocityX * static_cast<double>(offset),
                    entity.y - entity.velocityY * static_cast<double>(offset),
                    entity.z - entity.velocityZ * static_cast<double>(offset),
                    entity.velocityX,
                    entity.velocityY,
                    entity.velocityZ);
            }
        }
        drag = 0.8f;
    }
    entity.velocityX *= static_cast<double>(drag);
    entity.velocityY *= static_cast<double>(drag);
    entity.velocityZ *= static_cast<double>(drag);
    entity.velocityY -= static_cast<double>(gravity);
}

} // namespace

void tickThrownProjectile(Entity& projectile, LivingEntity* owner, int& inAirTime, int& blockX, int& blockY, int& blockZ,
    int& blockId, bool& inGround, int& removalTimer, int& shake, bool spawnChickens)
{
    if (projectile.world == nullptr) {
        return;
    }

    projectile.lastTickX = projectile.x;
    projectile.lastTickY = projectile.y;
    projectile.lastTickZ = projectile.z;

    if (shake > 0) {
        --shake;
    }

    if (inGround) {
        const int currentBlockId = projectile.world->getBlockId(blockX, blockY, blockZ);
        if (currentBlockId != blockId) {
            inGround = false;
            projectile.velocityX *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            projectile.velocityY *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            projectile.velocityZ *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            removalTimer = 0;
            inAirTime = 0;
        } else {
            if (++removalTimer == 1200) {
                projectile.markDead();
            }
            return;
        }
    }

    ++inAirTime;

    const Vec3d start {projectile.x, projectile.y, projectile.z};
    const Vec3d end {
        projectile.x + projectile.velocityX,
        projectile.y + projectile.velocityY,
        projectile.z + projectile.velocityZ};

    std::optional<HitResult> hit = projectile.world->raycast(start, end);
    Vec3d hitPos = end;
    if (hit.has_value()) {
        hitPos = hit->pos;
    }

    if (!projectile.world->isRemote()) {
        if (Entity* entityHit = findEntityOnPath(projectile, owner, inAirTime, start, hitPos); entityHit != nullptr) {
            hit = HitResult(entityHit, Vec3d {entityHit->x, entityHit->y, entityHit->z});
        }
    }

    if (hit.has_value()) {
        if (hit->entity != nullptr) {
            hit->entity->damage(owner, 0);
        }
        if (!projectile.world->isRemote() && spawnChickens && projectile.random.nextInt(8) == 0) {
            int chickenCount = 1;
            if (projectile.random.nextInt(32) == 0) {
                chickenCount = 4;
            }
            for (int i = 0; i < chickenCount; ++i) {
                auto chicken = std::make_unique<net::minecraft::entity::passive::ChickenEntity>(projectile.world);
                chicken->setPositionAndAnglesKeepPrevAngles(projectile.x, projectile.y, projectile.z, projectile.yaw, 0.0f);
                if (projectile.world->spawnMob(chicken.get())) {
                    chicken.release();
                }
            }
        }
        for (int i = 0; i < 8; ++i) {
            projectile.world->addParticle("snowballpoof", projectile.x, projectile.y, projectile.z, 0.0, 0.0, 0.0);
        }
        projectile.markDead();
        return;
    }

    projectile.x += projectile.velocityX;
    projectile.y += projectile.velocityY;
    projectile.z += projectile.velocityZ;
    updateProjectileRotation(projectile);
    applyProjectileDrag(projectile);
    projectile.setPosition(projectile.x, projectile.y, projectile.z);
}

void tickArrowProjectile(Entity& projectile, LivingEntity*& owner, int& inAirTime, int& blockX, int& blockY, int& blockZ,
    int& blockId, int& blockMeta, bool& inGround, int& life, int& shake, bool pickupAllowed)
{
    if (projectile.world == nullptr) {
        return;
    }

    if (projectile.prevPitch == 0.0f && projectile.prevYaw == 0.0f) {
        const float horizontal = MathHelper::sqrt(
            static_cast<float>(projectile.velocityX * projectile.velocityX + projectile.velocityZ * projectile.velocityZ));
        projectile.prevYaw = projectile.yaw =
            static_cast<float>(std::atan2(projectile.velocityX, projectile.velocityZ) * 180.0 / 3.141592653589793);
        projectile.prevPitch = projectile.pitch =
            static_cast<float>(std::atan2(projectile.velocityY, static_cast<double>(horizontal)) * 180.0 / 3.141592653589793);
    }

    const int embeddedBlockId = projectile.world->getBlockId(blockX, blockY, blockZ);
    if (embeddedBlockId > 0 && embeddedBlockId < Block::BLOCK_COUNT
        && Block::BLOCKS[static_cast<std::size_t>(embeddedBlockId)] != nullptr) {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(embeddedBlockId)];
        block->updateBoundingBox(projectile.world, blockX, blockY, blockZ);
        const std::optional<Box> shape = block->getCollisionShape(projectile.world, blockX, blockY, blockZ);
        if (shape.has_value()
            && projectile.x >= shape->minX && projectile.x <= shape->maxX
            && projectile.y >= shape->minY && projectile.y <= shape->maxY
            && projectile.z >= shape->minZ && projectile.z <= shape->maxZ) {
            inGround = true;
        }
    }

    if (shake > 0) {
        --shake;
    }

    if (inGround) {
        const int currentBlockId = projectile.world->getBlockId(blockX, blockY, blockZ);
        const int currentMeta = projectile.world->getBlockMeta(blockX, blockY, blockZ);
        if (currentBlockId != blockId || currentMeta != blockMeta) {
            inGround = false;
            projectile.velocityX *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            projectile.velocityY *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            projectile.velocityZ *= static_cast<double>(projectile.random.nextFloat() * 0.2f);
            life = 0;
            inAirTime = 0;
            return;
        }
        ++life;
        if (life == 1200) {
            projectile.markDead();
        }
        return;
    }

    ++inAirTime;

    const Vec3d start {projectile.x, projectile.y, projectile.z};
    const Vec3d end {
        projectile.x + projectile.velocityX,
        projectile.y + projectile.velocityY,
        projectile.z + projectile.velocityZ};

    std::optional<HitResult> hit = projectile.world->raycast(start, end, false, true);
    Vec3d hitPos = end;
    if (hit.has_value()) {
        hitPos = hit->pos;
    }

    if (!projectile.world->isRemote()) {
        if (Entity* entityHit = findEntityOnPath(projectile, owner, inAirTime, start, hitPos); entityHit != nullptr) {
            hit = HitResult(entityHit, Vec3d {entityHit->x, entityHit->y, entityHit->z});
        }
    }

    if (hit.has_value()) {
        if (hit->entity != nullptr) {
            if (hit->entity->damage(owner, 4)) {
                projectile.world->playSound(
                    &projectile,
                    "random.drr",
                    1.0f,
                    1.2f / (projectile.random.nextFloat() * 0.2f + 0.9f));
                projectile.markDead();
            } else {
                projectile.velocityX *= -0.1;
                projectile.velocityY *= -0.1;
                projectile.velocityZ *= -0.1;
                projectile.yaw += 180.0f;
                projectile.prevYaw += 180.0f;
                inAirTime = 0;
            }
        } else {
            blockX = hit->blockX;
            blockY = hit->blockY;
            blockZ = hit->blockZ;
            blockId = projectile.world->getBlockId(blockX, blockY, blockZ);
            blockMeta = projectile.world->getBlockMeta(blockX, blockY, blockZ);
            projectile.velocityX = hitPos.x - projectile.x;
            projectile.velocityY = hitPos.y - projectile.y;
            projectile.velocityZ = hitPos.z - projectile.z;
            const float length = MathHelper::sqrt(static_cast<float>(
                projectile.velocityX * projectile.velocityX
                + projectile.velocityY * projectile.velocityY
                + projectile.velocityZ * projectile.velocityZ));
            if (length > 1.0e-4f) {
                projectile.x -= projectile.velocityX / static_cast<double>(length) * 0.05;
                projectile.y -= projectile.velocityY / static_cast<double>(length) * 0.05;
                projectile.z -= projectile.velocityZ / static_cast<double>(length) * 0.05;
            }
            projectile.world->playSound(
                &projectile,
                "random.drr",
                1.0f,
                1.2f / (projectile.random.nextFloat() * 0.2f + 0.9f));
            inGround = true;
            shake = 7;
        }
        if (projectile.dead) {
            return;
        }
    }

    projectile.x += projectile.velocityX;
    projectile.y += projectile.velocityY;
    projectile.z += projectile.velocityZ;
    updateProjectileRotation(projectile);
    applyProjectileDrag(projectile);
    projectile.setPosition(projectile.x, projectile.y, projectile.z);
    (void)pickupAllowed;
}

} // namespace net::minecraft::entity::projectile
