#include "net/minecraft/world/World.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/world/NaturalSpawner.hpp"
#include "net/minecraft/world/WorldRegion.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace net::minecraft {

World::World(std::string name, std::uint64_t seed)
    : events_(*this),
      blockMutationContext_(*this),
      blockMutation_(blockMutationContext_),
      name_(std::move(name)),
      dimensionData_(nullptr),
      seed_(seed),
      time_(0),
      spawnPos_ {8, 64, 8},
      random_(seed),
      chunkGenerator_(nullptr, seed),
      biomeSource_(seed)
{
    initializeBlocks();
    if (dimension != nullptr) {
        dimension->setWorld(this);
        chunkGenerator_.setWorld(this);
    }
    lcgBlockSeed_ = random_.nextInt();
    ambientSoundCounter_ = random_.nextInt(12000);
}

World::World(WorldStorage* dimensionData, const std::string& name, std::int64_t seed, bool deferSpawnInit)
    : events_(*this),
      blockMutationContext_(*this),
      blockMutation_(blockMutationContext_),
      name_(name),
      dimensionData_(dimensionData),
      seed_(static_cast<std::uint64_t>(seed)),
      time_(0),
      spawnPos_ {8, 64, 8},
      random_(static_cast<std::uint64_t>(seed)),
      chunkGenerator_(nullptr, static_cast<std::uint64_t>(seed)),
      biomeSource_(static_cast<std::uint64_t>(seed))
{
    initializeBlocks();
    if (dimensionData_ == nullptr) {
        throw std::runtime_error("World storage is null");
    }

    const std::optional<WorldProperties> loaded = dimensionData_->loadProperties();
    newWorld = !loaded.has_value();
    if (newWorld) {
        properties_ = WorldProperties(static_cast<std::uint64_t>(seed), name);
    } else {
        properties_ = *loaded;
        properties_.setName(name);
    }

    name_ = properties_.getName();
    seed_ = properties_.getSeed();
    random_.setSeed(seed_);
    hasStorageBackedProperties_ = true;

    const int dimensionId = !newWorld && properties_.getDimensionId() == -1 ? -1 : 0;
    dimension = Dimension::fromId(dimensionId);
    if (dimension == nullptr) {
        dimension = Dimension::fromId(0);
    }
    dimension->setWorld(this);
    chunkGenerator_.setWorld(this);

    persistentStateManager = SavedDataStorage(dimensionData_);
    createChunkCache();

    if (newWorld && !deferSpawnInit) {
        initializeSpawnPoint();
    }

    spawnPos_ = Vec3i{properties_.getSpawnX(), properties_.getSpawnY(), properties_.getSpawnZ()};
    time_ = properties_.getTime();
    updateSkyBrightness();
    prepareWeather();
    lcgBlockSeed_ = random_.nextInt();
    ambientSoundCounter_ = random_.nextInt(12000);
}

World::World(World* parentWorld, std::unique_ptr<Dimension> dimensionIn)
    : properties_(parentWorld != nullptr ? parentWorld->properties_ : WorldProperties {}),
      hasStorageBackedProperties_(parentWorld != nullptr && parentWorld->hasStorageBackedProperties_),
      events_(*this),
      blockMutationContext_(*this),
      blockMutation_(blockMutationContext_),
      name_(parentWorld != nullptr ? parentWorld->name_ : std::string {}),
      dimensionData_(parentWorld != nullptr ? parentWorld->dimensionData_ : nullptr),
      seed_(parentWorld != nullptr ? parentWorld->seed_ : 0),
      time_(parentWorld != nullptr ? parentWorld->time_ : 0),
      spawnPos_(parentWorld != nullptr ? parentWorld->spawnPos_ : Vec3i {8, 64, 8}),
      random_(parentWorld != nullptr ? parentWorld->seed_ : 0),
      chunkGenerator_(nullptr, parentWorld != nullptr ? parentWorld->seed_ : 0),
      biomeSource_(parentWorld != nullptr ? parentWorld->seed_ : 0)
{
    if (parentWorld == nullptr) {
        throw std::runtime_error("Parent world is null");
    }
    initializeBlocks();
    time_ = parentWorld->time_;
    spawnPos_ = parentWorld->spawnPos_;
    dimension = std::move(dimensionIn);
    if (dimension == nullptr) {
        dimension = Dimension::fromId(0);
    }
    dimension->setWorld(this);
    persistentStateManager = SavedDataStorage(dimensionData_);
    chunkGeneratorSource_ = dimension->createChunkGenerator();
    chunkGenerator_.setWorld(this);
    if (dimensionData_ != nullptr) {
        createChunkCache();
    }
    updateSkyBrightness();
    prepareWeather();
    lcgBlockSeed_ = random_.nextInt();
    ambientSoundCounter_ = random_.nextInt(12000);
}

Vec3d World::getCloudColor(float partialTicks) const
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

Vec3d World::getSkyColor(Entity* entity, float partialTicks) const
{
    if (dimension == nullptr || entity == nullptr) {
        return {0.5, 0.7, 1.0};
    }
    float time = getTime(partialTicks);
    float brightness = MathHelper::cos(time * 3.14159265f * 2.0f) * 2.0f + 0.5f;
    if (brightness < 0.0f) {
        brightness = 0.0f;
    }
    if (brightness > 1.0f) {
        brightness = 1.0f;
    }
    const int x = MathHelper::floor(entity->x);
    const int z = MathHelper::floor(entity->z);
    const float temperature = static_cast<float>(getTemperature(x, z));
    const int skyColor = getBiomeDefinition(x, z).getSkyColor(temperature);
    float red = static_cast<float>((skyColor >> 16) & 0xFF) / 255.0f;
    float green = static_cast<float>((skyColor >> 8) & 0xFF) / 255.0f;
    float blue = static_cast<float>(skyColor & 0xFF) / 255.0f;
    red *= brightness;
    green *= brightness;
    blue *= brightness;
    const float rain = getRainGradient(partialTicks);
    if (rain > 0.0f) {
        const float gray = (red * 0.3f + green * 0.59f + blue * 0.11f) * 0.6f;
        const float blend = 1.0f - rain * 0.75f;
        red = red * blend + gray * (1.0f - blend);
        green = green * blend + gray * (1.0f - blend);
        blue = blue * blend + gray * (1.0f - blend);
    }
    const float thunder = getThunderGradient(partialTicks);
    if (thunder > 0.0f) {
        const float gray = (red * 0.3f + green * 0.59f + blue * 0.11f) * 0.2f;
        const float blend = 1.0f - thunder * 0.75f;
        red = red * blend + gray * (1.0f - blend);
        green = green * blend + gray * (1.0f - blend);
        blue = blue * blend + gray * (1.0f - blend);
    }
    if (weather_.lightningTicks > 0) {
        float flash = static_cast<float>(weather_.lightningTicks) - partialTicks;
        if (flash > 1.0f) {
            flash = 1.0f;
        }
        flash *= 0.45f;
        red = red * (1.0f - flash) + 0.8f * flash;
        green = green * (1.0f - flash) + 0.8f * flash;
        blue = blue * (1.0f - flash) + 1.0f * flash;
    }
    return {red, green, blue};
}

float World::getRainGradient(float partialTicks) const
{
    return weather_.rainGradient(partialTicks);
}

bool World::isRaining(int x, int y, int z) const
{
    if (!isRaining()) {
        return false;
    }
    if (!hasSkyLight(x, y, z)) {
        return false;
    }
    if (getTopSolidBlockY(x, z) > y) {
        return false;
    }
    const BiomeDefinition& biome = getBiomeDefinition(x, z);
    if (biome.canSnow()) {
        return false;
    }
    return biome.canRain();
}

float World::getThunderGradient(float partialTicks) const
{
    return weather_.thunderGradient(partialTicks);
}

int World::getAmbientDarkness(float partialTicks) const
{
    float brightness = getTime(partialTicks);
    float curve = 1.0f - (MathHelper::cos(brightness * 3.14159265f * 2.0f) * 2.0f + 0.5f);
    if (curve < 0.0f) {
        curve = 0.0f;
    }
    if (curve > 1.0f) {
        curve = 1.0f;
    }
    curve = 1.0f - curve;
    const float rain = getRainGradient(partialTicks) * 5.0f / 16.0f;
    const float thunder = getThunderGradient(partialTicks) * 5.0f / 16.0f;
    curve = static_cast<float>(static_cast<double>(curve) * (1.0 - static_cast<double>(rain)));
    curve = static_cast<float>(static_cast<double>(curve) * (1.0 - static_cast<double>(thunder)));
    curve = 1.0f - curve;
    return static_cast<int>(curve * 11.0f);
}

void World::updateSkyBrightness()
{
    const int value = getAmbientDarkness(1.0f);
    if (value != ambientDarkness) {
        ambientDarkness = value;
        events_.notifyAmbientDarknessChanged();
    }
}

void World::prepareWeather()
{
    if (!hasStorageBackedProperties_) {
        return;
    }
    if (properties_.getRaining()) {
        weather_.beginActiveWeather(properties_.getThundering());
    }
}

void World::saveLevelProperties()
{
    if (dimensionData_ != nullptr && hasStorageBackedProperties_) {
        checkSessionLock();
        dimensionData_->save(properties_, players);
    }
}

void World::save()
{
    if (!hasStorageBackedProperties_) {
        return;
    }
    saveLevelProperties();
    persistentStateManager.save();
    if (dimensionData_ != nullptr) {
        if (auto* alphaStorage = dynamic_cast<AlphaWorldStorage*>(dimensionData_)) {
            for (PlayerEntity* player : players) {
                if (player != nullptr) {
                    alphaStorage->savePlayerData(*player);
                }
            }
        }
    }
}

void World::checkSessionLock()
{
    if (dimensionData_ != nullptr) {
        dimensionData_->checkSessionLock();
    }
}

void World::initializeSpawnPoint()
{
    if (dimension == nullptr) {
        return;
    }
    eventProcessingEnabled = true;
    int x = 0;
    constexpr int y = 64;
    int z = 0;
    while (!dimension->isValidSpawnPoint(x, z)) {
        x += random_.nextInt(64) - random_.nextInt(64);
        z += random_.nextInt(64) - random_.nextInt(64);
    }
    properties_.setSpawn(x, y, z);
    setSpawnPos(Vec3i{x, y, z});
    eventProcessingEnabled = false;
}

void World::resetForProbeSeed(std::int64_t seed)
{
    seed_ = static_cast<std::uint64_t>(seed);
    properties_ = WorldProperties(seed_, name_);
    hasStorageBackedProperties_ = true;
    newWorld = true;
    random_.setSeed(seed_);
    spawnPos_ = Vec3i{8, 64, 8};
    properties_.setSpawn(8, 64, 8);
    time_ = 0;
    eventProcessingEnabled = false;
    chunks_.clear();
    lcgBlockSeed_ = random_.nextInt();
    ambientSoundCounter_ = random_.nextInt(12000);

    if (dimension != nullptr) {
        dimension->initBiomeSource();
        chunkGeneratorSource_ = dimension->createChunkGenerator();
        if (dimensionData_ != nullptr) {
            createChunkCache();
        }
    }
}

void World::updateSpawnPosition()
{
    if (properties_.getSpawnY() <= 0) {
        properties_.setSpawn(properties_.getSpawnX(), 64, properties_.getSpawnZ());
    }
    int x = properties_.getSpawnX();
    int z = properties_.getSpawnZ();
    while (getSpawnBlockId(x, z) == 0) {
        x += random_.nextInt(8) - random_.nextInt(8);
        z += random_.nextInt(8) - random_.nextInt(8);
    }
    properties_.setSpawn(x, properties_.getSpawnY(), z);
    setSpawnPos(Vec3i{x, properties_.getSpawnY(), z});
}

void World::addEventListener(GameEventListener* listener)
{
    events_.addEventListener(listener);
}

void World::removeEventListener(GameEventListener* listener)
{
    events_.removeEventListener(listener);
}

void World::blockUpdateEvent(int x, int y, int z)
{
    events_.blockUpdateEvent(x, y, z);
}

void World::blockUpdate(int x, int y, int z, int blockId)
{
    blockMutation_.blockUpdate(x, y, z, blockId);
}

void World::setBlockDirty(int x, int y, int z)
{
    events_.setBlockDirty(x, y, z);
}

bool World::isRemote() const
{
    return isRemote_;
}

bool World::setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta)
{
    return blockMutation_.setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta);
}

bool World::setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta)
{
    return blockMutation_.setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta);
}

bool World::canPlace(int blockId, int x, int y, int z, bool fallingBlock, int side)
{
    return blockMutation_.canPlace(blockId, x, y, z, fallingBlock, side);
}

void World::neighborUpdate(int x, int y, int z, int sourceBlockId)
{
    blockMutation_.neighborUpdate(x, y, z, sourceBlockId);
}

void World::notifyNeighbors(int x, int y, int z, int blockId)
{
    blockMutation_.notifyNeighbors(x, y, z, blockId);
}

void World::playSound(double x, double y, double z, const std::string& name, float volume, float pitch)
{
    events_.playSound(x, y, z, name, volume, pitch);
}

void World::playSound(Entity* source, const std::string& name, float volume, float pitch)
{
    if (source == nullptr) {
        playSound(0.0, 0.0, 0.0, name, volume, pitch);
        return;
    }
    playSound(source->x, source->y - static_cast<double>(source->standingEyeHeight), source->z, name, volume, pitch);
}

void World::playSound(PlayerEntity* player, const std::string& name, float volume, float pitch)
{
    if (player == nullptr) {
        playSound(0.0, 0.0, 0.0, name, volume, pitch);
        return;
    }
    playSound(player->x, player->y, player->z, name, volume, pitch);
}

void World::playStreaming(const std::string& name, int x, int y, int z)
{
    events_.playStreaming(name, x, y, z);
}

void World::worldEvent(PlayerEntity* player, int type, int x, int y, int z, int data)
{
    events_.worldEvent(player, type, x, y, z, data);
}

void World::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    events_.setBlocksDirty(minX, minY, minZ, maxX, maxY, maxZ);
}

void World::setBlocksDirtyColumn(int x, int z, int minY, int maxY)
{
    events_.setBlocksDirtyColumn(x, z, minY, maxY);
}

void World::setBlockEntity(int x, int y, int z, std::unique_ptr<block::entity::BlockEntity> blockEntity)
{
    if (blockEntity == nullptr) {
        return;
    }
    ensureChunk(x, z).setBlockEntity(mod_16(x), y, mod_16(z), std::move(blockEntity));
}

void World::removeBlockEntity(int x, int y, int z)
{
    block::entity::BlockEntity* blockEntity = getBlockEntity(x, y, z);
    if (blockEntity != nullptr && processingDeferred_) {
        blockEntity->markRemoved();
        return;
    }
    if (blockEntity != nullptr) {
        blockEntities.erase(
            std::remove(blockEntities.begin(), blockEntities.end(), blockEntity),
            blockEntities.end());
    }
    getChunk(chunk_coord(x), chunk_coord(z)).removeBlockEntityAt(mod_16(x), y, mod_16(z));
}

void World::updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity)
{
    events_.updateBlockEntity(x, y, z, blockEntity);
}

entity::ai::pathing::Path World::findPath(entity::Entity* entity, entity::Entity* target, float range)
{
    if (entity == nullptr || target == nullptr) {
        return entity::ai::pathing::Path {{}};
    }
    const int centerX = MathHelper::floor(entity->x);
    const int centerY = MathHelper::floor(entity->y);
    const int centerZ = MathHelper::floor(entity->z);
    const int radius = static_cast<int>(range + 16.0f);
    WorldRegion region(this, centerX - radius, centerY - radius, centerZ - radius, centerX + radius, centerY + radius,
        centerZ + radius);
    return entity::ai::pathing::PathNodeNavigator(&region).findPath(entity, target, range);
}

entity::ai::pathing::Path World::findPath(entity::Entity* entity, int x, int y, int z, float range)
{
    if (entity == nullptr) {
        return entity::ai::pathing::Path {{}};
    }
    const int centerX = MathHelper::floor(entity->x);
    const int centerY = MathHelper::floor(entity->y);
    const int centerZ = MathHelper::floor(entity->z);
    const int radius = static_cast<int>(range + 8.0f);
    WorldRegion region(this, centerX - radius, centerY - radius, centerZ - radius, centerX + radius, centerY + radius,
        centerZ + radius);
    return entity::ai::pathing::PathNodeNavigator(&region).findPath(entity, x, y, z, range);
}

void World::addParticle(const std::string& name, double px, double py, double pz, double vx, double vy, double vz)
{
    events_.addParticle(name, px, py, pz, vx, vy, vz);
}

void World::playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch)
{
    const int blockId = getBlockId(x, y, z);
    if (blockId > 0 && blockId < Block::BLOCK_COUNT) {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block != nullptr) {
            block->onBlockAction(this, x, y, z, soundType, pitch);
        }
    }
}

bool World::attemptSaving(int step)
{
    if (chunkCache_ == nullptr || !chunkCache_->canSave()) {
        return true;
    }
    if (step == 0) {
        save();
    }
    return chunkCache_->save(false, nullptr);
}

void World::savingProgress(client::gui::screen::LoadingDisplay* display)
{
    save();
    if (chunkCache_ != nullptr) {
        chunkCache_->save(true, display);
    }
}

void World::applyWorldSettings(bool weatherEnabled, int autoSaveTicks, int timeMode)
{
    weather_.setEnabled(weatherEnabled);
    if (isRemote_) {
        return;
    }
    saveInterval_ = autoSaveTicks;
    clientTimeMode_ = timeMode;
    if (!weather_.enabled()) {
        clearWeather();
        weather_.resetGradients();
    }
}

void World::setChunkResidentRadius(int radiusChunks)
{
    chunkResidentRadiusChunks_ = std::max(1, radiusChunks);
    if (auto* legacyCache = dynamic_cast<LegacyChunkCache*>(getChunkSource())) {
        legacyCache->setActiveRadius(chunkResidentRadiusChunks_);
    }
}

void World::setTime(std::uint64_t value) noexcept
{
    mod::WorldTimeEvent event {this, static_cast<long long>(time_), static_cast<long long>(value), false};
    mod::hooks().publish(event);
    if (event.canceled) {
        return;
    }
    time_ = static_cast<std::uint64_t>(event.newTime);
    if (hasStorageBackedProperties_) {
        properties_.setTime(time_);
    }
}

void World::updateWeatherCycles()
{
    mod::WeatherCycleEvent event {this, false, false};
    mod::hooks().publish(event);
    if (event.canceled) {
        return;
    }
    if (dimension != nullptr && dimension->hasCeiling) {
        return;
    }
    if (weather_.ticksSinceLightning > 0) {
        --weather_.ticksSinceLightning;
    }
    if (!weather_.enabled() && !isRemote_) {
        weather_.decayGradients();
        return;
    }
    if (!hasStorageBackedProperties_) {
        return;
    }
    int thunderTime = properties_.getThunderTime();
    if (thunderTime <= 0) {
        if (properties_.getThundering()) {
            properties_.setThunderTime(random_.nextInt(12000) + 3600);
        } else {
            properties_.setThunderTime(random_.nextInt(168000) + 12000);
        }
    } else {
        properties_.setThunderTime(--thunderTime);
        if (thunderTime <= 0) {
            properties_.setThundering(!properties_.getThundering());
        }
    }
    int rainTime = properties_.getRainTime();
    if (rainTime <= 0) {
        if (properties_.getRaining()) {
            properties_.setRainTime(random_.nextInt(12000) + 12000);
        } else {
            properties_.setRainTime(random_.nextInt(168000) + 12000);
        }
    } else {
        properties_.setRainTime(--rainTime);
        if (rainTime <= 0) {
            properties_.setRaining(!properties_.getRaining());
        }
    }
    weather_.tickGradients(properties_.getRaining(), properties_.getThundering());
}

void World::clearWeather()
{
    if (!hasStorageBackedProperties_) {
        return;
    }
    properties_.setRainTime(0);
    properties_.setRaining(false);
    properties_.setThunderTime(0);
    properties_.setThundering(false);
}

void World::displayTick(int x, int y, int z)
{
    constexpr int radius = 16;
    // Java uses new Random() each tick (time-seeded); derive from world RNG so pitch/volume vary.
    JavaRandom random(static_cast<std::uint64_t>(random_.nextLong()));
    for (int i = 0; i < 1000; ++i) {
        const int blockX = x + random_.nextInt(radius) - random_.nextInt(radius);
        const int blockY = y + random_.nextInt(radius) - random_.nextInt(radius);
        const int blockZ = z + random_.nextInt(radius) - random_.nextInt(radius);
        const int blockId = getBlockId(blockX, blockY, blockZ);
        if (blockId <= 0) {
            continue;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block != nullptr) {
            block->randomDisplayTick(this, blockX, blockY, blockZ, random);
        }
    }
}

void World::queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    queueLightUpdate(type, minX, minY, minZ, maxX, maxY, maxZ, true);
}

void World::queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge)
{
    if (dimension != nullptr && dimension->hasCeiling && type == LightType::Sky) {
        return;
    }

    const int centerX = (maxX + minX) / 2;
    const int centerZ = (maxZ + minZ) / 2;
    if (!isPosLoaded(centerX, 64, centerZ)) {
        return;
    }
    if (getChunkFromPos(centerX, centerZ).isEmpty()) {
        return;
    }

    lighting_.setSkyLightSuppressed(dimension != nullptr && dimension->hasCeiling);
    lighting_.push(type, minX, minY, minZ, maxX, maxY, maxZ, merge);
}

bool World::doLightingUpdates(std::size_t maxDirtyRegions)
{
    for (const LightingEngine::DirtyRegion& region : lighting_.drainDirtyRegions(maxDirtyRegions)) {
        events_.setBlocksDirty(region.minX, region.minY, region.minZ, region.maxX, region.maxY, region.maxZ);
    }
    return lighting_.busy() || lighting_.hasDirtyRegions();
}

void World::finishLightingUpdates()
{
    lighting_.flush();
    doLightingUpdates(std::numeric_limits<std::size_t>::max());
}

World::~World()
{
    lighting_.stop();
}

void World::scheduleBlockUpdate(int x, int y, int z, int id, int tickRate)
{
    mod::ScheduleBlockUpdateEvent scheduleEvent {this, x, y, z, id, tickRate, false};
    mod::hooks().publish(scheduleEvent);
    if (scheduleEvent.canceled) {
        return;
    }
    x = scheduleEvent.x;
    y = scheduleEvent.y;
    z = scheduleEvent.z;
    id = scheduleEvent.blockId;
    tickRate = scheduleEvent.tickRate;

    BlockEvent blockEvent(x, y, z, id);
    constexpr int regionPadding = 8;
    if (instantBlockUpdateEnabled) {
        if (isRegionLoaded(blockEvent.x - regionPadding, blockEvent.y - regionPadding, blockEvent.z - regionPadding,
                blockEvent.x + regionPadding, blockEvent.y + regionPadding, blockEvent.z + regionPadding)) {
            const int currentId = getBlockId(blockEvent.x, blockEvent.y, blockEvent.z);
            if (currentId == blockEvent.blockId && currentId > 0) {
                Block* block = Block::BLOCKS[static_cast<std::size_t>(currentId)];
                if (block != nullptr) {
                    mod::ScheduledBlockTickEvent tickEvent {
                        this, block, blockEvent.x, blockEvent.y, blockEvent.z, currentId, true, false};
                    mod::hooks().publish(tickEvent);
                    if (!tickEvent.canceled) {
                        block->onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, random_);
                    }
                }
            }
        }
        return;
    }

    if (!isRegionLoaded(x - regionPadding, y - regionPadding, z - regionPadding, x + regionPadding, y + regionPadding,
            z + regionPadding)) {
        return;
    }

    if (id > 0) {
        const long long scheduledTime = static_cast<long long>(tickRate)
            + static_cast<long long>(hasStorageBackedProperties_ ? properties_.getTime() : time_);
        blockEvent = blockEvent.withTicks(scheduledTime);
    }

    scheduledTicks_.schedule(blockEvent);
}

bool World::processScheduledTicks(bool flush)
{
    const int budget = scheduledTicks_.tickBudget();

    for (int i = 0; i < budget; ++i) {
        if (scheduledTicks_.empty()) {
            break;
        }
        BlockEvent blockEvent = scheduledTicks_.peek();
        const long long currentTime = static_cast<long long>(hasStorageBackedProperties_ ? properties_.getTime() : time_);
        if (!flush && blockEvent.ticks > currentTime) {
            break;
        }

        scheduledTicks_.popFront();

        constexpr int regionPadding = 8;
        if (!isRegionLoaded(blockEvent.x - regionPadding, blockEvent.y - regionPadding, blockEvent.z - regionPadding,
                blockEvent.x + regionPadding, blockEvent.y + regionPadding, blockEvent.z + regionPadding)) {
            continue;
        }
        const int currentId = getBlockId(blockEvent.x, blockEvent.y, blockEvent.z);
        if (currentId != blockEvent.blockId || currentId <= 0) {
            continue;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(currentId)];
        if (block != nullptr) {
            mod::ScheduledBlockTickEvent tickEvent {
                this, block, blockEvent.x, blockEvent.y, blockEvent.z, currentId, false, false};
            mod::hooks().publish(tickEvent);
            if (!tickEvent.canceled) {
                block->onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, random_);
            }
        }
    }
    return !scheduledTicks_.empty();
}

Explosion World::createExplosion(Entity* source, double x, double y, double z, float power)
{
    return createExplosion(source, x, y, z, power, false);
}

Explosion World::createExplosion(Entity* source, double x, double y, double z, float power, bool fire)
{
    Explosion explosion(this, source, x, y, z, power);
    explosion.fire = fire;
    explosion.explode();
    explosion.playExplosionSound(true);
    return explosion;
}

bool World::canTransferPowerInDirection(int x, int y, int z, int direction)
{
    const int blockId = getBlockId(x, y, z);
    if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
        return false;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    return block != nullptr && block->canTransferPowerInDirection(this, x, y, z, direction);
}

bool World::canTransferPower(int x, int y, int z)
{
    if (canTransferPowerInDirection(x, y - 1, z, 0)) {
        return true;
    }
    if (canTransferPowerInDirection(x, y + 1, z, 1)) {
        return true;
    }
    if (canTransferPowerInDirection(x, y, z - 1, 2)) {
        return true;
    }
    if (canTransferPowerInDirection(x, y, z + 1, 3)) {
        return true;
    }
    if (canTransferPowerInDirection(x - 1, y, z, 4)) {
        return true;
    }
    return canTransferPowerInDirection(x + 1, y, z, 5);
}

bool World::isEmittingRedstonePowerInDirection(int x, int y, int z, int direction)
{
    if (shouldSuffocate(x, y, z)) {
        return canTransferPower(x, y, z);
    }
    const int blockId = getBlockId(x, y, z);
    if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
        return false;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    return block != nullptr && block->isEmittingRedstonePowerInDirection(this, x, y, z, direction);
}

bool World::isEmittingRedstonePower(int x, int y, int z)
{
    if (isEmittingRedstonePowerInDirection(x, y - 1, z, 0)) {
        return true;
    }
    if (isEmittingRedstonePowerInDirection(x, y + 1, z, 1)) {
        return true;
    }
    if (isEmittingRedstonePowerInDirection(x, y, z - 1, 2)) {
        return true;
    }
    if (isEmittingRedstonePowerInDirection(x, y, z + 1, 3)) {
        return true;
    }
    if (isEmittingRedstonePowerInDirection(x - 1, y, z, 4)) {
        return true;
    }
    return isEmittingRedstonePowerInDirection(x + 1, y, z, 5);
}


float World::getTime(float partialTicks) const
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

Vec3d World::getFogColor(float partialTicks) const
{
    if (dimension) {
        return dimension->getFogColor(getTime(partialTicks), partialTicks);
    }
    return {0.75, 0.85, 1.0};
}

float World::calculateSkyLightIntensity(float partialTicks) const
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

void World::tick()
{
    mod::WorldTickEvent beforeTick {this, false, true};
    mod::hooks().publish(beforeTick);

    updateWeatherCycles();
    if (canSkipNight()) {
        int spawned = 0;
        if (allowMonsterSpawning && difficulty >= 1) {
            spawned = NaturalSpawner::spawnMonstersAndWakePlayers(this, players);
        }
        if (spawned == 0) {
            const std::uint64_t currentTime = hasStorageBackedProperties_ ? properties_.getTime() : time_;
            const std::uint64_t nextDawn = currentTime + 24000ULL;
            setTime(nextDawn - (nextDawn % 24000ULL));
            afterSkipNight();
        }
    }
    // Match Java World.tick NaturalSpawner argument order (monster field -> spawnAnimals param).
    NaturalSpawner::tick(this, allowMonsterSpawning, allowMobSpawning);
    if (chunkCache_ != nullptr) {
        chunkCache_->tick();
    }
    const std::uint64_t nextTime = time_ + 1;
    if (hasStorageBackedProperties_ && nextTime % static_cast<std::uint64_t>(saveInterval_) == 0) {
        save();
        if (chunkCache_ != nullptr) {
            chunkCache_->save(false, nullptr);
        }
    }
    setTime(nextTime);
    updateSkyBrightness();
    processScheduledTicks(false);
    manageChunkUpdatesAndEvents();

    mod::WorldTickEvent afterTick {this, false, false};
    mod::hooks().publish(afterTick);
}

} // namespace net::minecraft
