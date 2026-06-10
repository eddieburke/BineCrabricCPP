#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <unordered_set>

namespace net::minecraft::client::render::chunk {

namespace {

constexpr int kGlCompile = 0x1300;

} // namespace

ChunkMeshJob::ChunkMeshJob(ChunkBuilder& owner)
    : builder(&owner),
      version(owner.version),
      x(owner.x),
      y(owner.y),
      z(owner.z),
      sizeX(owner.sizeX),
      sizeY(owner.sizeY),
      sizeZ(owner.sizeZ),
      snapshot(*owner.world, owner.x - 1, owner.z - 1, owner.x + owner.sizeX + 1, owner.z + owner.sizeZ + 1)
{
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

void ChunkBuilder::buildMesh(ChunkMeshJob& job)
{
    const int minX = job.x;
    const int minY = job.y;
    const int minZ = job.z;
    const int maxX = job.x + job.sizeX;
    const int maxY = job.y + job.sizeY;
    const int maxZ = job.z + job.sizeZ;

    Tessellator tessellator;
    tessellator.setCaptureOnly(true);

    block::BlockRenderManager blockRenderManager(&job.snapshot, job.opts, job.fancyGraphics);
    blockRenderManager.ctx.tess = &tessellator;

    ChunkMeshResult& result = job.result;

    for (int layer = 0; layer < 2; ++layer) {
        bool hasOtherLayer = false;
        bool beganCompile = false;
        bool drewGeometry = false;

        for (int blockY = minY; blockY < maxY; ++blockY) {
            for (int blockZ = minZ; blockZ < maxZ; ++blockZ) {
                for (int blockX = minX; blockX < maxX; ++blockX) {
                    const int blockId = job.snapshot.getBlockId(blockX, blockY, blockZ);
                    if (blockId <= 0) {
                        continue;
                    }

                    net::minecraft::block::Block* block =
                        net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
                    if (block == nullptr) {
                        continue;
                    }

                    if (layer == 0 && Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(blockId)]) {
                        result.blockEntityPositions.push_back(net::minecraft::Vec3i {blockX, blockY, blockZ});
                    }

                    const int blockLayer = block->getRenderLayer();
                    if (blockLayer != layer) {
                        hasOtherLayer = true;
                        continue;
                    }

                    if (!beganCompile) {
                        beganCompile = true;
                        tessellator.startQuads();
                        tessellator.translate(
                            static_cast<double>(-job.x), static_cast<double>(-job.y), static_cast<double>(-job.z));
                    }

                    drewGeometry |= blockRenderManager.render(*block, blockX, blockY, blockZ);
                }
            }
        }

        if (beganCompile) {
            result.layers[static_cast<std::size_t>(layer)] = tessellator.takeMesh();
        }
        result.layerEmpty[static_cast<std::size_t>(layer)] = !(beganCompile && drewGeometry);

        if (!hasOtherLayer) {
            break;
        }
    }

    result.hasSkyLight = job.snapshot.sawSkyLight();
}

void ChunkBuilder::uploadMesh(const ChunkMeshJob& job)
{
    ++chunkUpdates;

    const ChunkDrawTransform transform =
        ChunkDrawTransform::fromChunkPosition(renderX, renderY, renderZ, sizeX, sizeY, sizeZ);

    for (int layer = 0; layer < 2; ++layer) {
        const TessellatorMesh& mesh = job.result.layers[static_cast<std::size_t>(layer)];
        renderLayerEmpty[static_cast<std::size_t>(layer)] = job.result.layerEmpty[static_cast<std::size_t>(layer)];
        if (baseRenderList < 0) {
            continue;
        }
        gl::GL11::glNewList(baseRenderList + layer, kGlCompile);
        gl::GL11::glPushMatrix();
        transform.applyGl();
        Tessellator::drawMesh(mesh);
        gl::GL11::glPopMatrix();
        gl::GL11::glEndList();
    }

    // Resolve block-entity positions against the live world and apply the same
    // joined/removed diff the old synchronous rebuild kept.
    std::unordered_set<::net::minecraft::block::entity::BlockEntity*> previousBlockEntities;
    previousBlockEntities.insert(blockEntities_.begin(), blockEntities_.end());
    blockEntities_.clear();

    if (world != nullptr) {
        auto& blockEntityDispatcher = block::entity::BlockEntityRenderDispatcher::instance();
        for (const net::minecraft::Vec3i& pos : job.result.blockEntityPositions) {
            ::net::minecraft::block::entity::BlockEntity* blockEntity = world->getBlockEntity(pos.x, pos.y, pos.z);
            if (blockEntity != nullptr && blockEntityDispatcher.hasRenderer(*blockEntity)) {
                blockEntities_.push_back(blockEntity);
            }
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

    hasSkyLight = job.result.hasSkyLight;
    built = true;
}

void ChunkBuilder::rebuild()
{
    if (!dirty || world == nullptr) {
        return;
    }

    ChunkMeshJob job(*this);
    job.opts = block::BlockRenderManager::blockRenderManagerOptionsSnapshot();
    buildMesh(job);
    uploadMesh(job);
    dirty = false;
}

void ChunkBuilder::renderLayer(int layerId, double interpX, double interpY, double interpZ) const
{
    if (!hasLayerGeometry(layerId)) {
        return;
    }

    const float cameraDx = static_cast<float>(cameraOffsetX - interpX);
    const float cameraDy = static_cast<float>(cameraOffsetY - interpY);
    const float cameraDz = static_cast<float>(cameraOffsetZ - interpZ);

    if (baseRenderList < 0) {
        return;
    }
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(cameraDx, cameraDy, cameraDz);
    gl::GL11::glCallList(baseRenderList + layerId);
    gl::GL11::glPopMatrix();
}

void ChunkBuilder::reset()
{
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
