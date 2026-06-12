#include "net/minecraft/block/LiquidBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FlowingLiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <cmath>

namespace net::minecraft::block {

namespace {

void vecAdd(net::minecraft::Vec3d& v, double ax, double ay, double az)
{
    v.x += ax;
    v.y += ay;
    v.z += az;
}

void vecNormalize(net::minecraft::Vec3d& v)
{
    const double len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1.0e-8) {
        v.x /= len;
        v.y /= len;
        v.z /= len;
    }
}

int getLiquidDepth(const LiquidBlock& self, const BlockView* blockView, int x, int y, int z)
{
    if (blockView == nullptr) {
        return -1;
    }
    if (&blockView->getMaterial(x, y, z) != &self.material) {
        return -1;
    }
    int meta = blockView->getBlockMeta(x, y, z);
    if (meta >= 8) {
        meta = 0;
    }
    return meta;
}

bool isSolidFace(const LiquidBlock& self, const BlockView* blockView, int x, int y, int z, int face)
{
    if (blockView == nullptr) {
        return false;
    }
    material::Material& material = blockView->getMaterial(x, y, z);
    if (&material == &self.material) {
        return false;
    }
    if (&material == &material::Material::ICE) {
        return false;
    }
    if (face == 1) {
        return true;
    }
    return material.isSolid();
}

net::minecraft::Vec3d getFlow(const LiquidBlock& self, const BlockView* blockView, int x, int y, int z)
{
    net::minecraft::Vec3d flowVector {};
    const int centerDepth = getLiquidDepth(self, blockView, x, y, z);
    for (int side = 0; side < 4; ++side) {
        int nx = x;
        int ny = y;
        int nz = z;
        if (side == 0) {
            --nx;
        }
        if (side == 1) {
            --nz;
        }
        if (side == 2) {
            ++nx;
        }
        if (side == 3) {
            ++nz;
        }
        int neighborDepth = getLiquidDepth(self, blockView, nx, ny, nz);
        if (neighborDepth < 0) {
            if (blockView->getMaterial(nx, ny, nz).blocksMovement()
                || (neighborDepth = getLiquidDepth(self, blockView, nx, ny - 1, nz)) < 0) {
                continue;
            }
            const int depthDelta = neighborDepth - (centerDepth - 8);
            vecAdd(flowVector, (nx - x) * depthDelta, (ny - y) * depthDelta, (nz - z) * depthDelta);
            continue;
        }
        if (neighborDepth < 0) {
            continue;
        }
        const int depthDelta = neighborDepth - centerDepth;
        vecAdd(flowVector, (nx - x) * depthDelta, (ny - y) * depthDelta, (nz - z) * depthDelta);
    }
    if (blockView != nullptr && blockView->getBlockMeta(x, y, z) >= 8) {
        int openSides = 0;
        if (openSides != 0 || isSolidFace(self, blockView, x, y, z - 1, 2)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x, y, z + 1, 3)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x - 1, y, z, 4)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x + 1, y, z, 5)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x, y + 1, z - 1, 2)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x, y + 1, z + 1, 3)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x - 1, y + 1, z, 4)) {
            openSides = 1;
        }
        if (openSides != 0 || isSolidFace(self, blockView, x + 1, y + 1, z, 5)) {
            openSides = 1;
        }
        if (openSides != 0) {
            vecNormalize(flowVector);
            vecAdd(flowVector, 0.0, -6.0, 0.0);
        }
    }
    vecNormalize(flowVector);
    return flowVector;
}

} // namespace

int LiquidBlock::getTexture(int side) const
{
    if (side == 0 || side == 1) {
        return textureId;
    }
    return textureId + 1;
}

bool LiquidBlock::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const
{
    if (blockView == nullptr) {
        return Block::isSideVisible(blockView, x, y, z, side);
    }
    const material::Material& neighborMaterial = blockView->getMaterial(x, y, z);
    if (&neighborMaterial == &material) {
        return false;
    }
    if (&neighborMaterial == &material::Material::ICE) {
        return false;
    }
    if (side == 1) {
        return true;
    }
    return Block::isSideVisible(blockView, x, y, z, side);
}

float LiquidBlock::getLuminance(const BlockView* blockView, int x, int y, int z) const
{
    if (blockView == nullptr) {
        return 1.0f;
    }
    const float below = blockView->getLightBrightness(x, y, z);
    const float above = blockView->getLightBrightness(x, y + 1, z);
    return below > above ? below : above;
}

float LiquidBlock::getFluidHeightFromMeta(int meta)
{
    if (meta >= 8) {
        meta = 0;
    }
    return static_cast<float>(meta + 1) / 9.0f;
}

double LiquidBlock::getFlowingAngle(const BlockView* view, int x, int y, int z, material::Material& material)
{
    const LiquidBlock* liquid = nullptr;
    if (&material == &material::Material::WATER) {
        liquid = dynamic_cast<LiquidBlock*>(Block::BLOCKS[8]);
    } else if (&material == &material::Material::LAVA) {
        liquid = dynamic_cast<LiquidBlock*>(Block::BLOCKS[10]);
    }
    if (liquid == nullptr) {
        return -1000.0;
    }
    const net::minecraft::Vec3d flow = getFlow(*liquid, view, x, y, z);
    if (flow.x == 0.0 && flow.z == 0.0) {
        return -1000.0;
    }
    return std::atan2(flow.z, flow.x) - 1.5707963267948966;
}

void LiquidBlock::applyVelocity(World* world, int x, int y, int z, net::minecraft::Entity* /*entity*/, net::minecraft::Vec3d& velocity)
{
    if (world == nullptr) {
        return;
    }
    const net::minecraft::Vec3d flow = getFlow(*this, static_cast<const BlockView*>(world), x, y, z);
    velocity.x += flow.x;
    velocity.y += flow.y;
    velocity.z += flow.z;
}

int LiquidBlock::getTickRate() const
{
    if (&material == &material::Material::WATER) {
        return 5;
    }
    if (&material == &material::Material::LAVA) {
        return 30;
    }
    return 0;
}

int LiquidBlock::getLiquidState(World* world, int x, int y, int z) const
{
    if (world == nullptr || &world->getMaterial(x, y, z) != &material) {
        return -1;
    }
    return world->getBlockMeta(x, y, z);
}

void LiquidBlock::fizz(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5, "random.fizz", 0.5f,
        2.6f + (world->random().nextFloat() - world->random().nextFloat()) * 0.8f);
    for (int i = 0; i < 8; ++i) {
        world->addParticle("largesmoke", static_cast<double>(x) + world->random().nextDouble(),
            static_cast<double>(y) + 1.2, static_cast<double>(z) + world->random().nextDouble(), 0.0, 0.0, 0.0);
    }
}

void LiquidBlock::checkBlockCollisions(World* world, int x, int y, int z)
{
    if (world == nullptr || world->getBlockId(x, y, z) != id || &material != &material::Material::LAVA) {
        return;
    }
    bool touchesWater = &world->getMaterial(x, y, z - 1) == &material::Material::WATER
        || &world->getMaterial(x, y, z + 1) == &material::Material::WATER
        || &world->getMaterial(x - 1, y, z) == &material::Material::WATER
        || &world->getMaterial(x + 1, y, z) == &material::Material::WATER
        || &world->getMaterial(x, y + 1, z) == &material::Material::WATER;
    if (!touchesWater) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if (meta == 0) {
        if (Block::OBSIDIAN != nullptr) {
            world->setBlock(x, y, z, Block::OBSIDIAN->id);
        }
    } else if (meta <= 4 && Block::COBBLESTONE != nullptr) {
        world->setBlock(x, y, z, Block::COBBLESTONE->id);
    }
    fizz(world, x, y, z);
}

void LiquidBlock::onPlaced(World* world, int x, int y, int z)
{
    checkBlockCollisions(world, x, y, z);
}

void LiquidBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    checkBlockCollisions(world, x, y, z);
}

void LiquidBlock::randomDisplayTick(
    World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr) {
        return;
    }
    if (&material == &material::Material::WATER) {
        const int meta = world->getBlockMeta(x, y, z);
        if (random.nextInt(64) == 0 && meta > 0 && meta < 8) {
            world->playSound(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(z) + 0.5f,
                "liquid.water",
                random.nextFloat() * 0.25f + 0.75f,
                random.nextFloat() * 1.0f + 0.5f);
        }
    }
    if (&material == &material::Material::LAVA && &world->getMaterial(x, y + 1, z) == &material::Material::AIR
        && !world->isBlockOpaqueCube(x, y + 1, z) && random.nextInt(100) == 0) {
        const double px = static_cast<double>(x) + random.nextFloat();
        const double py = static_cast<double>(y) + static_cast<double>(maxY);
        const double pz = static_cast<double>(z) + random.nextFloat();
        world->addParticle("lava", px, py, pz, 0.0, 0.0, 0.0);
    }
}

} // namespace net::minecraft::block
