#include "net/minecraft/client/render/world/WorldRenderer.hpp"

#include "net/minecraft/client/render/world/WorldRendererCore.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/MusicDiscItem.hpp"
#include "net/minecraft/block/LeavesBlock.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/gl/GlExtensions.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/particle/ExplosionParticle.hpp"
#include "net/minecraft/client/particle/FireSmokeParticle.hpp"
#include "net/minecraft/client/particle/FlameParticle.hpp"
#include "net/minecraft/client/particle/FootstepParticle.hpp"
#include "net/minecraft/client/particle/HeartParticle.hpp"
#include "net/minecraft/client/particle/ItemParticle.hpp"
#include "net/minecraft/client/particle/LavaEmberParticle.hpp"
#include "net/minecraft/client/particle/NoteParticle.hpp"
#include "net/minecraft/client/particle/PortalParticle.hpp"
#include "net/minecraft/client/particle/RainSplashParticle.hpp"
#include "net/minecraft/client/particle/PickupParticle.hpp"
#include "net/minecraft/client/particle/RedDustParticle.hpp"
#include "net/minecraft/client/particle/SnowParticle.hpp"
#include "net/minecraft/client/particle/WaterBubbleParticle.hpp"
#include "net/minecraft/client/particle/WaterSplashParticle.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/render/world/DirtyChunkSorter.hpp"
#include "net/minecraft/client/render/world/DistanceChunkSorter.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"

namespace {

net::minecraft::JavaRandom& mathRandom()
{
    static net::minecraft::JavaRandom rng;
    return rng;
}

} // namespace
#include "net/minecraft/world/WorldBlockViewAdapter.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>

namespace net::minecraft::client::render {

namespace {

constexpr int kChunkSectionSize = 16;
constexpr int kChunkSectionCountY = 8;
constexpr int kGlCompile = 0x1300;
constexpr int kGlTriangleFan = 6;
constexpr int kGlDstColor = 0x0306;
constexpr float kPi = 3.14159265f;

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
    if (const auto* living = dynamic_cast<const net::minecraft::LivingEntity*>(cameraEntity_)) {
        return living;
    }
    if (client != nullptr && client->camera != nullptr) {
        return dynamic_cast<const net::minecraft::LivingEntity*>(client->camera);
    }
    if (client != nullptr && client->player != nullptr) {
        return dynamic_cast<const net::minecraft::LivingEntity*>(client->player);
    }
    return nullptr;
}


std::size_t WorldRenderer::chunkIndex(int chunkX, int chunkY, int chunkZ) const
{
    return static_cast<std::size_t>((chunkZ * chunkCountY + chunkY) * chunkCountX + chunkX);
}

chunk::ChunkBuilder* WorldRenderer::chunkAt(int chunkX, int chunkY, int chunkZ)
{
    if (chunkX < 0 || chunkY < 0 || chunkZ < 0 || chunkX >= chunkCountX || chunkY >= chunkCountY
        || chunkZ >= chunkCountZ) {
        return nullptr;
    }
    return &chunks_[chunkIndex(chunkX, chunkY, chunkZ)];
}

void WorldRenderer::enqueueDirtyChunk(chunk::ChunkBuilder* chunk)
{
    if (chunk == nullptr || chunk->meshJobInFlight) {
        return;
    }
    if (dirtyChunks_.contains(chunk)) {
        return;
    }
    chunk->queuedForRebuild = true;
    dirtyChunks_.insert(chunk);
}

void WorldRenderer::setWorld(net::minecraft::World* worldIn)
{
    if (world != nullptr) {
        world->removeEventListener(this);
    }

    prevCameraX = -9999.0;
    prevCameraY = -9999.0;
    prevCameraZ = -9999.0;

    world = worldIn;
    blockRenderManager.setBlockView(worldIn);

    if (client != nullptr) {
        entity::EntityRenderDispatcher::instance().setWorld(worldIn);
    }

    if (world != nullptr) {
        world->addEventListener(this);
        reload();
    } else {
        meshScheduler_.cancelAll();
        pendingMeshUploads_.clear();
        for (chunk::ChunkBuilder& chunk : chunks_) {
            chunk.meshJobInFlight = false;
        }
        chunks_.clear();
        sortedChunks_.clear();
        dirtyChunks_.clear();
        cameraEntity_ = nullptr;
    }
}

void WorldRenderer::reload()
{
    meshScheduler_.cancelAll();
    pendingMeshUploads_.clear();
    for (chunk::ChunkBuilder& chunk : chunks_) {
        chunk.meshJobInFlight = false;
        chunk.close();
    }
    chunks_.clear();
    sortedChunks_.clear();
    dirtyChunks_.clear();

    if (world == nullptr) {
        return;
    }

    net::minecraft::client::option::GameOptions& opts = activeOptions();
    const option::ResolvedRenderOptions resolved = option::resolve(opts);
    if (Block::LEAVES != nullptr) {
        static_cast<net::minecraft::block::LeavesBlock*>(Block::LEAVES)->setFancyGraphics(
            resolved.fancyLeaves);
    }
    block::BlockRenderManager::fancyGraphics = opts.fancyGraphics;
    block::BlockRenderManager::fancyLeaves = resolved.fancyLeaves;
    lastViewDistance = opts.viewDistance;
    lastRenderScale = opts.ofRenderScale;

    gl::GlExtensions::ensureLoaded();
    // Chunks render exclusively through display lists. The VBO path was removed (it produced
    // corrupted geometry + heavy lag); reintroduce it as a separate, verified renderer later.
    if (chunkGlList == 0) {
        constexpr int kReserveLists = 64 * 64 * 64 * 2;
        chunkGlList = gl::GL11::glGenLists(kReserveLists);
    }

    const int renderDistance = resolved.chunkGridRadius;

    chunkCountX = renderDistance / kChunkSectionSize + 1;
    chunkCountY = kChunkSectionCountY;
    chunkCountZ = renderDistance / kChunkSectionSize + 1;

    const std::size_t totalChunks = static_cast<std::size_t>(chunkCountX * chunkCountY * chunkCountZ);
    chunks_.reserve(totalChunks);
    sortedChunks_.reserve(totalChunks);
    chunksInCurrentLayer_.reserve(totalChunks);

    minChunkX = 0;
    minChunkY = 0;
    minChunkZ = 0;
    maxChunkX = chunkCountX;
    maxChunkY = chunkCountY;
    maxChunkZ = chunkCountZ;

    dirtyChunks_.clear();
    globalBlockEntities.clear();

    for (chunk::ChunkBuilder& chunk : chunks_) {
        chunk.queuedForRebuild = false;
    }

    int nextId = 0;
    int glListOffset = 0;
    for (int chunkZ = 0; chunkZ < chunkCountZ; ++chunkZ) {
        for (int chunkY = 0; chunkY < chunkCountY; ++chunkY) {
            for (int chunkX = 0; chunkX < chunkCountX; ++chunkX) {
                const int baseRenderList = chunkGlList + glListOffset;
                chunks_.emplace_back(world, globalBlockEntities, chunkX * kChunkSectionSize, chunkY * kChunkSectionSize,
                    chunkZ * kChunkSectionSize, kChunkSectionSize, baseRenderList);
                chunk::ChunkBuilder& builder = chunks_.back();
                builder.inFrustum = true;
                builder.id = nextId++;
                builder.invalidate();
                sortedChunks_.push_back(&builder);
                enqueueDirtyChunk(&builder);
                glListOffset += 2;
            }
        }
    }

    const net::minecraft::LivingEntity* sortCamera = resolveSortCamera();
    if (sortCamera != nullptr) {
        sortChunks(MathHelper::floor(sortCamera->x), MathHelper::floor(sortCamera->y), MathHelper::floor(sortCamera->z));
        std::sort(sortedChunks_.begin(), sortedChunks_.end(), world::DistanceChunkSorter(*sortCamera));
    }

    prevCameraX = -9999.0;
    prevCameraY = -9999.0;
    prevCameraZ = -9999.0;

    entityRenderCooldown = 2;
}

void WorldRenderer::reloadIfViewDistanceChanged()
{
    const net::minecraft::client::option::GameOptions& opts = activeOptions();
    if (opts.viewDistance != lastViewDistance || opts.ofRenderScale != lastRenderScale) {
        reload();
    }
}

void WorldRenderer::sortChunks(int cameraX, int cameraY, int cameraZ)
{
    cameraX -= 8;
    cameraY -= 8; // Matches Java; Y is not used in toroidal repositioning.
    cameraZ -= 8;
    (void)cameraY;

    minChunkX = std::numeric_limits<int>::max();
    minChunkY = std::numeric_limits<int>::max();
    minChunkZ = std::numeric_limits<int>::max();
    maxChunkX = std::numeric_limits<int>::min();
    maxChunkY = std::numeric_limits<int>::min();
    maxChunkZ = std::numeric_limits<int>::min();

    const int span = chunkCountX * kChunkSectionSize;
    const int halfSpan = span / 2;

    for (int chunkX = 0; chunkX < chunkCountX; ++chunkX) {
        int blockX = chunkX * kChunkSectionSize;
        blockX -= MathHelper::floorDiv(blockX + halfSpan - cameraX, span) * span;
        if (blockX < minChunkX) {
            minChunkX = blockX;
        }
        if (blockX > maxChunkX) {
            maxChunkX = blockX;
        }

        for (int chunkZ = 0; chunkZ < chunkCountZ; ++chunkZ) {
            int blockZ = chunkZ * kChunkSectionSize;
            blockZ -= MathHelper::floorDiv(blockZ + halfSpan - cameraZ, span) * span;
            if (blockZ < minChunkZ) {
                minChunkZ = blockZ;
            }
            if (blockZ > maxChunkZ) {
                maxChunkZ = blockZ;
            }

            for (int chunkY = 0; chunkY < chunkCountY; ++chunkY) {
                const int blockY = chunkY * kChunkSectionSize;
                if (blockY < minChunkY) {
                    minChunkY = blockY;
                }
                if (blockY > maxChunkY) {
                    maxChunkY = blockY;
                }

                chunk::ChunkBuilder* builder = chunkAt(chunkX, chunkY, chunkZ);
                if (builder == nullptr) {
                    continue;
                }
                const bool wasDirty = builder->dirty;
                builder->setPosition(blockX, blockY, blockZ);
                if (wasDirty || !builder->dirty) {
                    continue;
                }
                enqueueDirtyChunk(builder);
            }
        }
    }
}

void WorldRenderer::beginWorldRenderFrame() noexcept
{
    framePhase_ = pipeline::FramePhase::Idle;
}

void WorldRenderer::expectFramePhase(const pipeline::FramePhase required) const
{
#ifndef NDEBUG
    assert(framePhase_ == required);
#else
    (void)required;
#endif
}

void WorldRenderer::advanceFramePhase(const pipeline::FramePhase next) noexcept
{
    framePhase_ = next;
}

void WorldRenderer::sortChunksOnMove(const net::minecraft::LivingEntity& camera)
{
    const double cameraDeltaX = camera.x - prevCameraX;
    const double cameraDeltaY = camera.y - prevCameraY;
    const double cameraDeltaZ = camera.z - prevCameraZ;
    if (cameraDeltaX * cameraDeltaX + cameraDeltaY * cameraDeltaY + cameraDeltaZ * cameraDeltaZ
        <= pipeline::kCameraResortDistanceSq) {
        return;
    }

    prevCameraX = camera.x;
    prevCameraY = camera.y;
    prevCameraZ = camera.z;
    sortChunks(net::minecraft::util::math::MathHelper::floor(camera.x),
        net::minecraft::util::math::MathHelper::floor(camera.y),
        net::minecraft::util::math::MathHelper::floor(camera.z));
    std::sort(sortedChunks_.begin(), sortedChunks_.end(), world::DistanceChunkSorter(camera));
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
        expectFramePhase(pipeline::FramePhase::Compiled);
    } else {
        expectFramePhase(pipeline::FramePhase::SolidLayerDone);
    }
    const int rendered = internal::WorldRendererCore::render(*this, camera, layer, tickDelta);
    if (layer == 0) {
        advanceFramePhase(pipeline::FramePhase::SolidLayerDone);
    }
    return rendered;
}

bool WorldRenderer::compileChunks(net::minecraft::LivingEntity& camera, bool force)
{
    expectFramePhase(pipeline::FramePhase::Culled);
    const bool finished = internal::WorldRendererCore::compileChunks(*this, camera, force);
    advanceFramePhase(pipeline::FramePhase::Compiled);
    return finished;
}

void WorldRenderer::renderEntities(const Vec3d& cameraPos, Culler* culler, float tickDelta)
{
    internal::WorldRendererCore::renderEntities(*this, cameraPos, culler, tickDelta);
}

void WorldRenderer::cullChunks(Culler* culler, float tickDelta)
{
    (void)tickDelta;
    expectFramePhase(pipeline::FramePhase::Idle);
    lastCuller_ = culler;
    if (culler == nullptr || !activeOptions().frustumCulling) {
        for (chunk::ChunkBuilder& chunk : chunks_) {
            chunk.inFrustum = true;
        }
    } else {
        for (std::size_t i = 0; i < chunks_.size(); ++i) {
            chunk::ChunkBuilder& chunk = chunks_[i];
            if (chunk.hasNoGeometry()) {
                continue;
            }
            if (chunk.inFrustum && static_cast<int>((i + cullStep) & 0xF) != 0) {
                continue;
            }
            chunk.updateFrustum(*culler);
        }
        ++cullStep;
    }
    advanceFramePhase(pipeline::FramePhase::Culled);
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
        const int localX = MathHelper::floorMod(chunkX, chunkCountX);
        for (int chunkY = startY; chunkY <= endY; ++chunkY) {
            const int localY = MathHelper::floorMod(chunkY, chunkCountY);
            for (int chunkZ = startZ; chunkZ <= endZ; ++chunkZ) {
                const int localZ = MathHelper::floorMod(chunkZ, chunkCountZ);
                chunk::ChunkBuilder* builder = chunkAt(localX, localY, localZ);
                if (builder == nullptr || builder->dirty) {
                    continue;
                }
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

void WorldRenderer::playSound(const std::string& sound, double x, double y, double z, float volume, float pitch)
{
    if (client == nullptr) {
        return;
    }
    net::minecraft::Entity* camera = cameraEntity_ != nullptr ? cameraEntity_ : client->camera;
    if (camera == nullptr) {
        return;
    }
    float range = 16.0f;
    if (volume > 1.0f) {
        range *= volume;
    }
    if (camera->getSquaredDistance(x, y, z) > static_cast<double>(range * range)) {
        return;
    }
    client->audio.playAt(sound, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), volume, pitch);
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
    World* world = client->world;
    if (particle == "bubble") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::WaterBubbleParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "smoke") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::FireSmokeParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "explode") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::ExplosionParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "flame") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::FlameParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "splash") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::WaterSplashParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "largesmoke") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::FireSmokeParticle(world, x, y, z, velocityX, velocityY, velocityZ, 2.5f));
    } else if (particle == "reddust") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::RedDustParticle(
            world, x, y, z, static_cast<float>(velocityX), static_cast<float>(velocityY), static_cast<float>(velocityZ)));
    } else if (particle == "note") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::NoteParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "lava") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::LavaEmberParticle(world, x, y, z));
    } else if (particle == "heart") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::HeartParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "portal") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::PortalParticle(world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "footstep") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::FootstepParticle(
            textureManager, world, x, y, z));
    } else if (particle == "snowballpoof") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::ItemParticle(
            world, x, y, z, Item::SNOWBALL));
    } else if (particle == "snowshovel") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::SnowParticle(
            world, x, y, z, velocityX, velocityY, velocityZ));
    } else if (particle == "slime") {
        client->particleManager.addParticle(new ::net::minecraft::client::particle::ItemParticle(
            world, x, y, z, Item::SLIMEBALL));
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
    for (chunk::ChunkBuilder& chunk : chunks_) {
        if (!chunk.hasSkyLight || chunk.dirty) {
            continue;
        }
        chunk.invalidate();
        enqueueDirtyChunk(&chunk);
    }
}

void WorldRenderer::playStreaming(const std::string& stream, int x, int y, int z)
{
    if (client == nullptr) {
        return;
    }
    if (!stream.empty()) {
        client->inGameHud.setRecordPlayingOverlay("C418 - " + stream);
    }
    client->audio.playRecord(stream, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 1.0f);
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

void WorldRenderer::worldEvent(net::minecraft::PlayerEntity* player, int type, int x, int y, int z, int data)
{
    (void)player;
    if (world == nullptr) {
        return;
    }
    JavaRandom& random = world->random();
    switch (type) {
    case 1001:
        playSound("random.click", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), 1.0f, 1.2f);
        break;
    case 1000:
        playSound("random.click", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), 1.0f, 1.0f);
        break;
    case 1002:
        playSound("random.bow", static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), 1.0f, 1.2f);
        break;
    case 2000: {
        const int offsetX = data % 3 - 1;
        const int offsetZ = data / 3 % 3 - 1;
        const double baseX = static_cast<double>(x) + static_cast<double>(offsetX) * 0.6 + 0.5;
        const double baseY = static_cast<double>(y) + 0.5;
        const double baseZ = static_cast<double>(z) + static_cast<double>(offsetZ) * 0.6 + 0.5;
        for (int i = 0; i < 10; ++i) {
            const double speed = random.nextDouble() * 0.2 + 0.01;
            const double px = baseX + static_cast<double>(offsetX) * 0.01 + (random.nextDouble() - 0.5) * offsetZ * 0.5;
            const double py = baseY + (random.nextDouble() - 0.5) * 0.5;
            const double pz = baseZ + static_cast<double>(offsetZ) * 0.01 + (random.nextDouble() - 0.5) * offsetX * 0.5;
            const double vx = static_cast<double>(offsetX) * speed + random.nextGaussian() * 0.01;
            const double vy = -0.03 + random.nextGaussian() * 0.01;
            const double vz = static_cast<double>(offsetZ) * speed + random.nextGaussian() * 0.01;
            addParticle("smoke", px, py, pz, vx, vy, vz);
        }
        break;
    }
    case 2001: {
        const int blockId = data & 0xFF;
        const int blockMeta = (data >> 8) & 0xFF;
        if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[blockId] != nullptr && client != nullptr) {
            Block* block = Block::BLOCKS[blockId];
            net::minecraft::BlockSoundGroup* group = block->soundGroup != nullptr ? block->soundGroup : &Block::DEFAULT_SOUND_GROUP;
            client->audio.playAt(
                group->getBreakSound(),
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(z) + 0.5f,
                (group->getVolume() + 1.0f) / 2.0f,
                group->getPitch() * 0.8f);
        }
        if (client != nullptr) {
            client->particleManager.addBlockBreakParticles(x, y, z, blockId, blockMeta);
        }
        break;
    }
    case 1003:
        if (mathRandom().nextDouble() < 0.5) {
            playSound("random.door_open", static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
                static_cast<double>(z) + 0.5, 1.0f, random.nextFloat() * 0.1f + 0.9f);
        } else {
            playSound("random.door_close", static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
                static_cast<double>(z) + 0.5, 1.0f, random.nextFloat() * 0.1f + 0.9f);
        }
        break;
    case 1004:
        playSound("random.fizz", static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
            static_cast<double>(z) + 0.5, 0.5f, 2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f);
        break;
    case 1005: {
        if (data >= 0 && data < static_cast<int>(Item::ITEM_COUNT)) {
            Item* item = Item::ITEMS[static_cast<std::size_t>(data)];
            if (auto* disc = dynamic_cast<::net::minecraft::item::MusicDiscItem*>(item)) {
                playStreaming(disc->sound, x, y, z);
                break;
            }
        }
        playStreaming("", x, y, z);
        break;
    }
    default:
        break;
    }
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
    if (chunkGlList > 0) {
        gl::GL11::glDeleteLists(chunkGlList, 64 * 64 * 64 * 2);
        chunkGlList = 0;
    }
}

} // namespace net::minecraft::client::render
