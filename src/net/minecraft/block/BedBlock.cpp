#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/BedBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/SleepAttemptResult.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/Facings.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

BedBlock::BedBlock(int id) : Block(id, 134, material::Material::WOOL)
{
    setDefaultShape();
}

void BedBlock::setDefaultShape()
{
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f);
}

int BedBlock::getTexture(int side, int meta) const
{
    if (side == 0) {
        return Block::PLANKS != nullptr ? Block::PLANKS->textureId : 4;
    }
    const int direction = getDirection(meta);
    const int mappedSide = util::math::Facings::BED_FACINGS[static_cast<std::size_t>(direction)][static_cast<std::size_t>(side)];
    if (isHeadOfBed(meta)) {
        if (mappedSide == 2) {
            return textureId + 2 + 16;
        }
        if (mappedSide == 5 || mappedSide == 4) {
            return textureId + 1 + 16;
        }
        return textureId + 1;
    }
    if (mappedSide == 3) {
        return textureId - 1 + 16;
    }
    if (mappedSide == 5 || mappedSide == 4) {
        return textureId + 16;
    }
    return textureId;
}

int BedBlock::getDroppedItemId(int blockMeta, JavaRandom& /*random*/) const
{
    if (isHeadOfBed(blockMeta)) {
        return 0;
    }
    return Item::BED != nullptr ? Item::BED->id : 355;
}

void BedBlock::updateBoundingBox(const BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/)
{
    setDefaultShape();
}

void BedBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const int direction = getDirection(meta);
    if (isHeadOfBed(meta)) {
        if (world->getBlockId(x - BED_OFFSETS[direction][0], y, z - BED_OFFSETS[direction][1]) != id) {
            world->setBlock(x, y, z, 0);
        }
        return;
    }
    if (world->getBlockId(x + BED_OFFSETS[direction][0], y, z + BED_OFFSETS[direction][1]) != id) {
        world->setBlock(x, y, z, 0);
        if (!world->isRemote()) {
            Block::dropStacks(world, x, y, z, meta);
        }
    }
}

void BedBlock::dropStacks(World* world, int x, int y, int z, int meta, float luck)
{
    if (!isHeadOfBed(meta)) {
        Block::dropStacks(world, x, y, z, meta, luck);
    }
}

void BedBlock::updateState(World* world, int x, int y, int z, bool occupied)
{
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    meta = occupied ? (meta | 4) : (meta & 0xFFFFFFFB);
    world->setBlockMeta(x, y, z, meta);
}

bool BedBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    if (world == nullptr || player == nullptr) {
        return true;
    }
    if (world->isRemote()) {
        return true;
    }
    int meta = world->getBlockMeta(x, y, z);
    if (!isHeadOfBed(meta)) {
        const int direction = getDirection(meta);
        x += BED_OFFSETS[direction][0];
        z += BED_OFFSETS[direction][1];
        if (world->getBlockId(x, y, z) != id) {
            return true;
        }
        meta = world->getBlockMeta(x, y, z);
    }
    if (world->dimension != nullptr && !world->dimension->hasWorldSpawn()) {
        double centerX = static_cast<double>(x) + 0.5;
        double centerY = static_cast<double>(y) + 0.5;
        double centerZ = static_cast<double>(z) + 0.5;
        world->setBlock(x, y, z, 0);
        const int direction = getDirection(meta);
        const int footX = x + BED_OFFSETS[direction][0];
        const int footZ = z + BED_OFFSETS[direction][1];
        if (world->getBlockId(footX, y, footZ) == id) {
            world->setBlock(footX, y, footZ, 0);
            centerX = (centerX + static_cast<double>(footX) + 0.5) / 2.0;
            centerY = (centerY + static_cast<double>(y) + 0.5) / 2.0;
            centerZ = (centerZ + static_cast<double>(footZ) + 0.5) / 2.0;
        }
        world->createExplosion(nullptr, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f,
            static_cast<float>(z) + 0.5f, 5.0f, true);
        return true;
    }
    if (isOccupied(meta)) {
        net::minecraft::PlayerEntity* occupant = nullptr;
        for (net::minecraft::PlayerEntity* other : world->players) {
            if (other == nullptr || !other->isSleeping() || !other->sleepingPos.has_value()) {
                continue;
            }
            const Vec3i& sleepPos = *other->sleepingPos;
            if (sleepPos.x == x && sleepPos.y == y && sleepPos.z == z) {
                occupant = other;
                break;
            }
        }
        if (occupant == nullptr) {
            updateState(world, x, y, z, false);
        } else {
            player->sendMessage("tile.bed.occupied");
            return true;
        }
    }
    const auto result = player->trySleep(x, y, z);
    if (result == net::minecraft::entity::player::SleepAttemptResult::Ok) {
        updateState(world, x, y, z, true);
        return true;
    }
    if (result == net::minecraft::entity::player::SleepAttemptResult::MonstersNearby) {
        player->sendMessage("tile.bed.noSleep");
    }
    return true;
}

std::optional<Vec3i> BedBlock::findWakeUpPosition(World* world, int x, int y, int z, int skip)
{
    if (world == nullptr) {
        return std::nullopt;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const int direction = getDirection(meta);
    int remainingSkip = skip;
    for (int i = 0; i <= 1; ++i) {
        const int minX = x - BED_OFFSETS[direction][0] * i - 1;
        const int minZ = z - BED_OFFSETS[direction][1] * i - 1;
        const int maxX = minX + 2;
        const int maxZ = minZ + 2;
        for (int bx = minX; bx <= maxX; ++bx) {
            for (int bz = minZ; bz <= maxZ; ++bz) {
                if (world->shouldSuffocate(bx, y - 1, bz) || !world->isAir(bx, y, bz) || !world->isAir(bx, y + 1, bz)) {
                    continue;
                }
                if (remainingSkip > 0) {
                    --remainingSkip;
                    continue;
                }
                return Vec3i{bx, y, bz};
            }
        }
    }
    return std::nullopt;
}
namespace {

void registerBedBlock()
{
    Block::BED = (new BedBlock(26))->setHardness(0.2f)->setTranslationKey("bed")->disableTrackingStatistics()->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerBedBlock, 26);

} // namespace
} // namespace net::minecraft::block

