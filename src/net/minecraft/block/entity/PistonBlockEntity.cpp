#include "net/minecraft/block/entity/PistonBlockEntity.hpp"

#include <vector>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonExtensionBlock.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block::entity {
namespace {
std::vector<Entity*> pushedEntities;
}  // namespace

PistonBlockEntity::PistonBlockEntity(int pushedBlockId, int pushedBlockData, int facing, bool extending, bool source)
    : pushedBlockId_(pushedBlockId),
      pushedBlockData_(pushedBlockData),
      facing_(facing),
      extending_(extending),
      source_(source) {
}

float PistonBlockEntity::getProgress(float tickDelta) const noexcept {
    tickDelta = std::min(tickDelta, 1.0f);
    return progress_ + (lastProgress_ - progress_) * tickDelta;
}

float PistonBlockEntity::renderOffset(float tickDelta, int axisOffset) const {
    const float progress = getProgress(tickDelta);
    if (extending_) {
        return (progress - 1.0f) * static_cast<float>(axisOffset);
    }
    return (1.0f - progress) * static_cast<float>(axisOffset);
}

float PistonBlockEntity::getRenderOffsetX(float tickDelta) const {
    return renderOffset(tickDelta, block::PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing_)]);
}

float PistonBlockEntity::getRenderOffsetY(float tickDelta) const {
    return renderOffset(tickDelta, block::PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing_)]);
}

float PistonBlockEntity::getRenderOffsetZ(float tickDelta) const {
    return renderOffset(tickDelta, block::PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing_)]);
}

void PistonBlockEntity::pushEntities(float collisionShapeSizeMultiplier, float entityMoveMultiplier) {
    if (world == nullptr) {
        return;
    }
    float multiplier = collisionShapeSizeMultiplier;
    if (!extending_) {
        multiplier -= 1.0f;
    } else {
        multiplier = 1.0f - multiplier;
    }
    Block* movingPiston = Block::MOVING_PISTON;
    auto* pistonExtension = dynamic_cast<PistonExtensionBlock*>(movingPiston);
    if (pistonExtension == nullptr) {
        return;
    }
    const std::optional<Box> box =
        pistonExtension->getPushedBlockCollisionShape(world, x, y, z, pushedBlockId_, multiplier, facing_);
    if (!box.has_value()) {
        return;
    }
    const std::vector<Entity*> entities = world->getEntities(nullptr, *box);
    if (entities.empty()) {
        return;
    }
    pushedEntities.insert(pushedEntities.end(), entities.begin(), entities.end());
    const float moveX = entityMoveMultiplier *
                        static_cast<float>(block::PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing_)]);
    const float moveY = entityMoveMultiplier *
                        static_cast<float>(block::PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing_)]);
    const float moveZ = entityMoveMultiplier *
                        static_cast<float>(block::PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing_)]);
    for (Entity* entity : pushedEntities) {
        if (entity != nullptr) {
            entity->move(moveX, moveY, moveZ);
        }
    }
    pushedEntities.clear();
}

void PistonBlockEntity::completeMovement() {
    if (world == nullptr) {
        markRemoved();
        return;
    }
    world->removeBlockEntity(x, y, z);
    markRemoved();
    const int movingPistonId = Block::MOVING_PISTON != nullptr ? Block::MOVING_PISTON->id : 36;
    if (world->getBlockId(x, y, z) == movingPistonId) {
        world->setBlock(x, y, z, pushedBlockId_, static_cast<std::uint8_t>(pushedBlockData_));
    }
}

void PistonBlockEntity::finish() {
    if (progress_ < 1.0f) {
        lastProgress_ = 1.0f;
        progress_ = 1.0f;
        completeMovement();
    }
}

void PistonBlockEntity::tick() {
    progress_ = lastProgress_;
    if (progress_ >= 1.0f) {
        pushEntities(1.0f, 0.25f);
        completeMovement();
        return;
    }
    lastProgress_ = std::min(1.0f, lastProgress_ + 0.5f);
    if (lastProgress_ >= 1.0f) {
        lastProgress_ = 1.0f;
    }
    if (extending_) {
        pushEntities(lastProgress_, lastProgress_ - progress_ + 0.0625f);
    }
}

void PistonBlockEntity::readNbt(const NbtCompound& nbt) {
    BlockEntity::readNbt(nbt);
    pushedBlockId_ = nbt.getInt("blockId");
    pushedBlockData_ = nbt.getInt("blockData");
    facing_ = nbt.getInt("facing");
    progress_ = lastProgress_ = nbt.getFloat("progress");
    extending_ = nbt.getBoolean("extending");
}

void PistonBlockEntity::writeNbt(NbtCompound& nbt) const {
    BlockEntity::writeNbt(nbt);
    nbt.putInt("blockId", pushedBlockId_);
    nbt.putInt("blockData", pushedBlockData_);
    nbt.putInt("facing", facing_);
    nbt.putFloat("progress", progress_);
    nbt.putBoolean("extending", extending_);
}
}  // namespace net::minecraft::block::entity
