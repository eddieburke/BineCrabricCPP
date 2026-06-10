#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FlowingLiquidBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

namespace net::minecraft::block {

void FlowingLiquidBlock::convertToSource(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    world->setBlockWithoutNotifyingNeighbors(x, y, z, id + 1, meta);
    world->setBlocksDirty(x, y, z, x, y, z);
    world->blockUpdateEvent(x, y, z);
}

void FlowingLiquidBlock::spreadTo(World* world, int x, int y, int z, int depth)
{
    if (world == nullptr || !canSpreadTo(world, x, y, z)) {
        return;
    }
    const int existingId = world->getBlockId(x, y, z);
    if (existingId > 0) {
        if (&material == &material::Material::LAVA) {
            fizz(world, x, y, z);
        } else if (existingId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(existingId)] != nullptr) {
            Block::BLOCKS[static_cast<std::size_t>(existingId)]->dropStacks(
                world, x, y, z, world->getBlockMeta(x, y, z));
        }
    }
    world->setBlock(x, y, z, id, static_cast<std::uint8_t>(depth));
}

int FlowingLiquidBlock::getDistanceToGap(World* world, int x, int y, int z, int distance, int fromDirection)
{
    int best = 1000;
    for (int direction = 0; direction < 4; ++direction) {
        if ((direction == 0 && fromDirection == 1) || (direction == 1 && fromDirection == 0)
            || (direction == 2 && fromDirection == 3) || (direction == 3 && fromDirection == 2)) {
            continue;
        }
        int nx = x;
        int ny = y;
        int nz = z;
        if (direction == 0) {
            --nx;
        } else if (direction == 1) {
            ++nx;
        } else if (direction == 2) {
            --nz;
        } else {
            ++nz;
        }
        if (isLiquidBreaking(world, nx, ny, nz)
            || (&world->getMaterial(nx, ny, nz) == &material && world->getBlockMeta(nx, ny, nz) == 0)) {
            continue;
        }
        if (!isLiquidBreaking(world, nx, ny - 1, nz)) {
            return distance;
        }
        if (distance >= 4) {
            continue;
        }
        const int nested = getDistanceToGap(world, nx, ny, nz, distance + 1, direction);
        if (nested >= best) {
            continue;
        }
        best = nested;
    }
    return best;
}

bool* FlowingLiquidBlock::getSpread(World* world, int x, int y, int z)
{
    for (int direction = 0; direction < 4; ++direction) {
        distanceToGap[direction] = 1000;
        int nx = x;
        int ny = y;
        int nz = z;
        if (direction == 0) {
            --nx;
        } else if (direction == 1) {
            ++nx;
        } else if (direction == 2) {
            --nz;
        } else {
            ++nz;
        }
        if (isLiquidBreaking(world, nx, ny, nz)
            || (&world->getMaterial(nx, ny, nz) == &material && world->getBlockMeta(nx, ny, nz) == 0)) {
            continue;
        }
        distanceToGap[direction] = !isLiquidBreaking(world, nx, ny - 1, nz)
            ? 0
            : getDistanceToGap(world, nx, ny, nz, 1, direction);
    }
    int best = distanceToGap[0];
    for (int direction = 1; direction < 4; ++direction) {
        if (distanceToGap[direction] < best) {
            best = distanceToGap[direction];
        }
    }
    for (int direction = 0; direction < 4; ++direction) {
        spread[direction] = distanceToGap[direction] == best;
    }
    return spread;
}

bool FlowingLiquidBlock::isLiquidBreaking(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    if (Block::DOOR != nullptr && blockId == Block::DOOR->id) {
        return true;
    }
    if (Block::IRON_DOOR != nullptr && blockId == Block::IRON_DOOR->id) {
        return true;
    }
    if (Block::SIGN != nullptr && blockId == Block::SIGN->id) {
        return true;
    }
    if (Block::LADDER != nullptr && blockId == Block::LADDER->id) {
        return true;
    }
    if (Block::SUGAR_CANE != nullptr && blockId == Block::SUGAR_CANE->id) {
        return true;
    }
    if (blockId == 0) {
        return false;
    }
    if (blockId >= Block::BLOCK_COUNT || Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr) {
        return false;
    }
    return Block::BLOCKS[static_cast<std::size_t>(blockId)]->material.blocksMovement();
}

int FlowingLiquidBlock::getLowestDepth(World* world, int x, int y, int z, int depth)
{
    const int state = getLiquidState(world, x, y, z);
    if (state < 0) {
        return depth;
    }
    if (state == 0) {
        ++adjacentSources;
    }
    int normalized = state >= 8 ? 0 : state;
    return depth < 0 || normalized < depth ? normalized : depth;
}

bool FlowingLiquidBlock::canSpreadTo(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const material::Material& target = world->getMaterial(x, y, z);
    if (&target == &material) {
        return false;
    }
    if (&target == &material::Material::LAVA) {
        return false;
    }
    return !isLiquidBreaking(world, x, y, z);
}

void FlowingLiquidBlock::onPlaced(World* world, int x, int y, int z)
{
    LiquidBlock::onPlaced(world, x, y, z);
    if (world != nullptr && world->getBlockId(x, y, z) == id) {
        world->scheduleBlockUpdate(x, y, z, id, getTickRate());
    }
}

void FlowingLiquidBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr) {
        return;
    }
    int currentState = getLiquidState(world, x, y, z);
    int flowRate = 1;
    if (&material == &material::Material::LAVA && world->dimension != nullptr && !world->dimension->evaporatesWater) {
        flowRate = 2;
    }
    bool shouldConvert = true;
    if (currentState > 0) {
        int lowestDepth = -100;
        adjacentSources = 0;
        lowestDepth = getLowestDepth(world, x - 1, y, z, lowestDepth);
        lowestDepth = getLowestDepth(world, x + 1, y, z, lowestDepth);
        lowestDepth = getLowestDepth(world, x, y, z - 1, lowestDepth);
        lowestDepth = getLowestDepth(world, x, y, z + 1, lowestDepth);
        int nextState = lowestDepth + flowRate;
        if (nextState >= 8 || lowestDepth < 0) {
            nextState = -1;
        }
        const int aboveState = getLiquidState(world, x, y + 1, z);
        if (aboveState >= 0) {
            nextState = aboveState >= 8 ? aboveState : aboveState + 8;
        }
        if (adjacentSources >= 2 && &material == &material::Material::WATER) {
            if (world->getMaterial(x, y - 1, z).isSolid()) {
                nextState = 0;
            } else if (&world->getMaterial(x, y - 1, z) == &material && world->getBlockMeta(x, y, z) == 0) {
                nextState = 0;
            }
        }
        if (&material == &material::Material::LAVA && currentState < 8 && nextState < 8 && nextState > currentState
            && random.nextInt(4) != 0) {
            nextState = currentState;
            shouldConvert = false;
        }
        if (nextState != currentState) {
            currentState = nextState;
            if (currentState < 0) {
                world->setBlock(x, y, z, 0);
            } else {
                world->setBlockMeta(x, y, z, currentState);
                world->scheduleBlockUpdate(x, y, z, id, getTickRate());
                world->notifyNeighbors(x, y, z, id);
            }
        } else if (shouldConvert) {
            convertToSource(world, x, y, z);
        }
    } else {
        convertToSource(world, x, y, z);
    }

    if (canSpreadTo(world, x, y - 1, z)) {
        if (currentState >= 8) {
            world->setBlock(x, y - 1, z, id, static_cast<std::uint8_t>(currentState));
        } else {
            world->setBlock(x, y - 1, z, id, static_cast<std::uint8_t>(currentState + 8));
        }
        return;
    }
    if (currentState < 0 || (currentState != 0 && !isLiquidBreaking(world, x, y - 1, z))) {
        return;
    }
    const bool* spreadDirs = getSpread(world, x, y, z);
    int spreadDepth = currentState + flowRate;
    if (currentState >= 8) {
        spreadDepth = 1;
    }
    if (spreadDepth >= 8) {
        return;
    }
    if (spreadDirs[0]) {
        spreadTo(world, x - 1, y, z, spreadDepth);
    }
    if (spreadDirs[1]) {
        spreadTo(world, x + 1, y, z, spreadDepth);
    }
    if (spreadDirs[2]) {
        spreadTo(world, x, y, z - 1, spreadDepth);
    }
    if (spreadDirs[3]) {
        spreadTo(world, x, y, z + 1, spreadDepth);
    }
}
namespace {

void FlowingLiquidBlock::registerClass()
{
    namespace mat = material;
    Block::FLOWING_WATER = (new FlowingLiquidBlock(8, mat::Material::WATER))->setHardness(100.0f)->setOpacity(3)->setTranslationKey("water")->disableTrackingStatistics()->ignoreMetaUpdates();
    Block::FLOWING_LAVA = (new FlowingLiquidBlock(10, mat::Material::LAVA))->setHardness(0.0f)->setLuminance(1.0f)->setOpacity(255)->setTranslationKey("lava")->disableTrackingStatistics()->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<FlowingLiquidBlock> autoReg(8);
} // namespace
} // namespace net::minecraft::block

