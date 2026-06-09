#include "net/minecraft/entity/LightningEntity.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity {

LightningEntity::LightningEntity(World* world) : AbstractLightningEntity(world)
{
    ambientTick = 2;
    seed = random.nextLong();
    remainingActions = random.nextInt(3) + 1;
}

LightningEntity::LightningEntity(World* world, double xIn, double yIn, double zIn) : LightningEntity(world)
{
    setPositionAndAnglesKeepPrevAngles(xIn, yIn, zIn, 0.0f, 0.0f);
    if (world != nullptr && world->difficulty >= 2
        && world->isRegionLoaded(MathHelper::floor(xIn), MathHelper::floor(yIn), MathHelper::floor(zIn), 10)) {
        const int bx = MathHelper::floor(xIn);
        const int by = MathHelper::floor(yIn);
        const int bz = MathHelper::floor(zIn);
        if (Block::FIRE != nullptr && world->getBlockId(bx, by, bz) == 0 && Block::FIRE->canPlaceAt(world, bx, by, bz, 0)) {
            world->setBlock(bx, by, bz, Block::FIRE->id);
        }
        for (int i = 0; i < 4; ++i) {
            const int fx = MathHelper::floor(xIn) + random.nextInt(3) - 1;
            const int fy = MathHelper::floor(yIn) + random.nextInt(3) - 1;
            const int fz = MathHelper::floor(zIn) + random.nextInt(3) - 1;
            if (world->getBlockId(fx, fy, fz) != 0 || Block::FIRE == nullptr || !Block::FIRE->canPlaceAt(world, fx, fy, fz, 0)) {
                continue;
            }
            world->setBlock(fx, fy, fz, Block::FIRE->id);
        }
    }
}

void LightningEntity::tick()
{
    AbstractLightningEntity::tick();
    if (world == nullptr) {
        return;
    }
    if (ambientTick == 2) {
        world->playSound(x, y, z, "ambient.weather.thunder", 10000.0f, 0.8f + random.nextFloat() * 0.2f);
        world->playSound(x, y, z, "random.explode", 2.0f, 0.5f + random.nextFloat() * 0.2f);
    }
    --ambientTick;
    if (ambientTick < 0) {
        if (remainingActions == 0) {
            markDead();
        } else if (ambientTick < -random.nextInt(10)) {
            --remainingActions;
            ambientTick = 1;
            seed = random.nextLong();
            const int bx = MathHelper::floor(x);
            const int by = MathHelper::floor(y);
            const int bz = MathHelper::floor(z);
            if (world->isRegionLoaded(bx, by, bz, 10) && world->getBlockId(bx, by, bz) == 0
                && Block::FIRE != nullptr && Block::FIRE->canPlaceAt(world, bx, by, bz, 0)) {
                world->setBlock(bx, by, bz, Block::FIRE->id);
            }
        }
    }
    if (ambientTick >= 0) {
        constexpr double radius = 3.0;
        const std::vector<Entity*> struck = world->getEntities(
            this,
            Box {x - radius, y - radius, z - radius, x + radius, y + 6.0 + radius, z + radius});
        for (Entity* entity : struck) {
            if (entity != nullptr) {
                entity->onStruckByLightning(this);
            }
        }
        world->lightningTicksLeft = 2;
    }
}

} // namespace net::minecraft::entity
