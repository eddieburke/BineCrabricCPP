#pragma once

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/ai/pathing/Path.hpp"
#include "net/minecraft/entity/SpawnGroup.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockEvent.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/events/WorldEvents.hpp"
#include "net/minecraft/world/mutation/BlockMutationModule.hpp"
#include "net/minecraft/world/ports/IEntityWorld.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#include "net/minecraft/world/chunk/light/LightUpdate.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
#include "net/minecraft/world/storage/SavedDataStorage.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net::minecraft::client::gui::screen {
class LoadingDisplay;
}

namespace net::minecraft {

class GameEventListener;
class WorldStorage;

class World : public IEntityWorld {
public:
    World(std::string name, std::uint64_t seed);

    World(WorldStorage* dimensionData, const std::string& name, std::int64_t seed, bool deferSpawnInit = false);

    // Faithful to Java World(World world, Dimension dimension) — dimension switch.
    World(World* parentWorld, std::unique_ptr<Dimension> dimensionIn);

    bool isRemote_ = false;
    bool newWorld = false;
    bool eventProcessingEnabled = false;
    bool pauseTicking = false;
    // Faithful to Java World.instantBlockUpdateEnabled: when true, fluid blocks
    // flow instantly during world gen (set by spring features around onTick).
    bool instantBlockUpdateEnabled = false;
    int difficulty = 2;
    int ambientDarkness = 0;
    bool allowMonsterSpawning = true;
    bool allowMobSpawning = true;
    float rainGradientPrev = 0.0f;
    float rainGradient = 0.0f;
    float thunderGradientPrev = 0.0f;
    float thunderGradient = 0.0f;
    std::uint64_t worldTimeMask = 0xFFFFFFULL;
    int lightningTicksLeft = 0;
    bool allPlayersSleeping = false;
    SavedDataStorage persistentStateManager {nullptr};
    std::unique_ptr<Dimension> dimension = Dimension::fromId(0);

    // Java exposes world.random publicly; Block/Item behavior reads from it.
    [[nodiscard]] JavaRandom& random() noexcept override { return random_; }

    [[nodiscard]] bool isRemote() const override;

    [[nodiscard]] const std::string& name() const noexcept
    {
        return name_;
    }

    [[nodiscard]] std::uint64_t seed() const noexcept
    {
        return seed_;
    }

    [[nodiscard]] std::uint64_t getSeed() const noexcept
    {
        return hasStorageBackedProperties_ ? properties_.getSeed() : seed_;
    }

    [[nodiscard]] std::uint64_t getTime() const noexcept
    {
        return time_;
    }

    [[nodiscard]] float getTime(float partialTicks) const
    {
        if (!isRemote_ && clientTimeMode_ != 0 && dimension) {
            const long long baseTime = clientTimeMode_ == 1 ? 6000LL : 18000LL;
            return dimension->getTimeOfDay(baseTime, partialTicks);
        }
        if (dimension) {
            return dimension->getTimeOfDay(static_cast<long long>(time_), partialTicks);
        }
        return 0.0f;
    }

    void applyWorldSettings(bool weatherEnabled, int autoSaveTicks, int timeMode);
    void setChunkPreloadRadius(int radius);

    [[nodiscard]] std::uint64_t time() const noexcept
    {
        return time_;
    }

    void setTime(std::uint64_t value) noexcept
    {
        time_ = value;
        if (hasStorageBackedProperties_) {
            properties_.setTime(value);
        }
    }

    [[nodiscard]] Vec3i getSpawnPos() const
    {
        return spawnPos_;
    }

    void setSpawnPos(const Vec3i& pos)
    {
        spawnPos_ = pos;
        if (hasStorageBackedProperties_) {
            properties_.setSpawn(pos.x, pos.y, pos.z);
        }
    }

    [[nodiscard]] int getAmbientDarkness(float partialTicks) const;
    [[nodiscard]] float getRainGradient(float partialTicks) const;
    [[nodiscard]] float getThunderGradient(float partialTicks) const;
    [[nodiscard]] Vec3d getSkyColor(Entity* entity, float partialTicks) const;
    [[nodiscard]] Vec3d getCloudColor(float partialTicks) const
    {
        float time = getTime(partialTicks);
        float brightness = MathHelper::cos(time * 3.14159265f * 2.0f) * 2.0f + 0.5f;
        if (brightness < 0.0f) {
            brightness = 0.0f;
        }
        if (brightness > 1.0f) {
            brightness = 1.0f;
        }
        float red = static_cast<float>((worldTimeMask >> 16) & 0xFFULL) / 255.0f;
        float green = static_cast<float>((worldTimeMask >> 8) & 0xFFULL) / 255.0f;
        float blue = static_cast<float>(worldTimeMask & 0xFFULL) / 255.0f;
        const float rain = getRainGradient(partialTicks);
        if (rain > 0.0f) {
            const float gray = (red * 0.3f + green * 0.59f + blue * 0.11f) * 0.6f;
            const float blend = 1.0f - rain * 0.95f;
            red = red * blend + gray * (1.0f - blend);
            green = green * blend + gray * (1.0f - blend);
            blue = blue * blend + gray * (1.0f - blend);
        }
        red *= brightness * 0.9f + 0.1f;
        green *= brightness * 0.9f + 0.1f;
        blue *= brightness * 0.85f + 0.15f;
        const float thunder = getThunderGradient(partialTicks);
        if (thunder > 0.0f) {
            const float gray = (red * 0.3f + green * 0.59f + blue * 0.11f) * 0.2f;
            const float blend = 1.0f - thunder * 0.95f;
            red = red * blend + gray * (1.0f - blend);
            green = green * blend + gray * (1.0f - blend);
            blue = blue * blend + gray * (1.0f - blend);
        }
        return {red, green, blue};
    }
    [[nodiscard]] Vec3d getFogColor(float partialTicks) const
    {
        if (dimension) {
            return dimension->getFogColor(getTime(partialTicks), partialTicks);
        }
        return {0.75, 0.85, 1.0};
    }

    [[nodiscard]] float calculateSkyLightIntensity(float partialTicks) const
    {
        float value = 1.0f - (MathHelper::cos(getTime(partialTicks) * 3.14159265f * 2.0f) * 2.0f + 0.75f);
        if (value < 0.0f) {
            value = 0.0f;
        }
        if (value > 1.0f) {
            value = 1.0f;
        }
        return value * value * 0.5f;
    }
    [[nodiscard]] int getTopSolidBlockY(int x, int z) const;
    [[nodiscard]] bool isMaterialInBox(const Box& boundingBox, block::material::Material& material) const;
    [[nodiscard]] bool isFluidInBox(const Box& boundingBox, block::material::Material& fluid) const;
    [[nodiscard]] bool isBoxSubmergedInFluid(const Box& box) const;
    [[nodiscard]] bool isFireOrLavaInBox(const Box& box) const;
    [[nodiscard]] bool updateMovementInFluid(const Box& entityBoundingBox, block::material::Material& fluidMaterial, Entity* entity);
    void extinguishFire(PlayerEntity* player, int x, int y, int z, int direction);
    [[nodiscard]] bool isRegionLoaded(int x, int y, int z, int range) const
    {
        return isRegionLoaded(x - range, y - range, z - range, x + range, y + range, z + range);
    }

    [[nodiscard]] bool isRegionLoaded(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const
    {
        if (maxY < 0 || minY >= Chunk::height) {
            return false;
        }
        minX >>= 4;
        minY >>= 4;
        minZ >>= 4;
        maxX >>= 4;
        maxY >>= 4;
        maxZ >>= 4;
        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (getChunkIfLoaded(x << 4, z << 4) == nullptr) {
                    return false;
                }
            }
        }
        return true;
    }
    [[nodiscard]] int getLightLevel(int x, int y, int z) const override
    {
        return getLightLevel(x, y, z, true);
    }

    [[nodiscard]] int getLightLevel(int x, int y, int z, bool useNeighborLight) const
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 15;
        }
        if (useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
            int brightness = getLightLevel(x, y + 1, z, false);
            brightness = std::max(brightness, getLightLevel(x + 1, y, z, false));
            brightness = std::max(brightness, getLightLevel(x - 1, y, z, false));
            brightness = std::max(brightness, getLightLevel(x, y, z + 1, false));
            brightness = std::max(brightness, getLightLevel(x, y, z - 1, false));
            return brightness;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= Chunk::height) {
            const int skyBrightness = 15 - ambientDarkness;
            return skyBrightness < 0 ? 0 : skyBrightness;
        }
        const Chunk& chunk = getChunk(chunk_coord(x), chunk_coord(z));
        return chunk.getLight(mod_16(x), y, mod_16(z), ambientDarkness);
    }
    [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override
    {
        int brightness = getLightLevel(x, y, z);
        if (brightness < blockLight) {
            brightness = blockLight;
        }
        if (dimension == nullptr) {
            return Dimension::luminanceForLightLevel(brightness);
        }
        return dimension->lightLevelToLuminance[static_cast<std::size_t>(brightness)];
    }

    [[nodiscard]] float getLightBrightness(int x, int y, int z) const override
    {
        if (dimension == nullptr) {
            return Dimension::luminanceForLightLevel(getLightLevel(x, y, z));
        }
        return dimension->lightLevelToLuminance[static_cast<std::size_t>(getLightLevel(x, y, z))];
    }
    [[nodiscard]] std::optional<HitResult> raycast(const Vec3d& start, const Vec3d& end, bool ignoreLiquids = false,
        bool ignoreNonFullBlocks = false) const;
    void updateSkyBrightness();
    void prepareWeather();
    void saveLevelProperties();
    void save();
    void checkSessionLock();

    void allowSpawning(bool allowMonsters, bool allowMobs)
    {
        allowMonsterSpawning = allowMonsters;
        allowMobSpawning = allowMobs;
    }

    [[nodiscard]] int countEntities(const std::string& entityType) const;
    [[nodiscard]] int countSpawnGroup(entity::SpawnGroupKind kind) const;

    bool attemptSaving(int step);
    void updateEntity(Entity* entity) { updateEntity(entity, true, 0); }
    void updateEntity(Entity* entity, bool requireLoaded) { updateEntity(entity, requireLoaded, 0); }
    void addEntities(std::vector<Entity*>& entities);
    void unloadEntities(std::vector<Entity*>& entities);
    void processBlockUpdates(const std::vector<block::entity::BlockEntity*>& blockUpdates);
    void updateSleepingPlayers();
    [[nodiscard]] bool canSkipNight();
    void tickEntities();
    void displayTick(int x, int y, int z);
    void loadChunksNearEntity(Entity* entity);

    [[nodiscard]] ChunkSource* getChunkSource() noexcept { return chunkCache_.get(); }
    void savingProgress(client::gui::screen::LoadingDisplay* display);
    bool doLightingUpdates();
    [[nodiscard]] bool hasPendingLightingUpdates() const noexcept
    {
        return !lightingQueue_.empty();
    }

    [[nodiscard]] bool hasPendingLightingUpdates(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const
    {
        for (const LightUpdate& update : lightingQueue_) {
            if (update.overlapsRegion(minX, minY, minZ, maxX, maxY, maxZ)) {
                return true;
            }
        }
        return false;
    }

    void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge);
    void setLight(LightType lightType, int x, int y, int z, int value);
    void updateLight(LightType lightType, int x, int y, int z, int lightLevel);
    [[nodiscard]] bool isTopY(int x, int y, int z) const;
    void addPlayer(PlayerEntity* player);
    virtual void updateWeatherCycles();
    void tickChunks();

    virtual void tick();

    virtual void scheduleBlockUpdate(int x, int y, int z, int id, int tickRate) override;
    virtual bool processScheduledTicks(bool flush);
    bool spawnEntity(Entity* entity) override;
    virtual bool spawnGlobalEntity(Entity* entity);
    virtual void notifyEntityAdded(Entity* entity);
    virtual void notifyEntityRemoved(Entity* entity);
    virtual void remove(Entity* entity);
    void serverRemove(Entity* entity);

    [[nodiscard]] const std::vector<Entity*>& entities() const noexcept
    {
        return entities_;
    }

    [[nodiscard]] std::vector<Entity*> getEntities(Entity* except, const Box& box)
    {
        std::vector<Entity*> result;
        for (Entity* entity : entities_) {
            if (entity == nullptr || entity == except || entity->dead) {
                continue;
            }
            if (entity->boundingBox.intersects(box)) {
                result.push_back(entity);
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Box> getEntityCollisions(Entity* except, const Box& box)
    {
        std::vector<Box> result;
        const int minX = MathHelper::floor(box.minX);
        const int maxX = MathHelper::floor(box.maxX + 1.0);
        const int minY = MathHelper::floor(box.minY);
        const int maxY = MathHelper::floor(box.maxY + 1.0);
        const int minZ = MathHelper::floor(box.minZ);
        const int maxZ = MathHelper::floor(box.maxZ + 1.0);

        for (int x = minX; x < maxX; ++x) {
            for (int z = minZ; z < maxZ; ++z) {
                if (!isPosLoaded(x, 64, z)) {
                    continue;
                }
                for (int y = minY - 1; y < maxY; ++y) {
                    const int blockId = getBlockId(x, y, z);
                    if (blockId <= 0 || blockId >= static_cast<int>(Block::BLOCK_COUNT)) {
                        continue;
                    }
                    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                    if (block == nullptr) {
                        continue;
                    }
                    block->addIntersectingBoundingBox(this, x, y, z, box, result);
                }
            }
        }

        for (Entity* other : getEntities(except, box.expand(0.25))) {
            if (other == nullptr) {
                continue;
            }
            if (const std::optional<Box> otherBox = other->getBoundingBox()) {
                if (otherBox->intersects(box)) {
                    result.push_back(*otherBox);
                }
            }
            if (except != nullptr) {
                if (const std::optional<Box> collisionShape = except->getCollisionAgainstShape(other)) {
                    if (collisionShape->intersects(box)) {
                        result.push_back(*collisionShape);
                    }
                }
            }
        }
        return result;
    }

    [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override
    {
        const int id = getBlockId(x, y, z);
        if (id == 0) {
            return false;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
        if (block == nullptr) {
            return false;
        }
        return block->material.suffocates() && block->isFullCube();
    }

    [[nodiscard]] BiomeSource* getBiomeSource() const override
    {
        if (dimension == nullptr || !dimension->biomeSource) {
            return nullptr;
        }
        return dimension->biomeSource.get();
    }

    [[nodiscard]] Chunk& ensureChunk(int blockX, int blockZ)
    {
        if (chunkCache_ != nullptr) {
            return chunkCache_->getChunk(chunk_coord(blockX), chunk_coord(blockZ));
        }
        const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
        auto it = chunks_.find(pos);
        if (it == chunks_.end()) {
            auto [inserted, _] = chunks_.emplace(pos, chunkGenerator_.loadChunk(nullptr, pos.x, pos.z));
            it = inserted;
        }
        return it->second;
    }

    [[nodiscard]] const Chunk* getChunkIfLoaded(int blockX, int blockZ) const
    {
        if (chunkCache_ != nullptr) {
            const int chunkX = chunk_coord(blockX);
            const int chunkZ = chunk_coord(blockZ);
            return chunkCache_->isChunkLoaded(chunkX, chunkZ) ? &chunkCache_->getChunk(chunkX, chunkZ) : nullptr;
        }
        const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
        const auto it = chunks_.find(pos);
        if (it == chunks_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] Chunk* getChunkIfLoaded(int blockX, int blockZ)
    {
        if (chunkCache_ != nullptr) {
            const int chunkX = chunk_coord(blockX);
            const int chunkZ = chunk_coord(blockZ);
            return chunkCache_->isChunkLoaded(chunkX, chunkZ) ? &chunkCache_->getChunk(chunkX, chunkZ) : nullptr;
        }
        const ChunkPos pos{chunk_coord(blockX), chunk_coord(blockZ)};
        const auto it = chunks_.find(pos);
        if (it == chunks_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const Chunk& getChunk(int chunkX, int chunkZ) const
    {
        if (chunkCache_ != nullptr) {
            return chunkCache_->getChunk(chunkX, chunkZ);
        }
        const auto it = chunks_.find(ChunkPos{chunkX, chunkZ});
        if (it == chunks_.end()) {
            throw std::runtime_error("Chunk not loaded");
        }
        return it->second;
    }

    [[nodiscard]] Chunk& getChunk(int chunkX, int chunkZ)
    {
        if (chunkCache_ != nullptr) {
            return chunkCache_->getChunk(chunkX, chunkZ);
        }
        const ChunkPos pos{chunkX, chunkZ};
        auto it = chunks_.find(pos);
        if (it == chunks_.end()) {
            auto [inserted, _] = chunks_.emplace(pos, chunkGenerator_.loadChunk(nullptr, chunkX, chunkZ));
            it = inserted;
        }
        return it->second;
    }

    [[nodiscard]] int getBlockId(int x, int y, int z) const override
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        if (y < 0 || y >= Chunk::height) {
            return 0;
        }
        return getChunk(chunk_coord(x), chunk_coord(z)).getBlockId(mod_16(x), y, mod_16(z));
    }

    [[nodiscard]] int getBlockMeta(int x, int y, int z) const override
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        if (y < 0 || y >= Chunk::height) {
            return 0;
        }
        return getChunk(chunk_coord(x), chunk_coord(z)).getBlockMeta(mod_16(x), y, mod_16(z));
    }

    bool setBlock(int x, int y, int z, int blockId, std::uint8_t meta = 0) override
    {
        return blockMutation_.setBlock(x, y, z, blockId, meta);
    }

    void setBlockMeta(int x, int y, int z, int meta) override
    {
        blockMutation_.setBlockMeta(x, y, z, meta);
    }

    bool setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) override;

    bool setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta = 0) override;

    void addEventListener(GameEventListener* listener);
    void removeEventListener(GameEventListener* listener);
    void blockUpdateEvent(int x, int y, int z);
    void blockUpdate(int x, int y, int z, int blockId);
    void neighborUpdate(int x, int y, int z, int sourceBlockId);
    void setBlockDirty(int x, int y, int z);
    [[nodiscard]] bool canPlace(int blockId, int x, int y, int z, bool fallingBlock, int side);
    [[nodiscard]] bool canSpawnEntity(const Box& box);
    [[nodiscard]] bool canInteract(PlayerEntity* /*user*/, int /*x*/, int /*y*/, int /*z*/) const { return true; }
    void notifyNeighbors(int x, int y, int z, int blockId) override;
    void playSound(double x, double y, double z, const std::string& name, float volume, float pitch);
    void playSound(Entity* source, const std::string& name, float volume, float pitch);
    void playSound(PlayerEntity* player, const std::string& name, float volume, float pitch);
    virtual void broadcastEntityEvent(Entity* entity, std::uint8_t event)
    {
        (void)entity;
        (void)event;
    }
    void playStreaming(const std::string& name, int x, int y, int z);
    void worldEvent(PlayerEntity* player, int type, int x, int y, int z, int data);
    void worldEvent(int type, int x, int y, int z, int data) { worldEvent(nullptr, type, x, y, z, data); }

    Explosion createExplosion(Entity* source, double x, double y, double z, float power);
    Explosion createExplosion(Entity* source, double x, double y, double z, float power, bool fire);
    [[nodiscard]] float getVisibilityRatio(const Vec3d& vec, const Box& box) const;
    [[nodiscard]] bool canTransferPowerInDirection(int x, int y, int z, int direction);
    [[nodiscard]] bool canTransferPower(int x, int y, int z);
    [[nodiscard]] bool isEmittingRedstonePower(int x, int y, int z);
    [[nodiscard]] bool isEmittingRedstonePowerInDirection(int x, int y, int z, int direction);

    void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    void setBlocksDirtyColumn(int x, int z, int minY, int maxY);

    [[nodiscard]] block::entity::BlockEntity* getBlockEntity(int x, int y, int z) override;
    void setBlockEntity(int x, int y, int z, std::unique_ptr<block::entity::BlockEntity> blockEntity);
    void removeBlockEntity(int x, int y, int z);
    void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity);

    [[nodiscard]] entity::ai::pathing::Path findPath(entity::Entity* entity, entity::Entity* target, float range);
    [[nodiscard]] entity::ai::pathing::Path findPath(entity::Entity* entity, int x, int y, int z, float range);

    template <typename StateT>
    StateT* getOrCreateState(const std::string& id)
    {
        PersistentState* existing = persistentStateManager.getOrCreate(typeid(StateT), id);
        if (existing != nullptr) {
            return dynamic_cast<StateT*>(existing);
        }
        auto state = std::make_unique<StateT>(id);
        StateT* pointer = state.get();
        persistentStateManager.set(id, std::move(state));
        return pointer;
    }

    void addParticle(const std::string& name, double px, double py, double pz, double vx, double vy, double vz);
    void notifyEntityPickup(Entity* entity, PlayerEntity* collector);
    void playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch);
    [[nodiscard]] PlayerEntity* getClosestPlayer(double x, double y, double z, double range);
    [[nodiscard]] PlayerEntity* getClosestPlayer(Entity* entity, double range);
    [[nodiscard]] Entity* getClosestPlayerEntity(Entity* entity, double range);
    [[nodiscard]] PlayerEntity* getPlayer(const std::string& name);

    [[nodiscard]] bool isPosLoaded(int x, int y, int z) const;

    void handleChunkDataUpdate(
        int x,
        int y,
        int z,
        int sizeX,
        int sizeY,
        int sizeZ,
        const std::vector<std::uint8_t>& chunkData);

    [[nodiscard]] bool hasChunk(int chunkX, int chunkZ) const
    {
        return getChunkIfLoaded(chunkX << 4, chunkZ << 4) != nullptr;
    }

    [[nodiscard]] Chunk& getChunkFromPos(int x, int z)
    {
        return getChunk(chunk_coord(x), chunk_coord(z));
    }

    [[nodiscard]] bool isAir(int x, int y, int z) const override
    {
        return getBlockId(x, y, z) == 0;
    }

    [[nodiscard]] block::material::Material& getMaterial(int x, int y, int z) const override
    {
        const int blockId = getBlockId(x, y, z);
        if (blockId == 0) {
            return block::material::Material::AIR;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            return block::material::Material::AIR;
        }
        return block->material;
    }

    [[nodiscard]] int getSpawnBlockId(int x, int z) const
    {
        int y = 63;
        while (getBlockId(x, y + 1, z) != 0) {
            ++y;
        }
        return getBlockId(x, y, z);
    }

    void updateSpawnPosition();
    void updateEntityLists();

    [[nodiscard]] bool isSolidBlock(int x, int y, int z) const
    {
        const int id = getBlockId(x, y, z);
        if (id == 0) {
            return false;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
        if (block == nullptr) {
            return false;
        }
        return block->material.blocksMovement();
    }

    [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override
    {
        const int id = getBlockId(x, y, z);
        if (id == 0) {
            return false;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
        return block != nullptr && block->isOpaque();
    }

    [[nodiscard]] int getTopY(int x, int z) const
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        const Chunk* chunk = getChunkIfLoaded(x, z);
        if (chunk == nullptr) {
            return 0;
        }
        return chunk->getHeight(mod_16(x), mod_16(z));
    }

    [[nodiscard]] int getSpawnPositionValidityY(int x, int z)
    {
        Chunk& chunk = getChunkFromPos(x, z);
        const int localX = mod_16(x);
        const int localZ = mod_16(z);
        for (int y = 127; y > 0; --y) {
            const int id = chunk.getBlockId(localX, y, localZ);
            if (id == 0) {
                continue;
            }
            Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
            if (block == nullptr || !block->material.blocksMovement()) {
                continue;
            }
            return y + 1;
        }
        return -1;
    }

    [[nodiscard]] bool canMonsterSpawn() const noexcept
    {
        return ambientDarkness < 4;
    }

    [[nodiscard]] bool isThundering() const
    {
        return static_cast<double>(getThunderGradient(1.0f)) > 0.9;
    }

    [[nodiscard]] bool isRaining() const
    {
        return static_cast<double>(getRainGradient(1.0f)) > 0.2;
    }

    [[nodiscard]] bool isRaining(int x, int y, int z) const override;

    void setRainGradient(float value) noexcept
    {
        rainGradientPrev = rainGradient = value;
    }

    [[nodiscard]] int getBrightness(int x, int y, int z) const override
    {
        return getBrightness(x, y, z, true);
    }

    [[nodiscard]] int getBrightness(int x, int y, int z, bool useNeighborLight) const
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 15;
        }
        if (useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
            int brightness = getBrightness(x, y + 1, z, false);
            brightness = std::max(brightness, getBrightness(x + 1, y, z, false));
            brightness = std::max(brightness, getBrightness(x - 1, y, z, false));
            brightness = std::max(brightness, getBrightness(x, y, z + 1, false));
            brightness = std::max(brightness, getBrightness(x, y, z - 1, false));
            return brightness;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= Chunk::height) {
            y = Chunk::height - 1;
        }
        return getChunk(chunk_coord(x), chunk_coord(z)).getLight(mod_16(x), y, mod_16(z), 0);
    }

    // Faithful to Java World.getBrightness(LightType, x, y, z).
    [[nodiscard]] int getBrightness(LightType type, int x, int y, int z) const
    {
        if (y < 0) {
            y = 0;
        }
        if (y >= Chunk::height) {
            y = Chunk::height - 1;
        }
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return lightValue(type);
        }
        if (getChunkIfLoaded(x, z) == nullptr) {
            return 0;
        }
        return getChunk(chunk_coord(x), chunk_coord(z)).getLight(type, mod_16(x), y, mod_16(z));
    }

    [[nodiscard]] bool hasSkyLight(int x, int y, int z) const override
    {
        return getChunk(chunk_coord(x), chunk_coord(z)).isAboveMaxHeight(mod_16(x), y, mod_16(z));
    }

    [[nodiscard]] WorldStorage* getDimensionData() const noexcept
    {
        return dimensionData_;
    }

    [[nodiscard]] WorldProperties& getProperties() noexcept
    {
        return properties_;
    }

    [[nodiscard]] const WorldProperties& getProperties() const noexcept
    {
        return properties_;
    }

    void loadSpawnChunks(int chunkRadius = 8)
    {
        initializeBlocks();

        for (int cx = -chunkRadius; cx <= chunkRadius; ++cx) {
            for (int cz = -chunkRadius; cz <= chunkRadius; ++cz) {
                [[maybe_unused]] Chunk& chunk = getChunk(cx, cz);
            }
        }
    }

    [[nodiscard]] std::size_t chunkCount() const noexcept
    {
        return chunks_.size();
    }

    [[nodiscard]] const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() const noexcept
    {
        return chunks_;
    }

    [[nodiscard]] std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() noexcept
    {
        return chunks_;
    }

    [[nodiscard]] std::string describe() const
    {
        std::ostringstream out;
        out << "World[name=" << name_ << ", seed=" << seed_ << ", time=" << time_ << ", chunks=" << chunkCount() << "]";
        return out.str();
    }

    [[nodiscard]] std::vector<std::uint8_t> getChunkData(int x, int y, int z, int sizeX, int sizeY, int sizeZ)
    {
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(sizeX * sizeY * sizeZ * 5 / 2));
        const int chunkMinX = x >> 4;
        const int chunkMinZ = z >> 4;
        const int chunkMaxX = (x + sizeX - 1) >> 4;
        const int chunkMaxZ = (z + sizeZ - 1) >> 4;
        std::size_t offset = 0;

        int minY = y;
        int maxY = y + sizeY;
        if (minY < 0) {
            minY = 0;
        }
        if (maxY > Chunk::height) {
            maxY = Chunk::height;
        }

        for (int chunkX = chunkMinX; chunkX <= chunkMaxX; ++chunkX) {
            int minLocalX = x - chunkX * 16;
            int maxLocalX = x + sizeX - chunkX * 16;
            if (minLocalX < 0) {
                minLocalX = 0;
            }
            if (maxLocalX > 16) {
                maxLocalX = 16;
            }
            for (int chunkZ = chunkMinZ; chunkZ <= chunkMaxZ; ++chunkZ) {
                int minLocalZ = z - chunkZ * 16;
                int maxLocalZ = z + sizeZ - chunkZ * 16;
                if (minLocalZ < 0) {
                    minLocalZ = 0;
                }
                if (maxLocalZ > 16) {
                    maxLocalZ = 16;
                }
                offset = getChunk(chunkX, chunkZ).toPacket(bytes, minLocalX, minY, minLocalZ, maxLocalX, maxY, maxLocalZ, offset);
            }
        }

        return bytes;
    }

    [[nodiscard]] double getTemperature(int x, int z) const
    {
        ensureBiomeSample(x, z);
        return biomeSource_.temperatureMap().empty() ? 0.5 : biomeSource_.temperatureMap().front();
    }

    [[nodiscard]] double getDownfall(int x, int z) const
    {
        ensureBiomeSample(x, z);
        return biomeSource_.downfallMap().empty() ? 0.5 : biomeSource_.downfallMap().front();
    }

    void initializeSpawnPoint();

    // Rebinds seed-dependent state for repeated seedfinder probes without reconstructing the world.
    void resetForProbeSeed(std::int64_t seed);

    std::vector<Entity*> globalEntities {};
    std::vector<block::entity::BlockEntity*> blockEntities {};
    std::vector<PlayerEntity*> players {};

protected:
    virtual ChunkSource* createChunkCache();
    virtual void manageChunkUpdatesAndEvents();
    void afterSkipNight();
    void clearWeather();

    bool processingDeferred_ = false;
    std::vector<Entity*> entitiesToUnload_;
    std::vector<block::entity::BlockEntity*> blockEntityUpdateQueue_;
    std::vector<LightUpdate> lightingQueue_;
    int lightingUpdateCount_ = 0;
    int ticksSinceLightning_ = 0;
    int saveInterval_ = 40;
    int chunkPreloadRadius_ = 2;
    bool weatherEnabled_ = true;
    int clientTimeMode_ = 0;
    WorldProperties properties_ {};
    bool hasStorageBackedProperties_ = false;
    std::unique_ptr<ChunkSource> chunkCache_;

    void setChunkCache(std::unique_ptr<ChunkSource> cache)
    {
        chunkCache_ = std::move(cache);
    }


    std::vector<Entity*> entities_;
    std::set<BlockEvent, BlockEventComparator> scheduledUpdates_;
    std::unordered_set<BlockEvent, BlockEventHash> scheduledUpdateSet_;
    int lcgBlockSeed_ = 0;
    int ambientSoundCounter_ = 0;
    std::unordered_set<ChunkPos, ChunkPosHash> activeChunks_;
    static inline int lightingQueueCount_ = 0;
    std::unique_ptr<ChunkSource> chunkGeneratorSource_;

private:
    void updateEntity(Entity* entity, bool requireLoaded, int depth);
    void ensureBiomeSample(int x, int z) const
    {
        biomeSource_.getBiomesInArea(biomeScratch_, x, z, 1, 1);
    }

    WorldEvents events_;
    BlockMutationContext blockMutationContext_;
    BlockMutationModule blockMutation_;

    std::string name_;
    WorldStorage* dimensionData_ = nullptr;
    std::uint64_t seed_ = 0;
    std::uint64_t time_ = 0;
    Vec3i spawnPos_ {8, 64, 8};
    JavaRandom random_;
    OverworldChunkGenerator chunkGenerator_;
    mutable BiomeSource biomeSource_;
    mutable std::vector<BiomeInfo> biomeScratch_;
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> chunks_;
};

} // namespace net::minecraft
