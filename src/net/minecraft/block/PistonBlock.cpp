#include "net/minecraft/block/PistonBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/block/PistonExtensionBlock.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/PistonBlockItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {
PistonBlock::PistonBlock(int id, int textureId, bool stickyIn) : Block(id, textureId, material::Material::PISTON) {
    sticky = stickyIn;
    setHardness(0.5f);
}

int PistonBlock::getTexture(int side, int meta) const {
    const int facing = getFacing(meta);
    if (facing > 5) {
        return textureId;
    }
    if (side == facing) {
        if (isExtended(meta) || minX > 0.0 || minY > 0.0 || minZ > 0.0 || maxX < 1.0 || maxY < 1.0 || maxZ < 1.0) {
            return PistonConstants::TEXTURE_FACE_EXTENDED;
        }
        return textureId;
    }
    if (side == PistonConstants::TEXTURE_SIDES[static_cast<std::size_t>(facing)]) {
        return PistonConstants::TEXTURE_INSIDE;
    }
    return PistonConstants::TEXTURE_EXTENSION;
}

int PistonBlock::getFacingForPlacement(World* /*world*/, int x, int y, int z, net::minecraft::PlayerEntity* player) {
    if (player == nullptr) {
        return 0;
    }
    int facing = 0;
    if (MathHelper::abs(player->x - static_cast<float>(x)) < 2.0f &&
        MathHelper::abs(player->z - static_cast<float>(z)) < 2.0f) {
        const double eyeY = player->y + 1.82 - static_cast<double>(player->standingEyeHeight);
        if (eyeY - static_cast<double>(y) > 2.0) {
            return 1;
        }
        if (static_cast<double>(y) - eyeY > 0.0) {
            return 0;
        }
    }
    const int direction = MathHelper::floor(static_cast<double>(player->yaw * 4.0f / 360.0f) + 0.5) & 3;
    if (direction == 0) {
        facing = 2;
    } else if (direction == 1) {
        facing = 5;
    } else if (direction == 2) {
        facing = 3;
    } else if (direction == 3) {
        facing = 4;
    }
    return facing;
}

bool PistonBlock::canMoveBlock(int blockId, World* world, int x, int y, int z, bool allowBreaking) {
    if (world == nullptr || blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
        return false;
    }
    if (Block::OBSIDIAN != nullptr && blockId == Block::OBSIDIAN->id) {
        return false;
    }
    if ((Block::PISTON != nullptr && blockId == Block::PISTON->id) ||
        (Block::STICKY_PISTON != nullptr && blockId == Block::STICKY_PISTON->id)) {
        if (isExtended(world->getBlockMeta(x, y, z))) {
            return false;
        }
    } else {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr || block->getHardness() == -1.0f || block->getPistonBehavior() == 2) {
            return false;
        }
        if (!allowBreaking && block->getPistonBehavior() == 1) {
            return false;
        }
    }
    return world->getBlockEntity(x, y, z) == nullptr;
}

bool PistonBlock::canExtend(World* world, int x, int y, int z, int dir) {
    if (world == nullptr || dir < 0 || dir >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
        return false;
    }
    int px = x + PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(dir)];
    int py = y + PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(dir)];
    int pz = z + PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(dir)];
    for (int step = 0; step < 13; ++step) {
        if (py <= 0 || py >= 127) {
            return false;
        }
        const int blockId = world->getBlockId(px, py, pz);
        if (blockId == 0) {
            break;
        }
        if (!canMoveBlock(blockId, world, px, py, pz, true)) {
            return false;
        }
        if (Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
            Block::BLOCKS[static_cast<std::size_t>(blockId)]->getPistonBehavior() == 1) {
            break;
        }
        if (step == 12) {
            return false;
        }
        px += PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(dir)];
        py += PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(dir)];
        pz += PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(dir)];
    }
    return true;
}

bool PistonBlock::shouldExtend(World* world, int x, int y, int z, int facing) const {
    if (world == nullptr) {
        return false;
    }
    if (facing != 0 && world->isEmittingRedstonePowerInDirection(x, y - 1, z, 0)) {
        return true;
    }
    if (facing != 1 && world->isEmittingRedstonePowerInDirection(x, y + 1, z, 1)) {
        return true;
    }
    if (facing != 2 && world->isEmittingRedstonePowerInDirection(x, y, z - 1, 2)) {
        return true;
    }
    if (facing != 3 && world->isEmittingRedstonePowerInDirection(x, y, z + 1, 3)) {
        return true;
    }
    if (facing != 5 && world->isEmittingRedstonePowerInDirection(x + 1, y, z, 5)) {
        return true;
    }
    if (facing != 4 && world->isEmittingRedstonePowerInDirection(x - 1, y, z, 4)) {
        return true;
    }
    if (world->isEmittingRedstonePowerInDirection(x, y, z, 0)) {
        return true;
    }
    if (world->isEmittingRedstonePowerInDirection(x, y + 2, z, 1)) {
        return true;
    }
    if (world->isEmittingRedstonePowerInDirection(x, y + 1, z - 1, 2)) {
        return true;
    }
    if (world->isEmittingRedstonePowerInDirection(x, y + 1, z + 1, 3)) {
        return true;
    }
    if (world->isEmittingRedstonePowerInDirection(x - 1, y + 1, z, 4)) {
        return true;
    }
    return world->isEmittingRedstonePowerInDirection(x + 1, y + 1, z, 5);
}

void PistonBlock::checkExtended(World* world, int x, int y, int z) {
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const int facing = getFacing(meta);
    const bool extend = shouldExtend(world, x, y, z, facing);
    if (meta == 7) {
        return;
    }
    if (extend && !isExtended(meta)) {
        if (canExtend(world, x, y, z, facing)) {
            world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, facing | 8);
            world->playNoteBlockActionAt(x, y, z, 0, facing);
        }
    } else if (!extend && isExtended(meta)) {
        world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, facing);
        world->playNoteBlockActionAt(x, y, z, 1, facing);
    }
}

void PistonBlock::onPlaced(World* world, int x, int y, int z) {
    if (world != nullptr && !world->isRemote() && world->getBlockEntity(x, y, z) == nullptr) {
        checkExtended(world, x, y, z);
    }
}

void PistonBlock::onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) {
    if (world == nullptr || player == nullptr) {
        return;
    }
    world->setBlockMeta(x, y, z, getFacingForPlacement(world, x, y, z, player));
    if (!world->isRemote()) {
        checkExtended(world, x, y, z);
    }
}

void PistonBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/) {
    if (world != nullptr && !world->isRemote() && !deaf_) {
        checkExtended(world, x, y, z);
    }
}

bool PistonBlock::push(World* world, int x, int y, int z, int dir) {
    if (world == nullptr || dir < 0 || dir >= static_cast<int>(PistonConstants::HEAD_OFFSET_X.size())) {
        return false;
    }
    int px = x + PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(dir)];
    int py = y + PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(dir)];
    int pz = z + PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(dir)];
    for (int step = 0; step < 13; ++step) {
        if (py <= 0 || py >= 127) {
            return false;
        }
        const int blockId = world->getBlockId(px, py, pz);
        if (blockId == 0) {
            break;
        }
        if (!canMoveBlock(blockId, world, px, py, pz, true)) {
            return false;
        }
        if (Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
            Block::BLOCKS[static_cast<std::size_t>(blockId)]->getPistonBehavior() == 1) {
            Block::BLOCKS[static_cast<std::size_t>(blockId)]->dropStacks(
                world, px, py, pz, world->getBlockMeta(px, py, pz));
            world->setBlock(px, py, pz, 0);
            break;
        }
        if (step == 12) {
            return false;
        }
        px += PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(dir)];
        py += PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(dir)];
        pz += PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(dir)];
    }
    while (px != x || py != y || pz != z) {
        const int sourceX = px - PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(dir)];
        const int sourceY = py - PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(dir)];
        const int sourceZ = pz - PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(dir)];
        const int sourceId = world->getBlockId(sourceX, sourceY, sourceZ);
        const int sourceMeta = world->getBlockMeta(sourceX, sourceY, sourceZ);
        if (sourceId == id && sourceX == x && sourceY == y && sourceZ == z) {
            const int headMeta = dir | (sticky ? 8 : 0);
            world->setBlockWithoutNotifyingNeighbors(
                px, py, pz, Block::MOVING_PISTON != nullptr ? Block::MOVING_PISTON->id : 36, headMeta);
            world->setBlockEntity(
                px,
                py,
                pz,
                PistonExtensionBlock::createPistonBlockEntity(
                    Block::PISTON_HEAD != nullptr ? Block::PISTON_HEAD->id : 34, headMeta, dir, true, false));
        } else {
            world->setBlockWithoutNotifyingNeighbors(
                px, py, pz, Block::MOVING_PISTON != nullptr ? Block::MOVING_PISTON->id : 36, sourceMeta);
            world->setBlockEntity(
                px, py, pz, PistonExtensionBlock::createPistonBlockEntity(sourceId, sourceMeta, dir, true, false));
        }
        px = sourceX;
        py = sourceY;
        pz = sourceZ;
    }
    return true;
}

void PistonBlock::onBlockAction(World* world, int x, int y, int z, int data1, int data2) {
    if (world == nullptr) {
        return;
    }
    deaf_ = true;
    const int facing = data2;
    if (data1 == 0) {
        if (push(world, x, y, z, facing)) {
            world->setBlockMeta(x, y, z, facing | 8);
            world->playSound(static_cast<double>(x) + 0.5,
                             static_cast<double>(y) + 0.5,
                             static_cast<double>(z) + 0.5,
                             "tile.piston.out",
                             0.5f,
                             world->random().nextFloat() * 0.25f + 0.6f);
        }
    } else if (data1 == 1) {
        const int headX = x + PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)];
        const int headY = y + PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)];
        const int headZ = z + PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)];
        if (auto* blockEntity = dynamic_cast<entity::PistonBlockEntity*>(world->getBlockEntity(headX, headY, headZ))) {
            blockEntity->finish();
        }
        world->setBlockWithoutNotifyingNeighbors(
            x, y, z, Block::MOVING_PISTON != nullptr ? Block::MOVING_PISTON->id : 36, facing);
        world->setBlockEntity(x, y, z, PistonExtensionBlock::createPistonBlockEntity(id, facing, facing, false, true));
        if (sticky) {
            const int farX = x + PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)] * 2;
            const int farY = y + PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)] * 2;
            const int farZ = z + PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)] * 2;
            int farId = world->getBlockId(farX, farY, farZ);
            int farMeta = world->getBlockMeta(farX, farY, farZ);
            bool pulled = false;
            if (Block::MOVING_PISTON != nullptr && farId == Block::MOVING_PISTON->id) {
                if (auto* moving = dynamic_cast<entity::PistonBlockEntity*>(world->getBlockEntity(farX, farY, farZ))) {
                    if (moving->getFacing() == facing && moving->isExtending()) {
                        moving->finish();
                        farId = moving->getPushedBlockId();
                        farMeta = moving->getPushedBlockData();
                        pulled = true;
                    }
                }
            }
            if (!pulled && farId > 0 && canMoveBlock(farId, world, farX, farY, farZ, false) &&
                Block::BLOCKS[static_cast<std::size_t>(farId)] != nullptr &&
                (Block::BLOCKS[static_cast<std::size_t>(farId)]->getPistonBehavior() == 0 ||
                 farId == (Block::PISTON != nullptr ? Block::PISTON->id : -1) ||
                 farId == (Block::STICKY_PISTON != nullptr ? Block::STICKY_PISTON->id : -1))) {
                deaf_ = false;
                world->setBlock(farX, farY, farZ, 0);
                deaf_ = true;
                x += PistonConstants::HEAD_OFFSET_X[static_cast<std::size_t>(facing)];
                y += PistonConstants::HEAD_OFFSET_Y[static_cast<std::size_t>(facing)];
                z += PistonConstants::HEAD_OFFSET_Z[static_cast<std::size_t>(facing)];
                world->setBlockWithoutNotifyingNeighbors(
                    x, y, z, Block::MOVING_PISTON != nullptr ? Block::MOVING_PISTON->id : 36, farMeta);
                world->setBlockEntity(
                    x, y, z, PistonExtensionBlock::createPistonBlockEntity(farId, farMeta, facing, false, false));
            } else if (!pulled) {
                deaf_ = false;
                world->setBlock(headX, headY, headZ, 0);
                deaf_ = true;
            }
        } else {
            deaf_ = false;
            world->setBlock(headX, headY, headZ, 0);
            deaf_ = true;
        }
        world->playSound(static_cast<double>(x) + 0.5,
                         static_cast<double>(y) + 0.5,
                         static_cast<double>(z) + 0.5,
                         "tile.piston.in",
                         0.5f,
                         world->random().nextFloat() * 0.15f + 0.6f);
    }
    deaf_ = false;
}

void PistonBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z) {
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box PistonBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const {
    if (blockView == nullptr) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    const int meta = blockView->getBlockMeta(x, y, z);
    if (!isExtended(meta)) {
        return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
    switch (getFacing(meta)) {
        case 0:
            return {0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f};
        case 1:
            return {0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f};
        case 2:
            return {0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f};
        case 3:
            return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f};
        case 4:
            return {0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
        case 5:
            return {0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f};
        default:
            return {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    }
}

void PistonBlock::addIntersectingBoundingBox(
    World* /*world*/, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const {
    const Box fullCube(static_cast<double>(x),
                       static_cast<double>(y),
                       static_cast<double>(z),
                       static_cast<double>(x + 1),
                       static_cast<double>(y + 1),
                       static_cast<double>(z + 1));
    if (box.intersects(fullCube)) {
        boxes.push_back(fullCube);
    }
}

void PistonBlock::registerClass() {
    Block::STICKY_PISTON = (new PistonBlock(kBlockId, 106, true))
                               ->setHardness(0.5f)
                               ->setTranslationKey("pistonStickyBase")
                               ->ignoreMetaUpdates();
    Block::PISTON =
        (new PistonBlock(33, 107, false))->setHardness(0.5f)->setTranslationKey("pistonBase")->ignoreMetaUpdates();
}

void PistonBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Block::PISTON),
                                  {std::string("TTT"),
                                   std::string("#X#"),
                                   std::string("#R#"),
                                   '#',
                                   Block::COBBLESTONE,
                                   'X',
                                   Item::byRawId(9),
                                   'R',
                                   Item::byRawId(75),
                                   'T',
                                   Block::PLANKS});
    recipeManager.addShapedRecipe(ItemStack(Block::STICKY_PISTON),
                                  {std::string("S"), std::string("P"), 'S', Item::byRawId(85), 'P', Block::PISTON});
}

void PistonBlock::registerBlockItems() {
    new item::PistonBlockItem(29 - 256);
    new item::PistonBlockItem(33 - 256);
}
MC_REGISTER_BLOCK(PistonBlock)
}  // namespace net::minecraft::block
