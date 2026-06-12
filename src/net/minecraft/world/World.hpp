#pragma once

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/entity/ai/pathing/Path.hpp"
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
#include "net/minecraft/world/light/LightUpdate.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
#include "net/minecraft/world/storage/SavedDataStorage.hpp"
#include "net/minecraft/world/tick/ScheduledTickQueue.hpp"
#include "net/minecraft/world/weather/WorldWeather.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
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
class BiomeDefinition;

class World : public IEntityWorld {
public:
    World(std::string name, std::uint64_t seed);

    World(WorldStorage* dimensionData, const std::string& name, std::int64_t seed, bool deferSpawnInit = false);

    // Faithful to Java World(World world, Dimension dimension) — dimension switch.
    World(World* parentWorld, std::unique_ptr<Dimension> dimensionIn);

    // Joins the lighting thread before members (and the chunks they own) die.
    ~World() override;

    bool isRemote_ = false;
    bool pauseTicking = false;
    int difficulty = 2;
    int ambientDarkness = 0;
    SavedDataStorage persistentStateManager {nullptr};
    std::unique_ptr<Dimension> dimension = Dimension::fromId(0);

    // Construction-time flag: true for a freshly created (not loaded) world.
    [[nodiscard]] bool isNewWorld() const noexcept { return newWorld; }

    // Whether world events (block-place fan-out, neighbor updates) are processed.
    [[nodiscard]] bool isEventProcessingEnabled() const noexcept { return eventProcessingEnabled; }
    void setEventProcessingEnabled(bool value) noexcept { eventProcessingEnabled = value; }

    // Faithful to Java World.instantBlockUpdateEnabled: when true, fluid blocks
    // flow instantly during world gen (set by spring features around onTick).
    void setInstantBlockUpdate(bool value) noexcept { instantBlockUpdateEnabled = value; }

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

    [[nodiscard]] float getTime(float partialTicks) const;

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
    [[nodiscard]] Vec3d getCloudColor(float partialTicks) const;
    [[nodiscard]] Vec3d getFogColor(float partialTicks) const;

    [[nodiscard]] float calculateSkyLightIntensity(float partialTicks) const;
    [[nodiscard]] int getTopSolidBlockY(int x, int z) const;
    [[nodiscard]] const BiomeDefinition& getBiomeDefinition(int x, int z) const;
    [[nodiscard]] bool isMaterialInBox(const Box& boundingBox, block::material::Material& material) const;
    [[nodiscard]] bool isFluidInBox(const Box& boundingBox, block::material::Material& fluid) const;
    [[nodiscard]] bool isBoxSubmergedInFluid(const Box& box) const;
    [[nodiscard]] bool isFireOrLavaInBox(const Box& box) const;
    [[nodiscard]] bool updateMovementInFluid(const Box& entityBoundingBox, block::material::Material& fluidMaterial, Entity* entity);
    void extinguishFire(PlayerEntity* player, int x, int y, int z, int direction);
    [[nodiscard]] bool isRegionLoaded(int x, int y, int z, int range) const;

    [[nodiscard]] bool isRegionLoaded(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const;
    [[nodiscard]] int getLightLevel(int x, int y, int z) const override;

    [[nodiscard]] int getLightLevel(int x, int y, int z, bool useNeighborLight) const;
    [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override;

    [[nodiscard]] float getLightBrightness(int x, int y, int z) const override;
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
    [[nodiscard]] entity::LivingEntity* spawnMob(
        const std::string& entityType,
        const std::function<bool(entity::LivingEntity&)>& setup = {});
    [[nodiscard]] entity::LivingEntity* spawnMob(
        const std::string& entityType,
        double x,
        double y,
        double z,
        float yaw = 0.0f,
        float pitch = 0.0f);
    [[nodiscard]] bool spawnMob(entity::LivingEntity* mob);

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
    // Pump lighting-thread results: forward finished dirty regions to the
    // renderer. Cheap; call once per frame/tick. Returns true while the
    // lighting thread still has work queued.
    bool doLightingUpdates(std::size_t maxDirtyRegions = 128);
    // Block until the lighting thread has fully converged (world load).
    void finishLightingUpdates();
    [[nodiscard]] bool hasPendingLightingUpdates() const noexcept
    {
        return lighting_.busy();
    }

    void registerChunkForLighting(Chunk* chunk) { lighting_.registerChunk(chunk); }
    void unregisterChunkForLighting(Chunk* chunk) { lighting_.unregisterChunk(chunk); }

    void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge);
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

    [[nodiscard]] std::vector<Entity*> getEntities(Entity* except, const Box& box);

    [[nodiscard]] std::vector<Box> getEntityCollisions(Entity* except, const Box& box);

    [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override;

    [[nodiscard]] BiomeSource* getBiomeSource() const override
    {
        if (dimension == nullptr || !dimension->biomeSource) {
            return nullptr;
        }
        return dimension->biomeSource.get();
    }

    [[nodiscard]] Chunk& ensureChunk(int blockX, int blockZ);

    [[nodiscard]] const Chunk* getChunkIfLoaded(int blockX, int blockZ) const;

    [[nodiscard]] Chunk* getChunkIfLoaded(int blockX, int blockZ);

    [[nodiscard]] const Chunk& getChunk(int chunkX, int chunkZ) const;

    [[nodiscard]] Chunk& getChunk(int chunkX, int chunkZ);

    [[nodiscard]] int getBlockId(int x, int y, int z) const override;

    [[nodiscard]] int getBlockMeta(int x, int y, int z) const override;

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

    [[nodiscard]] bool hasChunk(int chunkX, int chunkZ) const;

    [[nodiscard]] Chunk& getChunkFromPos(int x, int z);

    [[nodiscard]] bool isAir(int x, int y, int z) const override;

    [[nodiscard]] block::material::Material& getMaterial(int x, int y, int z) const override;

    [[nodiscard]] int getSpawnBlockId(int x, int z) const;

    void updateSpawnPosition();
    void updateEntityLists();

    [[nodiscard]] bool isSolidBlock(int x, int y, int z) const;

    [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override;

    [[nodiscard]] int getTopY(int x, int z) const;

    [[nodiscard]] int getSpawnPositionValidityY(int x, int z);

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
        weather_.setRainGradient(value);
    }

    [[nodiscard]] WorldWeather& weather() noexcept { return weather_; }
    [[nodiscard]] const WorldWeather& weather() const noexcept { return weather_; }

    [[nodiscard]] int getBrightness(int x, int y, int z) const override;

    [[nodiscard]] int getBrightness(int x, int y, int z, bool useNeighborLight) const;

    // Faithful to Java World.getBrightness(LightType, x, y, z).
    [[nodiscard]] int getBrightness(LightType type, int x, int y, int z) const;

    [[nodiscard]] bool hasSkyLight(int x, int y, int z) const override;

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

    void loadSpawnChunks(int chunkRadius = 8);

    [[nodiscard]] std::size_t chunkCount() const noexcept;

    [[nodiscard]] const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() const noexcept;

    [[nodiscard]] std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() noexcept;

    [[nodiscard]] std::string describe() const;

    [[nodiscard]] std::vector<std::uint8_t> getChunkData(int x, int y, int z, int sizeX, int sizeY, int sizeZ);

    [[nodiscard]] double getTemperature(int x, int z) const;

    [[nodiscard]] double getDownfall(int x, int z) const;

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
    LightingEngine lighting_;
    int saveInterval_ = 40;
    int chunkPreloadRadius_ = 2;
    WorldWeather weather_;
    int clientTimeMode_ = 0;
    WorldProperties properties_ {};
    bool hasStorageBackedProperties_ = false;
    std::unique_ptr<ChunkSource> chunkCache_;

    void setChunkCache(std::unique_ptr<ChunkSource> cache)
    {
        chunkCache_ = std::move(cache);
    }


    std::vector<Entity*> entities_;
    ScheduledTickQueue scheduledTicks_;
    int lcgBlockSeed_ = 0;
    int ambientSoundCounter_ = 0;
    std::unordered_set<ChunkPos, ChunkPosHash> activeChunks_;
    std::unique_ptr<ChunkSource> chunkGeneratorSource_;

private:
    void updateEntity(Entity* entity, bool requireLoaded, int depth);

    // Mode flags — exposed through accessors above; not for direct external poke.
    bool newWorld = false;
    bool eventProcessingEnabled = false;
    bool instantBlockUpdateEnabled = false;
    // Internal spawn/time/sleep state — World-only.
    bool allowMonsterSpawning = true;
    bool allowMobSpawning = true;
    std::uint64_t worldTimeMask = 0xFFFFFFULL;
    bool allPlayersSleeping = false;

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
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> chunks_;
};

} // namespace net::minecraft
