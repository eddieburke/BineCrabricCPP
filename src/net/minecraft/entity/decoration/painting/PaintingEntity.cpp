#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>
#include <vector>

namespace net::minecraft::entity::decoration::painting {

PaintingEntity::PaintingEntity(World* worldIn) : Entity(worldIn)
{
    standingEyeHeight = 0.0f;
    setBoundingBoxSpacing(0.5f, 0.5f);
}

PaintingEntity::PaintingEntity(World* worldIn, int x, int y, int z, int facingIn) : PaintingEntity(worldIn)
{
    attachmentX = x;
    attachmentY = y;
    attachmentZ = z;
    std::vector<const PaintingVariant*> validVariants;
    validVariants.reserve(PAINTING_VARIANTS.size());
    for (const PaintingVariant& candidate : PAINTING_VARIANTS) {
        variant = candidate;
        setFacing(facingIn);
        if (canStayAttached()) {
            validVariants.push_back(&candidate);
        }
    }
    if (!validVariants.empty()) {
        variant = *validVariants[static_cast<std::size_t>(random.nextInt(static_cast<int>(validVariants.size())))];
    }
    setFacing(facingIn);
}

PaintingEntity::PaintingEntity(World* worldIn, int x, int y, int z, int facingIn, const std::string& variantId)
    : PaintingEntity(worldIn)
{
    attachmentX = x;
    attachmentY = y;
    attachmentZ = z;
    variant = paintingVariantById(variantId);
    setFacing(facingIn);
}

void PaintingEntity::setFacing(int facingIn)
{
    facing = facingIn;
    prevYaw = yaw = static_cast<float>(facingIn * 90);
    float halfWidth = static_cast<float>(variant.width);
    float halfHeight = static_cast<float>(variant.height);
    float halfDepth = static_cast<float>(variant.width);
    if (facingIn == 0 || facingIn == 2) {
        halfDepth = 0.5f;
    } else {
        halfWidth = 0.5f;
    }
    halfWidth /= 32.0f;
    halfHeight /= 32.0f;
    halfDepth /= 32.0f;
    float centerX = static_cast<float>(attachmentX) + 0.5f;
    float centerY = static_cast<float>(attachmentY) + 0.5f;
    float centerZ = static_cast<float>(attachmentZ) + 0.5f;
    constexpr float wallOffset = 0.5625f;
    if (facingIn == 0) {
        centerZ -= wallOffset;
    } else if (facingIn == 1) {
        centerX -= wallOffset;
    } else if (facingIn == 2) {
        centerZ += wallOffset;
    } else if (facingIn == 3) {
        centerX += wallOffset;
    }
    if (facingIn == 0) {
        centerX -= getHorizontalOffset(variant.width);
    } else if (facingIn == 1) {
        centerZ += getHorizontalOffset(variant.width);
    } else if (facingIn == 2) {
        centerX += getHorizontalOffset(variant.width);
    } else if (facingIn == 3) {
        centerZ -= getHorizontalOffset(variant.width);
    }
    centerY += getHorizontalOffset(variant.height);
    setPosition(static_cast<double>(centerX), static_cast<double>(centerY), static_cast<double>(centerZ));
    constexpr float inset = -0.00625f;
    boundingBox = Box(
        static_cast<double>(centerX) - static_cast<double>(halfWidth) - static_cast<double>(inset),
        static_cast<double>(centerY) - static_cast<double>(halfHeight) - static_cast<double>(inset),
        static_cast<double>(centerZ) - static_cast<double>(halfDepth) - static_cast<double>(inset),
        static_cast<double>(centerX) + static_cast<double>(halfWidth) + static_cast<double>(inset),
        static_cast<double>(centerY) + static_cast<double>(halfHeight) + static_cast<double>(inset),
        static_cast<double>(centerZ) + static_cast<double>(halfDepth) + static_cast<double>(inset));
}

float PaintingEntity::getHorizontalOffset(int width) const
{
    if (width == 32 || width == 64) {
        return 0.5f;
    }
    return 0.0f;
}

void PaintingEntity::dropPaintingItem()
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int paintingId = Item::PAINTING != nullptr ? Item::PAINTING->id : 321;
    dropItem(ItemStack(paintingId, 1, 0), 0.0f);
}

void PaintingEntity::tick()
{
    Entity::tick();
    if (world == nullptr || world->isRemote()) {
        return;
    }
    if (obstructionCheckCounter++ == 100) {
        obstructionCheckCounter = 0;
        if (!canStayAttached()) {
            markDead();
            dropPaintingItem();
        }
    }
}

bool PaintingEntity::canStayAttached() const
{
    if (world == nullptr || variant.width <= 0 || variant.height <= 0) {
        return false;
    }
    if (!world->getEntityCollisions(const_cast<PaintingEntity*>(this), boundingBox).empty()) {
        return false;
    }

    const int blockWidth = variant.width / 16;
    const int blockHeight = variant.height / 16;
    int originX = attachmentX;
    int originY = attachmentY;
    int originZ = attachmentZ;
    if (facing == 0 || facing == 2) {
        originX = MathHelper::floor(x - static_cast<double>(static_cast<float>(variant.width) / 32.0f));
    }
    if (facing == 1 || facing == 3) {
        originZ = MathHelper::floor(z - static_cast<double>(static_cast<float>(variant.width) / 32.0f));
    }
    originY = MathHelper::floor(y - static_cast<double>(static_cast<float>(variant.height) / 32.0f));

    for (int dx = 0; dx < blockWidth; ++dx) {
        for (int dy = 0; dy < blockHeight; ++dy) {
            block::material::Material& material = (facing == 0 || facing == 2)
                ? world->getMaterial(originX + dx, originY + dy, attachmentZ)
                : world->getMaterial(attachmentX, originY + dy, originZ + dx);
            if (!material.isSolid()) {
                return false;
            }
        }
    }

    for (Entity* entity : world->getEntities(const_cast<PaintingEntity*>(this), boundingBox)) {
        if (dynamic_cast<const PaintingEntity*>(entity) != nullptr) {
            return false;
        }
    }
    return true;
}

bool PaintingEntity::damage(Entity* /*damageSource*/, int /*amount*/)
{
    if (!dead && world != nullptr && !world->isRemote()) {
        markDead();
        scheduleVelocityUpdate();
        dropPaintingItem();
    }
    return true;
}

void PaintingEntity::move(double dx, double dy, double dz)
{
    if (world != nullptr && !world->isRemote() && dx * dx + dy * dy + dz * dz > 0.0) {
        markDead();
        dropPaintingItem();
        return;
    }
    Entity::move(dx, dy, dz);
}

void PaintingEntity::addVelocity(double vx, double vy, double vz)
{
    if (world != nullptr && !world->isRemote() && vx * vx + vy * vy + vz * vz > 0.0) {
        markDead();
        dropPaintingItem();
        return;
    }
    Entity::addVelocity(vx, vy, vz);
}

void PaintingEntity::writeNbt(NbtCompound& nbt) const
{
    Entity::writeNbt(nbt);
    nbt.putByte("Dir", static_cast<std::int8_t>(facing));
    nbt.putString("Motive", variant.id != nullptr ? variant.id : "Kebab");
    nbt.putInt("TileX", attachmentX);
    nbt.putInt("TileY", attachmentY);
    nbt.putInt("TileZ", attachmentZ);
}

void PaintingEntity::readNbt(const NbtCompound& nbt)
{
    Entity::readNbt(nbt);
    facing = nbt.getByte("Dir");
    attachmentX = nbt.getInt("TileX");
    attachmentY = nbt.getInt("TileY");
    attachmentZ = nbt.getInt("TileZ");
    variant = paintingVariantById(nbt.getString("Motive"));
    setFacing(facing);
}

} // namespace net::minecraft::entity::decoration::painting
