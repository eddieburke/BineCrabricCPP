#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/WorldRegion.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <algorithm>
#include <unordered_set>

namespace net::minecraft::client::render::chunk {

namespace {

constexpr int kGlCompile = 0x1300;

} // namespace

void ChunkBuilder::endOpenDisplayList() noexcept
{
    if (!dlCompileOpen_) {
        return;
    }

    auto& tessellator = net::minecraft::client::render::INSTANCE;
    tessellator.draw();
    gl::GL11::glPopMatrix();
    gl::GL11::glEndList();
    dlCompileOpen_ = false;
    tessellator.translate(0.0, 0.0, 0.0);
}

void ChunkBuilder::beginLayerCompile(int baseListId, const ChunkDrawTransform& transform)
{
    if (baseListId < 0) {
        return;
    }

    gl::GL11::glNewList(baseListId, kGlCompile);
    dlCompileOpen_ = true;
    gl::GL11::glPushMatrix();
    transform.applyGl();
    net::minecraft::client::render::INSTANCE.startQuads();
    net::minecraft::client::render::INSTANCE.translate(static_cast<double>(-x), static_cast<double>(-y),
        static_cast<double>(-z));
}

void ChunkBuilder::finishLayerCompile()
{
    endOpenDisplayList();
}

void ChunkBuilder::drawCompiledLayer(int layerId, float cameraDx, float cameraDy, float cameraDz) const
{
    if (baseRenderList < 0) {
        return;
    }

    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(cameraDx, cameraDy, cameraDz);
    gl::GL11::glCallList(baseRenderList + layerId);
    gl::GL11::glPopMatrix();
}

void ChunkBuilder::setPosition(int newX, int newY, int newZ)
{
    if (newX == x && newY == y && newZ == z) {
        return;
    }
    reset();
    x = newX;
    y = newY;
    z = newZ;
    centerX = x + sizeX / 2;
    centerY = y + sizeY / 2;
    centerZ = z + sizeZ / 2;
    renderX = x & 0x3FF;
    renderY = y;
    renderZ = z & 0x3FF;
    cameraOffsetX = x - renderX;
    cameraOffsetY = y - renderY;
    cameraOffsetZ = z - renderZ;
    constexpr float padding = 6.0f;
    cullingBox = net::minecraft::Box(
        static_cast<double>(x) - padding,
        static_cast<double>(y) - padding,
        static_cast<double>(z) - padding,
        static_cast<double>(x + sizeX) + padding,
        static_cast<double>(y + sizeY) + padding,
        static_cast<double>(z + sizeZ) + padding);
    invalidate();
}

void ChunkBuilder::rebuild()
{
    if (!dirty || world == nullptr) {
        return;
    }

    ++chunkUpdates;
    const int minX = x;
    const int minY = y;
    const int minZ = z;
    const int maxX = x + sizeX;
    const int maxY = y + sizeY;
    const int maxZ = z + sizeZ;

    for (int layer = 0; layer < 2; ++layer) {
        renderLayerEmpty[static_cast<std::size_t>(layer)] = true;
    }

    net::minecraft::Chunk::hasSkyLight = false;

    std::unordered_set<::net::minecraft::block::entity::BlockEntity*> previousBlockEntities;
    previousBlockEntities.insert(blockEntities_.begin(), blockEntities_.end());
    blockEntities_.clear();

    const ChunkDrawTransform transform =
        ChunkDrawTransform::fromChunkPosition(renderX, renderY, renderZ, sizeX, sizeY, sizeZ);

    constexpr int padding = 1;
    net::minecraft::WorldRegion region(world, minX - padding, minY - padding, minZ - padding, maxX + padding,
        maxY + padding, maxZ + padding);
    net::minecraft::client::render::block::BlockRenderManager blockRenderManager(&region);
    auto& blockEntityDispatcher = block::entity::BlockEntityRenderDispatcher::instance();

    for (int layer = 0; layer < 2; ++layer) {
        bool hasOtherLayer = false;
        bool beganCompile = false;
        bool drewGeometry = false;

        for (int blockY = minY; blockY < maxY; ++blockY) {
            for (int blockZ = minZ; blockZ < maxZ; ++blockZ) {
                for (int blockX = minX; blockX < maxX; ++blockX) {
                    const int blockId = region.getBlockId(blockX, blockY, blockZ);
                    if (blockId <= 0) {
                        continue;
                    }

                    net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
                    if (block == nullptr) {
                        continue;
                    }

                    if (layer == 0 && Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(blockId)]) {
                        if (::net::minecraft::block::entity::BlockEntity* blockEntity =
                                region.getBlockEntity(blockX, blockY, blockZ);
                            blockEntity != nullptr && blockEntityDispatcher.hasRenderer(*blockEntity)) {
                            blockEntities_.push_back(blockEntity);
                        }
                    }

                    const int blockLayer = block->getRenderLayer();
                    if (blockLayer != layer) {
                        hasOtherLayer = true;
                        continue;
                    }

                    if (!beganCompile) {
                        beganCompile = true;
                        const int baseListId = baseRenderList >= 0 ? baseRenderList + layer : -1;
                        beginLayerCompile(baseListId, transform);
                    }

                    drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
                }
            }
        }

        if (beganCompile) {
            finishLayerCompile();
        } else {
            drewGeometry = false;
        }

        if (drewGeometry) {
            renderLayerEmpty[static_cast<std::size_t>(layer)] = false;
        }

        if (!hasOtherLayer) {
            break;
        }
    }

    if (currentBlockEntities_ != nullptr) {
        std::unordered_set<::net::minecraft::block::entity::BlockEntity*> currentBlockEntities;
        currentBlockEntities.insert(blockEntities_.begin(), blockEntities_.end());
        for (::net::minecraft::block::entity::BlockEntity* blockEntity : currentBlockEntities) {
            if (!previousBlockEntities.contains(blockEntity)) {
                currentBlockEntities_->push_back(blockEntity);
            }
        }
        for (::net::minecraft::block::entity::BlockEntity* blockEntity : previousBlockEntities) {
            if (!currentBlockEntities.contains(blockEntity)) {
                const auto it = std::find(currentBlockEntities_->begin(), currentBlockEntities_->end(), blockEntity);
                if (it != currentBlockEntities_->end()) {
                    currentBlockEntities_->erase(it);
                }
            }
        }
    }

    hasSkyLight = net::minecraft::Chunk::hasSkyLight;
    built = true;
}

void ChunkBuilder::renderLayer(int layerId, double interpX, double interpY, double interpZ) const
{
    if (!hasLayerGeometry(layerId)) {
        return;
    }

    const float cameraDx = static_cast<float>(cameraOffsetX - interpX);
    const float cameraDy = static_cast<float>(cameraOffsetY - interpY);
    const float cameraDz = static_cast<float>(cameraOffsetZ - interpZ);
    drawCompiledLayer(layerId, cameraDx, cameraDy, cameraDz);
}

void ChunkBuilder::reset()
{
    endOpenDisplayList();
    renderLayerEmpty = {true, true};
    inFrustum = false;
    built = false;
}

void ChunkBuilder::close()
{
    reset();
    world = nullptr;
}

} // namespace net::minecraft::client::render::chunk
