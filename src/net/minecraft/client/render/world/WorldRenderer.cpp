#include "net/minecraft/client/render/world/WorldRenderer.hpp"

#include "net/minecraft/client/render/world/WorldRendererCore.hpp"

#include "net/minecraft/client/Minecraft.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/gl/GlExtensions.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/particle/ParticleRegistry.hpp"
#include "net/minecraft/client/particle/PickupParticle.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/ViewDistance.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/WorldBlockViewAdapter.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>

namespace net::minecraft::client::render {

namespace {

constexpr int kChunkSectionSize = 16;
constexpr int kChunkSectionCountY = 8;
constexpr int kGlDstColor = 0x0306;
constexpr int kFrustumCullStride = 8;

} // namespace

WorldRenderer::WorldRenderer(net::minecraft::client::Minecraft* minecraftIn,
    net::minecraft::client::texture::TextureManager* textureManagerIn)
    : client(minecraftIn),
      textureManager(textureManagerIn)
{
    if (client != nullptr) {
        options_ = &client->options;
    }
}

net::minecraft::client::option::GameOptions& WorldRenderer::activeOptions() const
{
    if (options_ != nullptr) {
        return *options_;
    }
    if (client != nullptr) {
        return const_cast<net::minecraft::client::option::GameOptions&>(client->options);
    }
    return const_cast<WorldRenderer*>(this)->defaultOptions_;
}

const net::minecraft::LivingEntity* WorldRenderer::resolveSortCamera() const
{
    if (cameraEntity_ != nullptr) {
        return dynamic_cast<const net::minecraft::LivingEntity*>(cameraEntity_);
    }
    return client != nullptr ? client->camera : nullptr;
}


chunk::ChunkBuilder* WorldRenderer::sectionAt(int sectionX, int sectionY, int sectionZ)
{
    const auto it = sections_.find(world::SectionPos {sectionX, sectionY, sectionZ});
    return it == sections_.end() ? nullptr : it->second.get();
}

int WorldRenderer::ringOf(int sectionX, int sectionZ) const noexcept
{
    const int dx = std::abs(sectionX - centerSectionX_);
    const int dz = std::abs(sectionZ - centerSectionZ_);
    int ring = dx > dz ? dx : dz;
    if (ring < 0) {
        ring = 0;
    }
    if (ring > renderRadiusChunks_) {
        ring = renderRadiusChunks_;
    }
    return ring;
}

void WorldRenderer::enqueueDirtyChunk(chunk::ChunkBuilder* chunk)
{
    if (chunk == nullptr || chunk->meshJobInFlight) {
        return;
    }
    // Near classification runs even when already dirty: a fresh block edit in a
    // section the backlog dirtied earlier must still jump to the fast lane.
    noteNearDirty(chunk);
    dirtyChunks_.insert(chunk);
}

void WorldRenderer::noteNearDirty(chunk::ChunkBuilder* chunk)
{
    // 32 blocks: reach distance plus the neighbor sections a boundary edit
    // dirties. Fixed in world space, so "near" stays near at any render scale.
    constexpr float kNearDirtyDistSq = 32.0f * 32.0f;
    constexpr std::size_t kNearDirtyCap = 64;

    if (nearDirtyChunks_.size() >= kNearDirtyCap) {
        return;
    }
    const net::minecraft::Entity* camera = cameraEntity_ != nullptr
        ? cameraEntity_
        : (client != nullptr ? client->camera : nullptr);
    if (camera == nullptr || chunk->squaredDistanceTo(camera->x, camera->y, camera->z) > kNearDirtyDistSq) {
        return;
    }
    if (std::find(nearDirtyChunks_.begin(), nearDirtyChunks_.end(), chunk) != nearDirtyChunks_.end()) {
        return;
    }
    nearDirtyChunks_.push_back(chunk);
}

void WorldRenderer::enqueueColumn(int sectionX, int sectionZ)
{
    if (sectionAt(sectionX, 0, sectionZ) != nullptr) {
        return;
    }
    const world::SectionPos key {sectionX, 0, sectionZ};
    if (pendingSet_.insert(key).second) {
        pendingColumns_.push_back(key);
    }
}

void WorldRenderer::createColumn(int sectionX, int sectionZ)
{
    const int ring = ringOf(sectionX, sectionZ);
    if (ring >= static_cast<int>(drawRings_.size())) {
        drawRings_.resize(static_cast<std::size_t>(ring) + 1);
    }
    for (int sectionY = 0; sectionY < kChunkSectionCountY; ++sectionY) {
        const world::SectionPos pos {sectionX, sectionY, sectionZ};
        if (sections_.contains(pos)) {
            continue;
        }
        const int base = listPool_.acquirePair();
        auto builder = std::make_unique<chunk::ChunkBuilder>(world, globalBlockEntities, sectionX * kChunkSectionSize,
            sectionY * kChunkSectionSize, sectionZ * kChunkSectionSize, kChunkSectionSize, base);
        builder->id = nextSectionId_++;
        builder->inFrustum = true;
        builder->invalidate();
        chunk::ChunkBuilder* raw = builder.get();
        sections_.emplace(pos, std::move(builder));
        drawRings_[static_cast<std::size_t>(ring)].push_back(raw);
        dirtyChunks_.insert(raw);
    }
}

void WorldRenderer::retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section)
{
    if (section == nullptr) {
        return;
    }
    dirtyChunks_.erase(section.get());
    std::erase(nearDirtyChunks_, section.get());
    if (section->meshJobInFlight) {
        // A mesh job still references this builder; keep it alive until the job
        // completes (sweepRetiring), then recycle its display-list pair.
        section->retired = true;
        retiring_.push_back(std::move(section));
        return;
    }
    listPool_.releasePair(section->baseRenderList);
}

void WorldRenderer::sweepRetiring()
{
    for (auto it = retiring_.begin(); it != retiring_.end();) {
        if ((*it)->meshJobInFlight) {
            ++it;
            continue;
        }
        listPool_.releasePair((*it)->baseRenderList);
        it = retiring_.erase(it);
    }
}

void WorldRenderer::rebuildDrawRings()
{
    drawRings_.assign(static_cast<std::size_t>(renderRadiusChunks_) + 1, {});
    for (auto& entry : sections_) {
        drawRings_[static_cast<std::size_t>(ringOf(entry.first.x, entry.first.z))].push_back(entry.second.get());
    }
}

void WorldRenderer::clearSections()
{
    // Drain the worker pool before destroying the ChunkBuilders its jobs point at.
    meshScheduler_.cancelAll();
    pendingMeshUploads_.clear();
    sections_.clear();
    retiring_.clear();
    dirtyChunks_.clear();
    nearDirtyChunks_.clear();
    drawRings_.clear();
    pendingColumns_.clear();
    pendingSet_.clear();
    chunkRenderers_.clear();
    listPool_.releaseAll();
    centerSectionX_ = std::numeric_limits<int>::min();
    centerSectionZ_ = std::numeric_limits<int>::min();
}

void WorldRenderer::updateSectionFrontier()
{
    const net::minecraft::LivingEntity* camera = resolveSortCamera();
    if (camera == nullptr) {
        return;
    }
    const int camSectionX = MathHelper::floor(camera->x) >> 4;
    const int camSectionZ = MathHelper::floor(camera->z) >> 4;
    if (camSectionX == centerSectionX_ && camSectionZ == centerSectionZ_) {
        return;
    }

    const int oldCenterX = centerSectionX_;
    const int oldCenterZ = centerSectionZ_;
    centerSectionX_ = camSectionX;
    centerSectionZ_ = camSectionZ;
    const int radius = renderRadiusChunks_;

    // Free / retire every section now outside the render square.
    for (auto it = sections_.begin(); it != sections_.end();) {
        if (std::abs(it->first.x - camSectionX) > radius || std::abs(it->first.z - camSectionZ) > radius) {
            std::unique_ptr<chunk::ChunkBuilder> section = std::move(it->second);
            it = sections_.erase(it);
            retireOrFreeSection(std::move(section));
        } else {
            ++it;
        }
    }

    rebuildDrawRings();

    // Enqueue the columns newly in range, nearest ring first, so near terrain is
    // created (and meshed) before the distance. teleported = whole disc on a
    // first recenter / long jump; otherwise just the border that moved in.
    const bool teleported = oldCenterX == std::numeric_limits<int>::min()
        || std::abs(camSectionX - oldCenterX) > radius || std::abs(camSectionZ - oldCenterZ) > radius;
    for (int r = 0; r <= radius; ++r) {
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (std::max(std::abs(dx), std::abs(dz)) != r) {
                    continue; // ring border only
                }
                const int sx = camSectionX + dx;
                const int sz = camSectionZ + dz;
                if (!teleported && std::abs(sx - oldCenterX) <= radius && std::abs(sz - oldCenterZ) <= radius) {
                    continue;
                }
                enqueueColumn(sx, sz);
            }
        }
    }
}

void WorldRenderer::drainPendingColumns()
{
    if (world == nullptr || centerSectionX_ == std::numeric_limits<int>::min()) {
        return;
    }
    net::minecraft::ChunkSource* source = world->getChunkSource();
    if (source == nullptr) {
        return;
    }

    constexpr int kMaxColumnCreatesPerFrame = 16;
    const int radius = renderRadiusChunks_;
    int created = 0;
    std::size_t inspected = 0;
    const std::size_t budget = pendingColumns_.size();

    while (created < kMaxColumnCreatesPerFrame && inspected < budget && !pendingColumns_.empty()) {
        const world::SectionPos col = pendingColumns_.front();
        pendingColumns_.pop_front();
        pendingSet_.erase(col);
        ++inspected;

        if (std::abs(col.x - centerSectionX_) > radius || std::abs(col.z - centerSectionZ_) > radius) {
            continue; // left the frontier before we got to it
        }
        if (sectionAt(col.x, 0, col.z) != nullptr) {
            continue; // already created
        }
        if (!source->isChunkLoaded(col.x, col.z)) {
            // World chunk not resident yet; retry on a later frame.
            if (pendingSet_.insert(col).second) {
                pendingColumns_.push_back(col);
            }
            continue;
        }
        createColumn(col.x, col.z);
        ++created;
    }
}

void WorldRenderer::setWorld(net::minecraft::World* worldIn)
{
    if (world != nullptr) {
        world->removeEventListener(this);
    }

    world = worldIn;
    blockRenderManager.setBlockView(worldIn);

    if (client != nullptr) {
        entity::EntityRenderDispatcher::instance().setWorld(worldIn);
    }

    if (world != nullptr) {
        world->addEventListener(this);
        reload();
    } else {
        clearSections();
        cameraEntity_ = nullptr;
    }
}

void WorldRenderer::reload()
{
    // Tears down the section store + display-list pool and recancels mesh jobs.
    clearSections();

    if (world == nullptr) {
        return;
    }

    net::minecraft::client::option::GameOptions& opts = activeOptions();
    const option::ResolvedRenderOptions resolved = option::resolve(opts);
    const ViewDistance viewDistance = ViewDistance::fromOptions(opts);
    if (Block::LEAVES != nullptr) {
        static_cast<net::minecraft::block::LeavesBlock*>(Block::LEAVES)->setFancyGraphics(
            resolved.fancyLeaves);
    }
    block::BlockRenderManager::fancyGraphics = opts.fancyGraphics;
    block::BlockRenderManager::fancyLeaves = resolved.fancyLeaves;
    lastViewDistance = opts.viewDistance;
    lastRenderScale = viewDistance.renderScale();

    gl::GlExtensions::ensureLoaded();
    // Chunks render exclusively through display lists, allocated per section from
    // listPool_ (no fixed reservation). The VBO path was removed (corrupted
    // geometry + heavy lag); reintroduce it as a separate, verified renderer later.

    renderRadiusChunks_ = viewDistance.chunkGridRadiusChunks();
    globalBlockEntities.clear();
    entityRenderCooldown = 2;

    // Sections are created lazily by updateSectionFrontier / drainPendingColumns
    // once the camera position is known and world chunks become resident; no
    // up-front grid allocation.
}

void WorldRenderer::reloadIfViewDistanceChanged()
{
    const net::minecraft::client::option::GameOptions& opts = activeOptions();
    const ViewDistance viewDistance = ViewDistance::fromOptions(opts);
    if (opts.viewDistance != lastViewDistance || viewDistance.renderScale() != lastRenderScale) {
        reload();
    }
}

void WorldRenderer::beginWorldRenderFrame() noexcept
{
    framePhase_ = FramePhase::Idle;
}

void WorldRenderer::expectFramePhase(const FramePhase required) const
{
#ifndef NDEBUG
    assert(framePhase_ == required);
#else
    (void)required;
#endif
}

void WorldRenderer::advanceFramePhase(const FramePhase next) noexcept
{
    framePhase_ = next;
}

void WorldRenderer::renderLastChunks(int layer, double tickDelta)
{
    internal::WorldRendererCore::renderLastChunks(*this, layer, tickDelta);
}

void WorldRenderer::render(const net::minecraft::Entity& camera, int layer, float tickDelta)
{
    internal::WorldRendererCore::render(*this, camera, layer, tickDelta);
}

int WorldRenderer::render(net::minecraft::LivingEntity& camera, int layer, double tickDelta)
{
    if (layer == 0) {
        expectFramePhase(FramePhase::Compiled);
    } else {
        expectFramePhase(FramePhase::SolidLayerDone);
    }
    const int rendered = internal::WorldRendererCore::render(*this, camera, layer, tickDelta);
    if (layer == 0) {
        advanceFramePhase(FramePhase::SolidLayerDone);
    }
    return rendered;
}

bool WorldRenderer::compileChunks(net::minecraft::LivingEntity& camera, bool force)
{
    expectFramePhase(FramePhase::Culled);
    const bool finished = internal::WorldRendererCore::compileChunks(*this, camera, force);
    advanceFramePhase(FramePhase::Compiled);
    return finished;
}

void WorldRenderer::renderEntities(const Vec3d& cameraPos, Culler* culler, float tickDelta)
{
    internal::WorldRendererCore::renderEntities(*this, cameraPos, culler, tickDelta);
}

void WorldRenderer::cullChunks(Culler* culler, float tickDelta)
{
    (void)tickDelta;
    expectFramePhase(FramePhase::Idle);

    // First phase of the frame: recenter the sparse section set on the camera,
    // create any sections whose world chunk just became resident, then cull.
    reloadIfViewDistanceChanged();
    updateSectionFrontier();
    drainPendingColumns();

    if (culler == nullptr || !activeOptions().frustumCulling) {
        for (auto& entry : sections_) {
            entry.second->inFrustum = true;
        }
        advanceFramePhase(FramePhase::Culled);
        return;
    }

    // Asymmetric cadence: sections outside the frustum are re-tested every frame
    // so they appear the moment the camera turns onto them; visible ones are only
    // re-tested every kFrustumCullStride frames (worst case: a stale section
    // renders a few frames longer).
    std::size_t index = 0;
    for (auto& entry : sections_) {
        chunk::ChunkBuilder& chunk = *entry.second;
        const std::size_t i = index++;
        if (chunk.hasNoGeometry()) {
            continue;
        }
        if (chunk.inFrustum
            && static_cast<int>((i + static_cast<std::size_t>(cullStep)) % kFrustumCullStride) != 0) {
            continue;
        }
        chunk.updateFrustum(*culler);
    }
    cullStep = (cullStep + 1) % kFrustumCullStride;
    advanceFramePhase(FramePhase::Culled);
}

std::string WorldRenderer::getChunkDebugInfo() const
{
    return "C: " + std::to_string(compiledChunkCount) + "/" + std::to_string(chunkCount) + ". F: "
        + std::to_string(invisibleChunkCount) + ", E: " + std::to_string(emptyChunkCount);
}

std::string WorldRenderer::getEntityDebugInfo() const
{
    return "E: " + std::to_string(renderedEntityCount) + "/" + std::to_string(entityCount) + ". B: "
        + std::to_string(culledEntityCount) + ", I: "
        + std::to_string(entityCount - culledEntityCount - renderedEntityCount);
}

void WorldRenderer::markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    const int startX = MathHelper::floorDiv(minX, kChunkSectionSize);
    const int startY = MathHelper::floorDiv(minY, kChunkSectionSize);
    const int startZ = MathHelper::floorDiv(minZ, kChunkSectionSize);
    const int endX = MathHelper::floorDiv(maxX, kChunkSectionSize);
    const int endY = MathHelper::floorDiv(maxY, kChunkSectionSize);
    const int endZ = MathHelper::floorDiv(maxZ, kChunkSectionSize);

    for (int chunkX = startX; chunkX <= endX; ++chunkX) {
        for (int chunkY = startY; chunkY <= endY; ++chunkY) {
            for (int chunkZ = startZ; chunkZ <= endZ; ++chunkZ) {
                chunk::ChunkBuilder* builder = sectionAt(chunkX, chunkY, chunkZ);
                if (builder == nullptr) {
                    // No live section here (out of range or not yet created); it
                    // will be (re)built dirty when its column is created.
                    continue;
                }
                // Always bump the version: a mesh job may be in flight for this
                // builder, and only a version mismatch makes its result stale.
                // Skipping the bump when already dirty would let that job upload
                // a mesh missing this change and clear the dirty flag.
                builder->invalidate();
                enqueueDirtyChunk(builder);
            }
        }
    }
}

void WorldRenderer::blockUpdate(int x, int y, int z)
{
    markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}

void WorldRenderer::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    markDirty(minX - 1, minY - 1, minZ - 1, maxX + 1, maxY + 1, maxZ + 1);
}

void WorldRenderer::addParticle(const std::string& particle, double x, double y, double z, double velocityX,
    double velocityY, double velocityZ)
{
    if (client == nullptr) {
        return;
    }
    if (!client::option::shouldSpawnParticle(client::option::resolve(client->options), particle)) {
        return;
    }
    net::minecraft::Entity* camera = cameraEntity_ != nullptr ? cameraEntity_ : client->camera;
    if (camera == nullptr) {
        return;
    }
    const double dx = camera->x - x;
    const double dy = camera->y - y;
    const double dz = camera->z - z;
    if (dx * dx + dy * dy + dz * dz > 16.0 * 16.0) {
        return;
    }
    client::particle::ParticleSpawnContext context {
        world, textureManager, x, y, z, velocityX, velocityY, velocityZ};
    std::unique_ptr<client::particle::Particle> spawned =
        client::particle::ParticleRegistry::instance().create(particle, context);
    if (spawned != nullptr) {
        client->particleManager.addParticle(std::move(spawned));
    }
}

void WorldRenderer::notifyEntityAdded(net::minecraft::Entity* entity)
{
    if (entity == nullptr || client == nullptr) {
        return;
    }
    entity->updateCapeUrl();
    if (!entity->skinUrl.empty()) {
        client->textureManager.downloadSkinImage(entity->skinUrl);
    }
    if (!entity->capeUrl.empty()) {
        client->textureManager.downloadCapeImage(entity->capeUrl);
    }
}

void WorldRenderer::notifyEntityRemoved(net::minecraft::Entity* entity)
{
    if (entity == nullptr || client == nullptr) {
        return;
    }
    if (!entity->skinUrl.empty()) {
        client->textureManager.releaseImage(entity->skinUrl);
    }
    if (!entity->capeUrl.empty()) {
        client->textureManager.releaseImage(entity->capeUrl);
    }
}

void WorldRenderer::notifyAmbientDarknessChanged()
{
    for (auto& entry : sections_) {
        chunk::ChunkBuilder& chunk = *entry.second;
        if (!chunk.hasSkyLight || chunk.dirty) {
            continue;
        }
        chunk.invalidate();
        enqueueDirtyChunk(&chunk);
    }
}

void WorldRenderer::updateBlockEntity(int x, int y, int z, net::minecraft::block::entity::BlockEntity* /*blockEntity*/)
{
    markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}

void WorldRenderer::onEntityPickup(net::minecraft::Entity* entity, net::minecraft::PlayerEntity* collector)
{
    if (client == nullptr || world == nullptr || entity == nullptr || collector == nullptr) {
        return;
    }
    client->particleManager.addParticle(
        new ::net::minecraft::client::particle::PickupParticle(world, entity, collector, -0.5f));
}

void WorldRenderer::blockBreakParticles(int x, int y, int z, int blockId, int blockMeta)
{
    if (client == nullptr) {
        return;
    }
    client->particleManager.addBlockBreakParticles(x, y, z, blockId, blockMeta);
}

void WorldRenderer::renderOutline(const Box& box)
{
    Tessellator& tessellator = INSTANCE;
    tessellator.start(gl::GL11::GL_LINE_STRIP);
    tessellator.vertex(box.minX, box.minY, box.minZ);
    tessellator.vertex(box.maxX, box.minY, box.minZ);
    tessellator.vertex(box.maxX, box.minY, box.maxZ);
    tessellator.vertex(box.minX, box.minY, box.maxZ);
    tessellator.vertex(box.minX, box.minY, box.minZ);
    tessellator.draw();

    tessellator.start(gl::GL11::GL_LINE_STRIP);
    tessellator.vertex(box.minX, box.maxY, box.minZ);
    tessellator.vertex(box.maxX, box.maxY, box.minZ);
    tessellator.vertex(box.maxX, box.maxY, box.maxZ);
    tessellator.vertex(box.minX, box.maxY, box.maxZ);
    tessellator.vertex(box.minX, box.maxY, box.minZ);
    tessellator.draw();

    tessellator.start(gl::GL11::GL_LINES);
    tessellator.vertex(box.minX, box.minY, box.minZ);
    tessellator.vertex(box.minX, box.maxY, box.minZ);
    tessellator.vertex(box.maxX, box.minY, box.minZ);
    tessellator.vertex(box.maxX, box.maxY, box.minZ);
    tessellator.vertex(box.maxX, box.minY, box.maxZ);
    tessellator.vertex(box.maxX, box.maxY, box.maxZ);
    tessellator.vertex(box.minX, box.minY, box.maxZ);
    tessellator.vertex(box.minX, box.maxY, box.maxZ);
    tessellator.draw();
}

void WorldRenderer::renderMiningProgress(net::minecraft::PlayerEntity* entity, const net::minecraft::HitResult& hit,
    int i, const net::minecraft::ItemStack& handStack, float tickDelta)
{
    (void)handStack;
    if (entity == nullptr || world == nullptr || textureManager == nullptr) {
        return;
    }
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE);
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const float pulse = std::sin(static_cast<float>(nowMs) / 100.0f) * 0.2f + 0.4f;
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, pulse * 0.5f);

    if (i == 0 && miningProgress > 0.0f) {
        gl::GL11::glBlendFunc(gl::GL11::GL_DST_COLOR, gl::GL11::GL_SRC_COLOR);
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureManager->getTextureId("/terrain.png"));
        gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        gl::GL11::glPushMatrix();
        int blockId = world->getBlockId(hit.blockX, hit.blockY, hit.blockZ);
        Block* block = (blockId > 0 && blockId < Block::BLOCK_COUNT) ? Block::BLOCKS[blockId] : nullptr;
        if (block == nullptr) {
            block = Block::STONE;
        }
        gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
        gl::GL11::glPolygonOffset(-3.0f, -3.0f);
        gl::GL11::glEnable(gl::GL11::GL_POLYGON_OFFSET_FILL);
        const double interpX = entity->lastTickX + (entity->x - entity->lastTickX) * static_cast<double>(tickDelta);
        const double interpY = entity->lastTickY + (entity->y - entity->lastTickY) * static_cast<double>(tickDelta);
        const double interpZ = entity->lastTickZ + (entity->z - entity->lastTickZ) * static_cast<double>(tickDelta);
        if (block != nullptr) {
            gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
            Tessellator& tessellator = INSTANCE;
            tessellator.startQuads();
            tessellator.translate(-interpX, -interpY, -interpZ);
            tessellator.disableColor();
            blockRenderManager.renderWithTexture(*block, hit.blockX, hit.blockY, hit.blockZ,
                240 + static_cast<int>(miningProgress * 10.0f));
            tessellator.draw();
            tessellator.translate(0.0, 0.0, 0.0);
            gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
        }
        gl::GL11::glDisable(gl::GL11::GL_POLYGON_OFFSET_FILL);
        gl::GL11::glPolygonOffset(0.0f, 0.0f);
        gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
        gl::GL11::glDepthMask(true);
        gl::GL11::glPopMatrix();
    }

    gl::GL11::glDisable(gl::GL11::GL_BLEND);
    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void WorldRenderer::renderBlockOutline(net::minecraft::PlayerEntity* player, const net::minecraft::HitResult& hitResult,
    int i, const net::minecraft::ItemStack& handStack, float tickDelta)
{
    (void)handStack;
    if (i != 0 || hitResult.type != HitResultType::BLOCK || player == nullptr || world == nullptr) {
        return;
    }
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    gl::GL11::glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    gl::GL11::glLineWidth(2.0f);
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glDepthMask(false);
    constexpr float expand = 0.002f;
    const int blockId = world->getBlockId(hitResult.blockX, hitResult.blockY, hitResult.blockZ);
    if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[blockId] != nullptr) {
        Block* block = Block::BLOCKS[blockId];
        WorldBlockViewAdapter blockView(world);
        block->updateBoundingBox(&blockView, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
        const double interpX = player->lastTickX + (player->x - player->lastTickX) * static_cast<double>(tickDelta);
        const double interpY = player->lastTickY + (player->y - player->lastTickY) * static_cast<double>(tickDelta);
        const double interpZ = player->lastTickZ + (player->z - player->lastTickZ) * static_cast<double>(tickDelta);
        Box outline = block->getBoundingBox(world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
        outline = outline.expand(expand).offset(-interpX, -interpY, -interpZ);
        renderOutline(outline);
    }
    gl::GL11::glDepthMask(true);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
}

void WorldRenderer::releaseGlLists()
{
    clearSections();
}

} // namespace net::minecraft::client::render
