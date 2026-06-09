#include "net/minecraft/world/World.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/Monster.hpp"
#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"
#include "net/minecraft/entity/SpawnGroup.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
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
#include "net/minecraft/world/event/listener/GameEventListener.hpp"
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

ChunkSource* World::createChunkCache()
{
    if (dimensionData_ == nullptr || dimension == nullptr) {
        return getChunkSource();
    }
    std::unique_ptr<ChunkStorage> chunkStorage = dimensionData_->getChunkStorage(dimension.get());
    chunkGeneratorSource_ = dimension->createChunkGenerator();
    setChunkCache(std::make_unique<LegacyChunkCache>(this, std::move(chunkStorage), chunkGeneratorSource_.get()));
    return getChunkSource();
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
    const float temperature = getBiomeSource() != nullptr ? static_cast<float>(getBiomeSource()->getTemperature(x, z)) : 0.5f;
    const int skyColor = getBiomeSource() != nullptr
        ? Biomes::byInfo(getBiomeSource()->getBiome(x, z)).getSkyColor(temperature)
        : 0x87CEEB;
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
    if (lightningTicksLeft > 0) {
        float flash = static_cast<float>(lightningTicksLeft) - partialTicks;
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
    if (!weatherEnabled_) {
        return 0.0f;
    }
    return rainGradientPrev + (rainGradient - rainGradientPrev) * partialTicks;
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
    if (getBiomeSource() == nullptr) {
        return false;
    }
    const BiomeDefinition& biome = Biomes::byId(getBiomeSource()->getBiome(x, z).id);
    if (biome.canSnow()) {
        return false;
    }
    return biome.canRain();
}

float World::getThunderGradient(float partialTicks) const
{
    if (!weatherEnabled_) {
        return 0.0f;
    }
    return (thunderGradientPrev + (thunderGradient - thunderGradientPrev) * partialTicks) * getRainGradient(partialTicks);
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
        rainGradient = 1.0f;
        if (properties_.getThundering()) {
            thunderGradient = 1.0f;
        }
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

void World::updateEntityLists()
{
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(), [this](Entity* entity) {
            return entity != nullptr
                && std::find(entitiesToUnload_.begin(), entitiesToUnload_.end(), entity) != entitiesToUnload_.end();
        }),
        entities_.end());

    for (Entity* entity : entitiesToUnload_) {
        if (entity == nullptr) {
            continue;
        }
        if (entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
            getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
        }
    }
    for (Entity* entity : entitiesToUnload_) {
        notifyEntityRemoved(entity);
    }
    entitiesToUnload_.clear();

    for (std::size_t i = 0; i < entities_.size(); ++i) {
        Entity* entity = entities_[i];
        if (entity == nullptr) {
            continue;
        }
        if (entity->vehicle != nullptr) {
            if (!entity->vehicle->dead && entity->vehicle->passenger == entity) {
                continue;
            }
            entity->vehicle->passenger = nullptr;
            entity->vehicle = nullptr;
        }
        if (!entity->dead) {
            continue;
        }
        if (entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
            getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
        }
        entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(i));
        notifyEntityRemoved(entity);
        --i;
    }
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

bool World::spawnGlobalEntity(Entity* entity)
{
    if (entity == nullptr) {
        return false;
    }
    entity->world = this;
    globalEntities.push_back(entity);
    notifyEntityAdded(entity);
    return true;
}

bool World::spawnEntity(Entity* entity)
{
    if (entity == nullptr) {
        return false;
    }

    const int chunkX = MathHelper::floor(entity->x / 16.0);
    const int chunkZ = MathHelper::floor(entity->z / 16.0);
    const bool isPlayer = dynamic_cast<PlayerEntity*>(entity) != nullptr;
    if (!isPlayer && !hasChunk(chunkX, chunkZ)) {
        return false;
    }
    if (isPlayer) {
        players.push_back(static_cast<PlayerEntity*>(entity));
        updateSleepingPlayers();
    }

    entity->world = this;
    entities_.push_back(entity);
    getChunk(chunkX, chunkZ).addEntity(entity);
    notifyEntityAdded(entity);
    return true;
}

void World::notifyEntityAdded(Entity* entity)
{
    events_.notifyEntityAdded(entity);
}

void World::notifyEntityRemoved(Entity* entity)
{
    events_.notifyEntityRemoved(entity);
}

void World::serverRemove(Entity* entity)
{
    if (entity == nullptr) {
        return;
    }

    entity->markDead();
    entities_.erase(std::remove(entities_.begin(), entities_.end(), entity), entities_.end());
    if (auto* player = dynamic_cast<PlayerEntity*>(entity)) {
        players.erase(std::remove(players.begin(), players.end(), player), players.end());
        updateSleepingPlayers();
    }

    if (entity->isPersistent) {
        if (Chunk* chunk = getChunkIfLoaded(entity->chunkX << 4, entity->chunkZ << 4)) {
            chunk->removeEntity(entity);
        }
    }
    notifyEntityRemoved(entity);
}

void World::remove(Entity* entity)
{
    if (entity == nullptr) {
        return;
    }
    if (entity->passenger != nullptr) {
        entity->passenger->setVehicle(nullptr);
    }
    if (entity->vehicle != nullptr) {
        entity->setVehicle(nullptr);
    }
    entity->markDead();
    if (auto* player = dynamic_cast<PlayerEntity*>(entity)) {
        players.erase(std::remove(players.begin(), players.end(), player), players.end());
        updateSleepingPlayers();
    }
}

bool World::canSpawnEntity(const Box& box)
{
    for (Entity* entity : getEntities(nullptr, box)) {
        if (entity == nullptr || entity->dead || !entity->blocksSameBlockSpawning) {
            continue;
        }
        return false;
    }
    return true;
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

bool World::isPosLoaded(int x, int y, int z) const
{
    if (y < 0 || y >= Chunk::height) {
        return false;
    }
    return getChunkIfLoaded(x, z) != nullptr;
}

void World::handleChunkDataUpdate(
    int x,
    int y,
    int z,
    int sizeX,
    int sizeY,
    int sizeZ,
    const std::vector<std::uint8_t>& chunkData)
{
    const int minChunkX = x >> 4;
    const int minChunkZ = z >> 4;
    const int maxChunkX = (x + sizeX - 1) >> 4;
    const int maxChunkZ = (z + sizeZ - 1) >> 4;
    int offset = 0;
    int minY = y;
    int maxY = y + sizeY;
    if (minY < 0) {
        minY = 0;
    }
    if (maxY > Chunk::height) {
        maxY = Chunk::height;
    }
    for (int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
        int localMinX = x - chunkX * 16;
        int localMaxX = x + sizeX - chunkX * 16;
        if (localMinX < 0) {
            localMinX = 0;
        }
        if (localMaxX > 16) {
            localMaxX = 16;
        }
        for (int chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
            int localMinZ = z - chunkZ * 16;
            int localMaxZ = z + sizeZ - chunkZ * 16;
            if (localMinZ < 0) {
                localMinZ = 0;
            }
            if (localMaxZ > 16) {
                localMaxZ = 16;
            }
            offset = getChunk(chunkX, chunkZ).loadFromPacket(
                chunkData,
                localMinX,
                minY,
                localMinZ,
                localMaxX,
                maxY,
                localMaxZ,
                offset);
            setBlocksDirty(
                chunkX * 16 + localMinX,
                minY,
                chunkZ * 16 + localMinZ,
                chunkX * 16 + localMaxX,
                maxY,
                chunkZ * 16 + localMaxZ);
        }
    }
}

block::entity::BlockEntity* World::getBlockEntity(int x, int y, int z)
{
    return getChunk(chunk_coord(x), chunk_coord(z)).getBlockEntity(mod_16(x), y, mod_16(z));
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

void World::notifyEntityPickup(Entity* entity, PlayerEntity* collector)
{
    events_.notifyEntityPickup(entity, collector);
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

PlayerEntity* World::getClosestPlayer(double targetX, double targetY, double targetZ, double range)
{
    double bestDistance = -1.0;
    PlayerEntity* closest = nullptr;
    const double rangeSq = range < 0.0 ? -1.0 : range * range;

    for (PlayerEntity* player : players) {
        if (player == nullptr) {
            continue;
        }
        const double distanceSq = player->getSquaredDistance(targetX, targetY, targetZ);
        if (rangeSq >= 0.0 && distanceSq >= rangeSq) {
            continue;
        }
        if (bestDistance >= 0.0 && distanceSq >= bestDistance) {
            continue;
        }
        bestDistance = distanceSq;
        closest = player;
    }
    return closest;
}

PlayerEntity* World::getClosestPlayer(Entity* entity, double range)
{
    if (entity == nullptr) {
        return nullptr;
    }
    return getClosestPlayer(entity->x, entity->y, entity->z, range);
}

Entity* World::getClosestPlayerEntity(Entity* entity, double range)
{
    return getClosestPlayer(entity, range);
}

PlayerEntity* World::getPlayer(const std::string& name)
{
    for (PlayerEntity* player : players) {
        if (player != nullptr && player->name == name) {
            return player;
        }
    }
    return nullptr;
}

int World::getTopSolidBlockY(int x, int z) const
{
    const Chunk& chunk = getChunk(chunk_coord(x), chunk_coord(z));
    x = mod_16(x);
    z = mod_16(z);
    for (int y = Chunk::height - 1; y > 0; --y) {
        const int blockId = chunk.getBlockId(x, y, z);
        const Block* block = blockId > 0 && blockId < Block::BLOCK_COUNT ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
        if (block == nullptr) {
            continue;
        }
        const auto& material = block->material;
        if (!material.blocksMovement() && !material.isFluid()) {
            continue;
        }
        return y + 1;
    }
    return -1;
}

bool World::isMaterialInBox(const Box& boundingBox, block::material::Material& material) const
{
    const int minX = MathHelper::floor(boundingBox.minX);
    const int maxX = MathHelper::floor(boundingBox.maxX + 1.0);
    const int minY = MathHelper::floor(boundingBox.minY);
    const int maxY = MathHelper::floor(boundingBox.maxY + 1.0);
    const int minZ = MathHelper::floor(boundingBox.minZ);
    const int maxZ = MathHelper::floor(boundingBox.maxZ + 1.0);
    for (int x = minX; x < maxX; ++x) {
        for (int y = minY; y < maxY; ++y) {
            for (int z = minZ; z < maxZ; ++z) {
                const int blockId = getBlockId(x, y, z);
                if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
                    continue;
                }
                Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                if (block != nullptr && &block->material == &material) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool World::isFluidInBox(const Box& boundingBox, block::material::Material& fluid) const
{
    const int minX = MathHelper::floor(boundingBox.minX);
    const int maxX = MathHelper::floor(boundingBox.maxX + 1.0);
    const int minY = MathHelper::floor(boundingBox.minY);
    const int maxY = MathHelper::floor(boundingBox.maxY + 1.0);
    const int minZ = MathHelper::floor(boundingBox.minZ);
    const int maxZ = MathHelper::floor(boundingBox.maxZ + 1.0);
    for (int x = minX; x < maxX; ++x) {
        for (int y = minY; y < maxY; ++y) {
            for (int z = minZ; z < maxZ; ++z) {
                const int blockId = getBlockId(x, y, z);
                if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
                    continue;
                }
                Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                if (block == nullptr || &block->material != &fluid) {
                    continue;
                }
                const int meta = getBlockMeta(x, y, z);
                double fluidSurfaceY = static_cast<double>(y + 1);
                if (meta < 8) {
                    fluidSurfaceY = static_cast<double>(y + 1) - static_cast<double>(meta) / 8.0;
                }
                if (fluidSurfaceY >= boundingBox.minY) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool World::isFireOrLavaInBox(const Box& box) const
{
    const int minX = MathHelper::floor(box.minX);
    const int maxX = MathHelper::floor(box.maxX + 1.0);
    const int minY = MathHelper::floor(box.minY);
    const int maxY = MathHelper::floor(box.maxY + 1.0);
    const int minZ = MathHelper::floor(box.minZ);
    const int maxZ = MathHelper::floor(box.maxZ + 1.0);
    if (!isRegionLoaded(minX, minY, minZ, maxX, maxY, maxZ)) {
        return false;
    }
    for (int x = minX; x < maxX; ++x) {
        for (int y = minY; y < maxY; ++y) {
            for (int z = minZ; z < maxZ; ++z) {
                const int blockId = getBlockId(x, y, z);
                if (Block::FIRE != nullptr && blockId == Block::FIRE->id) {
                    return true;
                }
                if (Block::FLOWING_LAVA != nullptr && blockId == Block::FLOWING_LAVA->id) {
                    return true;
                }
                if (Block::LAVA != nullptr && blockId == Block::LAVA->id) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool World::updateMovementInFluid(const Box& entityBoundingBox, block::material::Material& fluidMaterial, Entity* entity)
{
    const int minX = MathHelper::floor(entityBoundingBox.minX);
    const int maxX = MathHelper::floor(entityBoundingBox.maxX + 1.0);
    const int minY = MathHelper::floor(entityBoundingBox.minY);
    const int maxY = MathHelper::floor(entityBoundingBox.maxY + 1.0);
    const int minZ = MathHelper::floor(entityBoundingBox.minZ);
    const int maxZ = MathHelper::floor(entityBoundingBox.maxZ + 1.0);
    if (!isRegionLoaded(minX, minY, minZ, maxX, maxY, maxZ)) {
        return false;
    }
    bool inFluid = false;
    Vec3d flowVelocity {};
    for (int x = minX; x < maxX; ++x) {
        for (int y = minY; y < maxY; ++y) {
            for (int z = minZ; z < maxZ; ++z) {
                const int blockId = getBlockId(x, y, z);
                if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
                    continue;
                }
                Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                if (block == nullptr || &block->material != &fluidMaterial) {
                    continue;
                }
                const double fluidSurface =
                    static_cast<double>(static_cast<float>(y + 1)
                        - block::LiquidBlock::getFluidHeightFromMeta(getBlockMeta(x, y, z)));
                if (static_cast<double>(maxY) < fluidSurface) {
                    continue;
                }
                inFluid = true;
                if (entity != nullptr) {
                    block->applyVelocity(this, x, y, z, entity, flowVelocity);
                }
            }
        }
    }
    if (entity != nullptr) {
        const double flowLength = std::sqrt(flowVelocity.x * flowVelocity.x + flowVelocity.y * flowVelocity.y
            + flowVelocity.z * flowVelocity.z);
        if (flowLength > 0.0) {
            const Vec3d normalized = flowVelocity.normalize();
            constexpr double flowPush = 0.014;
            entity->velocityX += normalized.x * flowPush;
            entity->velocityY += normalized.y * flowPush;
            entity->velocityZ += normalized.z * flowPush;
        }
    }
    return inFluid;
}

bool World::isBoxSubmergedInFluid(const Box& box) const
{
    int minX = MathHelper::floor(box.minX);
    int maxX = MathHelper::floor(box.maxX + 1.0);
    int minY = MathHelper::floor(box.minY);
    int maxY = MathHelper::floor(box.maxY + 1.0);
    int minZ = MathHelper::floor(box.minZ);
    int maxZ = MathHelper::floor(box.maxZ + 1.0);
    if (box.minX < 0.0) {
        --minX;
    }
    if (box.minY < 0.0) {
        --minY;
    }
    if (box.minZ < 0.0) {
        --minZ;
    }
    for (int x = minX; x < maxX; ++x) {
        for (int y = minY; y < maxY; ++y) {
            for (int z = minZ; z < maxZ; ++z) {
                const int blockId = getBlockId(x, y, z);
                if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
                    continue;
                }
                Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                if (block == nullptr || !block->material.isFluid()) {
                    continue;
                }
                return true;
            }
        }
    }
    return false;
}

void World::extinguishFire(PlayerEntity* player, int x, int y, int z, int direction)
{
    if (direction == 0) {
        --y;
    } else if (direction == 1) {
        ++y;
    } else if (direction == 2) {
        --z;
    } else if (direction == 3) {
        ++z;
    } else if (direction == 4) {
        --x;
    } else if (direction == 5) {
        ++x;
    }
    if (Block::FIRE != nullptr && getBlockId(x, y, z) == Block::FIRE->id) {
        worldEvent(player, 1004, x, y, z, 0);
        setBlock(x, y, z, 0);
    }
}

std::optional<HitResult> World::raycast(const Vec3d& start, const Vec3d& end, bool ignoreLiquids, bool ignoreNonFullBlocks) const
{
    if (std::isnan(start.x) || std::isnan(start.y) || std::isnan(start.z) || std::isnan(end.x) || std::isnan(end.y)
        || std::isnan(end.z)) {
        return std::nullopt;
    }

    int x = MathHelper::floor(start.x);
    int y = MathHelper::floor(start.y);
    int z = MathHelper::floor(start.z);
    const int endX = MathHelper::floor(end.x);
    const int endY = MathHelper::floor(end.y);
    const int endZ = MathHelper::floor(end.z);

    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double dz = end.z - start.z;

    double tMaxX = 0.0;
    double tMaxY = 0.0;
    double tMaxZ = 0.0;
    double tDeltaX = 0.0;
    double tDeltaY = 0.0;
    double tDeltaZ = 0.0;
    int stepX = 0;
    int stepY = 0;
    int stepZ = 0;

    if (dx > 0.0) {
        stepX = 1;
        tMaxX = ((static_cast<double>(x) + 1.0) - start.x) / dx;
        tDeltaX = 1.0 / dx;
    } else if (dx < 0.0) {
        stepX = -1;
        tMaxX = (start.x - static_cast<double>(x)) / -dx;
        tDeltaX = 1.0 / -dx;
    } else {
        tMaxX = std::numeric_limits<double>::infinity();
        tDeltaX = std::numeric_limits<double>::infinity();
    }

    if (dy > 0.0) {
        stepY = 1;
        tMaxY = ((static_cast<double>(y) + 1.0) - start.y) / dy;
        tDeltaY = 1.0 / dy;
    } else if (dy < 0.0) {
        stepY = -1;
        tMaxY = (start.y - static_cast<double>(y)) / -dy;
        tDeltaY = 1.0 / -dy;
    } else {
        tMaxY = std::numeric_limits<double>::infinity();
        tDeltaY = std::numeric_limits<double>::infinity();
    }

    if (dz > 0.0) {
        stepZ = 1;
        tMaxZ = ((static_cast<double>(z) + 1.0) - start.z) / dz;
        tDeltaZ = 1.0 / dz;
    } else if (dz < 0.0) {
        stepZ = -1;
        tMaxZ = (start.z - static_cast<double>(z)) / -dz;
        tDeltaZ = 1.0 / -dz;
    } else {
        tMaxZ = std::numeric_limits<double>::infinity();
        tDeltaZ = std::numeric_limits<double>::infinity();
    }

    for (int i = 0; i < 200; ++i) {
        const int blockId = getBlockId(x, y, z);
        if (blockId > 0 && blockId < Block::BLOCK_COUNT) {
            Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
            if (block != nullptr) {
                const int meta = getBlockMeta(x, y, z);
                const std::optional<Box> collisionShape = block->getCollisionShape(const_cast<World*>(this), x, y, z);
                const bool hasShape = collisionShape.has_value();
                if ((!ignoreNonFullBlocks || hasShape) && block->hasCollision(meta, ignoreLiquids)) {
                    if (const std::optional<HitResult> hit =
                            block->raycast(const_cast<World*>(this), x, y, z, start, end);
                        hit.has_value()) {
                        return hit;
                    }
                }
            }
        }

        if (x == endX && y == endY && z == endZ) {
            return std::nullopt;
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                tMaxX += tDeltaX;
            } else {
                z += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else if (tMaxY < tMaxZ) {
            y += stepY;
            tMaxY += tDeltaY;
        } else {
            z += stepZ;
            tMaxZ += tDeltaZ;
        }
    }

    return std::nullopt;
}

int World::countEntities(const std::string& entityType) const
{
    int count = 0;
    for (Entity* entity : entities_) {
        if (entity == nullptr || entity->dead) {
            continue;
        }
        if (EntityRegistry::getId(*entity) == entityType) {
            ++count;
        }
    }
    return count;
}

int World::countSpawnGroup(const entity::SpawnGroupKind kind) const
{
    int count = 0;
    for (Entity* entity : entities_) {
        if (entity == nullptr) {
            continue;
        }
        switch (kind) {
        case entity::SpawnGroupKind::Monster:
            if (dynamic_cast<entity::Monster*>(entity) != nullptr) {
                ++count;
            }
            break;
        case entity::SpawnGroupKind::Creature:
            if (dynamic_cast<entity::passive::AnimalEntity*>(entity) != nullptr) {
                ++count;
            }
            break;
        case entity::SpawnGroupKind::WaterCreature:
            if (dynamic_cast<entity::WaterCreatureEntity*>(entity) != nullptr) {
                ++count;
            }
            break;
        }
    }
    return count;
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

bool World::doLightingUpdates()
{
    if (lightingUpdateCount_ >= 50) {
        return false;
    }
    ++lightingUpdateCount_;
    bool hadWork = false;
    try {
        int budget = 100;
        while (!lightingQueue_.empty()) {
            if (--budget <= 0) {
                hadWork = true;
                break;
            }
            LightUpdate update = lightingQueue_.back();
            lightingQueue_.pop_back();
            update.updateLight(this);
        }
    } catch (...) {
        --lightingUpdateCount_;
        throw;
    }
    --lightingUpdateCount_;
    return hadWork;
}

void World::addPlayer(PlayerEntity* player)
{
    if (player == nullptr) {
        return;
    }
    try {
        if (const NbtCompound* playerNbt = properties_.getPlayerNbt(); playerNbt != nullptr) {
            player->readNbt(*playerNbt);
            properties_.clearPlayerNbt();
        }
        if (ChunkSource* chunkSource = getChunkSource()) {
            if (auto* legacyCache = dynamic_cast<LegacyChunkCache*>(chunkSource)) {
                const int chunkX = MathHelper::floor(player->x) >> 4;
                const int chunkZ = MathHelper::floor(player->z) >> 4;
                legacyCache->setSpawnPoint(chunkX, chunkZ);
            }
        }
        spawnEntity(player);
    } catch (...) {
    }
}

void World::applyWorldSettings(bool weatherEnabled, int autoSaveTicks, int timeMode)
{
    weatherEnabled_ = weatherEnabled;
    if (isRemote_) {
        return;
    }
    saveInterval_ = autoSaveTicks;
    clientTimeMode_ = timeMode;
    if (!weatherEnabled_) {
        clearWeather();
        rainGradient = rainGradientPrev = 0.0f;
        thunderGradient = thunderGradientPrev = 0.0f;
    }
}

void World::setChunkPreloadRadius(int radius)
{
    chunkPreloadRadius_ = std::max(1, radius);
}

void World::updateWeatherCycles()
{
    if (dimension != nullptr && dimension->hasCeiling) {
        return;
    }
    if (ticksSinceLightning_ > 0) {
        --ticksSinceLightning_;
    }
    if (!weatherEnabled_ && !isRemote_) {
        rainGradientPrev = rainGradient;
        thunderGradientPrev = thunderGradient;
        if (rainGradient > 0.0f) {
            rainGradient = static_cast<float>(static_cast<double>(rainGradient) - 0.01);
        }
        if (thunderGradient > 0.0f) {
            thunderGradient = static_cast<float>(static_cast<double>(thunderGradient) - 0.01);
        }
        rainGradient = std::clamp(rainGradient, 0.0f, 1.0f);
        thunderGradient = std::clamp(thunderGradient, 0.0f, 1.0f);
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
    rainGradientPrev = rainGradient;
    if (properties_.getRaining()) {
        rainGradient = static_cast<float>(static_cast<double>(rainGradient) + 0.01);
    } else {
        rainGradient = static_cast<float>(static_cast<double>(rainGradient) - 0.01);
    }
    rainGradient = std::clamp(rainGradient, 0.0f, 1.0f);
    thunderGradientPrev = thunderGradient;
    if (properties_.getThundering()) {
        thunderGradient = static_cast<float>(static_cast<double>(thunderGradient) + 0.01);
    } else {
        thunderGradient = static_cast<float>(static_cast<double>(thunderGradient) - 0.01);
    }
    thunderGradient = std::clamp(thunderGradient, 0.0f, 1.0f);
}

void World::tickChunks()
{
    while (chunkCache_ != nullptr && chunkCache_->tick()) {
    }
}

namespace {

bool isInvalidDouble(double value)
{
    return std::isnan(value) || std::isinf(value);
}

} // namespace

void World::updateEntity(Entity* entity, bool requireLoaded, int depth)
{
    if (entity == nullptr) {
        return;
    }

    const int floorX = MathHelper::floor(entity->x);
    const int floorZ = MathHelper::floor(entity->z);
    constexpr int regionRadius = 32;
    if (requireLoaded
        && !isRegionLoaded(floorX - regionRadius, 0, floorZ - regionRadius, floorX + regionRadius, Chunk::height,
            floorZ + regionRadius)) {
        return;
    }

    entity->lastTickX = entity->x;
    entity->lastTickY = entity->y;
    entity->lastTickZ = entity->z;
    entity->prevYaw = entity->yaw;
    entity->prevPitch = entity->pitch;

    if (requireLoaded && entity->isPersistent) {
        if (entity->vehicle != nullptr) {
            entity->tickRiding();
        } else {
            entity->tick();
        }
    }

    if (isInvalidDouble(entity->x)) {
        entity->x = entity->lastTickX;
    }
    if (isInvalidDouble(entity->y)) {
        entity->y = entity->lastTickY;
    }
    if (isInvalidDouble(entity->z)) {
        entity->z = entity->lastTickZ;
    }
    if (isInvalidDouble(entity->pitch)) {
        entity->pitch = entity->prevPitch;
    }
    if (isInvalidDouble(entity->yaw)) {
        entity->yaw = entity->prevYaw;
    }

    const int chunkX = MathHelper::floor(entity->x / 16.0);
    const int chunkSlice = MathHelper::floor(entity->y / 16.0);
    const int chunkZ = MathHelper::floor(entity->z / 16.0);
    if (!entity->isPersistent || entity->chunkX != chunkX || entity->chunkSlice != chunkSlice || entity->chunkZ != chunkZ) {
        if (entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
            getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity, entity->chunkSlice);
        }
        if (hasChunk(chunkX, chunkZ)) {
            entity->isPersistent = true;
            getChunk(chunkX, chunkZ).addEntity(entity);
        } else {
            entity->isPersistent = false;
        }
    }

    if (requireLoaded && entity->isPersistent && entity->passenger != nullptr) {
        if (entity->passenger->dead || entity->passenger->vehicle != entity) {
            entity->passenger->vehicle = nullptr;
            entity->passenger = nullptr;
        } else if (depth < 8) {
            updateEntity(entity->passenger, requireLoaded, depth + 1);
        } else {
            entity->passenger->vehicle = nullptr;
            entity->passenger = nullptr;
        }
    }
}

void World::addEntities(std::vector<Entity*>& entities)
{
    entities_.insert(entities_.end(), entities.begin(), entities.end());
    for (Entity* entity : entities) {
        notifyEntityAdded(entity);
    }
}

void World::unloadEntities(std::vector<Entity*>& entities)
{
    entitiesToUnload_.insert(entitiesToUnload_.end(), entities.begin(), entities.end());
}

void World::processBlockUpdates(const std::vector<block::entity::BlockEntity*>& blockUpdates)
{
    if (processingDeferred_) {
        blockEntityUpdateQueue_.insert(blockEntityUpdateQueue_.end(), blockUpdates.begin(), blockUpdates.end());
    } else {
        for (block::entity::BlockEntity* blockEntity : blockUpdates) {
            if (blockEntity == nullptr || blockEntity->isRemoved()) {
                continue;
            }
            if (std::find(blockEntities.begin(), blockEntities.end(), blockEntity) == blockEntities.end()) {
                blockEntities.push_back(blockEntity);
            }
        }
    }
}

void World::updateSleepingPlayers()
{
    allPlayersSleeping = !players.empty();
    for (PlayerEntity* player : players) {
        if (player == nullptr || player->isSleeping()) {
            continue;
        }
        allPlayersSleeping = false;
        break;
    }
}

bool World::canSkipNight()
{
    if (!allPlayersSleeping || isRemote_) {
        return false;
    }
    for (PlayerEntity* player : players) {
        if (player == nullptr || player->isFullyAsleep()) {
            continue;
        }
        return false;
    }
    return true;
}

void World::afterSkipNight()
{
    allPlayersSleeping = false;
    for (PlayerEntity* player : players) {
        if (player == nullptr || !player->isSleeping()) {
            continue;
        }
        player->wakeUp(false, false, true);
    }
    clearWeather();
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

void World::tickEntities()
{
    for (std::size_t i = 0; i < globalEntities.size(); ++i) {
        Entity* entity = globalEntities[i];
        if (entity == nullptr) {
            continue;
        }
        entity->tick();
        if (!entity->dead) {
            continue;
        }
        globalEntities.erase(globalEntities.begin() + static_cast<std::ptrdiff_t>(i));
        --i;
    }

    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(), [this](Entity* entity) {
            return entity != nullptr && std::find(entitiesToUnload_.begin(), entitiesToUnload_.end(), entity) != entitiesToUnload_.end();
        }),
        entities_.end());

    for (Entity* entity : entitiesToUnload_) {
        if (entity == nullptr) {
            continue;
        }
        if (entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
            getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
        }
    }
    for (Entity* entity : entitiesToUnload_) {
        notifyEntityRemoved(entity);
    }
    entitiesToUnload_.clear();

    for (std::size_t i = 0; i < entities_.size(); ++i) {
        Entity* entity = entities_[i];
        if (entity == nullptr) {
            continue;
        }
        if (entity->vehicle != nullptr) {
            if (!entity->vehicle->dead && entity->vehicle->passenger == entity) {
                continue;
            }
            entity->vehicle->passenger = nullptr;
            entity->vehicle = nullptr;
        }
        if (!entity->dead) {
            updateEntity(entity);
        }
        if (!entity->dead) {
            continue;
        }
        if (entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
            getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
        }
        if (auto* deadPlayer = dynamic_cast<PlayerEntity*>(entity)) {
            players.erase(std::remove(players.begin(), players.end(), deadPlayer), players.end());
            updateSleepingPlayers();
        }
        entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(i));
        notifyEntityRemoved(entity);
        --i;
    }

    processingDeferred_ = true;
    for (std::size_t i = 0; i < blockEntities.size(); ++i) {
        block::entity::BlockEntity* blockEntity = blockEntities[i];
        if (blockEntity == nullptr) {
            continue;
        }
        if (!blockEntity->isRemoved()) {
            blockEntity->tick();
        }
        if (!blockEntity->isRemoved()) {
            continue;
        }
        blockEntities.erase(blockEntities.begin() + static_cast<std::ptrdiff_t>(i));
        --i;
        Chunk& chunk = getChunk(blockEntity->x >> 4, blockEntity->z >> 4);
        chunk.removeBlockEntityAt(blockEntity->x & 0xF, blockEntity->y, blockEntity->z & 0xF);
    }
    processingDeferred_ = false;

    if (!blockEntityUpdateQueue_.empty()) {
        for (block::entity::BlockEntity* blockEntity : blockEntityUpdateQueue_) {
            if (blockEntity == nullptr || blockEntity->isRemoved()) {
                continue;
            }
            if (std::find(blockEntities.begin(), blockEntities.end(), blockEntity) == blockEntities.end()) {
                blockEntities.push_back(blockEntity);
            }
            blockUpdateEvent(blockEntity->x, blockEntity->y, blockEntity->z);
        }
        blockEntityUpdateQueue_.clear();
    }
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

void World::loadChunksNearEntity(Entity* entity)
{
    if (entity == nullptr) {
        return;
    }
    const int chunkX = MathHelper::floor(entity->x / 16.0);
    const int chunkZ = MathHelper::floor(entity->z / 16.0);
    const int radius = chunkPreloadRadius_;
    const int diameter = radius * 2 + 1;
    const int totalSlots = diameter * diameter;
    static int preloadCursor = 0;
    if (preloadCursor >= totalSlots) {
        preloadCursor = 0;
    }
    constexpr int kMaxPreloadsPerCall = 2;
    int preloaded = 0;
    while (preloaded < kMaxPreloadsPerCall && preloadCursor < totalSlots) {
        const int dx = (preloadCursor % diameter) - radius;
        const int dz = (preloadCursor / diameter) - radius;
        ++preloadCursor;
        [[maybe_unused]] Chunk& chunk = getChunk(chunkX + dx, chunkZ + dz);
        ++preloaded;
    }
    if (std::find(entities_.begin(), entities_.end(), entity) == entities_.end()) {
        entities_.push_back(entity);
    }
}

bool World::isTopY(int x, int y, int z) const
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return false;
    }
    if (y < 0) {
        return false;
    }
    if (y >= Chunk::height) {
        return true;
    }
    if (!hasChunk(x >> 4, z >> 4)) {
        return false;
    }
    const Chunk& chunk = getChunk(x >> 4, z >> 4);
    return chunk.isAboveMaxHeight(mod_16(x), y, mod_16(z));
}

void World::setLight(LightType lightType, int x, int y, int z, int value)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return;
    }
    if (y < 0 || y >= Chunk::height) {
        return;
    }
    if (!hasChunk(x >> 4, z >> 4)) {
        return;
    }
    Chunk& chunk = getChunk(x >> 4, z >> 4);
    const int localX = mod_16(x);
    const int localZ = mod_16(z);
    const int previous = chunk.getLight(lightType, localX, y, localZ);
    if (previous == value) {
        return;
    }
    chunk.setLight(lightType, localX, y, localZ, value);
    blockUpdateEvent(x, y, z);
}

void World::updateLight(LightType lightType, int x, int y, int z, int lightLevel)
{
    if (dimension != nullptr && dimension->hasCeiling && lightType == LightType::Sky) {
        return;
    }
    if (!isPosLoaded(x, y, z)) {
        return;
    }
    if (lightType == LightType::Sky) {
        if (isTopY(x, y, z)) {
            lightLevel = 15;
        }
    } else if (lightType == LightType::Block) {
        const int blockId = getBlockId(x, y, z);
        const int luminance = Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(blockId)];
        if (luminance > lightLevel) {
            lightLevel = luminance;
        }
    }
    if (getBrightness(lightType, x, y, z) != lightLevel) {
        queueLightUpdate(lightType, x, y, z, x, y, z);
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

    ++lightingQueueCount_;
    try {
        if (lightingQueueCount_ == 50) {
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

        const std::size_t queueSize = lightingQueue_.size();
        if (merge) {
            constexpr int mergeBudget = 5;
            const int budget = queueSize < static_cast<std::size_t>(mergeBudget) ? static_cast<int>(queueSize) : mergeBudget;
            for (int i = 0; i < budget; ++i) {
                LightUpdate& existing = lightingQueue_[lightingQueue_.size() - static_cast<std::size_t>(i) - 1];
                if (existing.lightType == type && existing.expand(minX, minY, minZ, maxX, maxY, maxZ)) {
                    return;
                }
            }
        }

        lightingQueue_.emplace_back(type, minX, minY, minZ, maxX, maxY, maxZ);
        constexpr std::size_t maxQueueSize = 1000000;
        if (lightingQueue_.size() > maxQueueSize) {
            lightingQueue_.clear();
        }
    } catch (...) {
        --lightingQueueCount_;
        throw;
    }
    --lightingQueueCount_;
}

void World::scheduleBlockUpdate(int x, int y, int z, int id, int tickRate)
{
    BlockEvent blockEvent(x, y, z, id);
    constexpr int regionPadding = 8;
    if (instantBlockUpdateEnabled) {
        if (isRegionLoaded(blockEvent.x - regionPadding, blockEvent.y - regionPadding, blockEvent.z - regionPadding,
                blockEvent.x + regionPadding, blockEvent.y + regionPadding, blockEvent.z + regionPadding)) {
            const int currentId = getBlockId(blockEvent.x, blockEvent.y, blockEvent.z);
            if (currentId == blockEvent.blockId && currentId > 0) {
                Block* block = Block::BLOCKS[static_cast<std::size_t>(currentId)];
                if (block != nullptr) {
                    block->onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, random_);
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

    if (!scheduledUpdateSet_.contains(blockEvent)) {
        scheduledUpdateSet_.insert(blockEvent);
        scheduledUpdates_.insert(blockEvent);
    }
}

bool World::processScheduledTicks(bool flush)
{
    int budget = static_cast<int>(scheduledUpdates_.size());
    if (budget != static_cast<int>(scheduledUpdateSet_.size())) {
        throw std::runtime_error("TickNextTick list out of synch");
    }
    if (budget > 1000) {
        budget = 1000;
    }

    for (int i = 0; i < budget; ++i) {
        if (scheduledUpdates_.empty()) {
            break;
        }
        BlockEvent blockEvent = *scheduledUpdates_.begin();
        const long long currentTime = static_cast<long long>(hasStorageBackedProperties_ ? properties_.getTime() : time_);
        if (!flush && blockEvent.ticks > currentTime) {
            break;
        }

        scheduledUpdates_.erase(scheduledUpdates_.begin());
        scheduledUpdateSet_.erase(blockEvent);

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
            block->onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, random_);
        }
    }
    return !scheduledUpdates_.empty();
}

void World::manageChunkUpdatesAndEvents()
{
    activeChunks_.clear();
    for (PlayerEntity* player : players) {
        if (player == nullptr) {
            continue;
        }
        const int chunkX = MathHelper::floor(player->x / 16.0);
        const int chunkZ = MathHelper::floor(player->z / 16.0);
        constexpr int radius = 9;
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                activeChunks_.insert(ChunkPos{dx + chunkX, dz + chunkZ});
            }
        }
    }

    if (ambientSoundCounter_ > 0) {
        --ambientSoundCounter_;
    }

    for (const ChunkPos& chunkPos : activeChunks_) {
        if (chunkCache_ != nullptr && !chunkCache_->isChunkLoaded(chunkPos.x, chunkPos.z)) {
            continue;
        }
        const int chunkOriginX = chunkPos.x * 16;
        const int chunkOriginZ = chunkPos.z * 16;
        Chunk& chunk = getChunk(chunkPos.x, chunkPos.z);

        if (ambientSoundCounter_ == 0) {
            lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
            const int randomValue = lcgBlockSeed_ >> 2;
            const int localX = randomValue & 0xF;
            const int localZ = (randomValue >> 8) & 0xF;
            const int localY = (randomValue >> 16) & 0x7F;
            const int blockId = chunk.getBlockId(localX, localY, localZ);
            const int worldX = localX + chunkOriginX;
            const int worldZ = localZ + chunkOriginZ;
            if (blockId == 0 && getBrightness(worldX, localY, worldZ) <= random_.nextInt(8)
                && getBrightness(LightType::Sky, worldX, localY, worldZ) <= 0) {
                if (PlayerEntity* closest = getClosestPlayer(static_cast<double>(worldX) + 0.5,
                        static_cast<double>(localY) + 0.5, static_cast<double>(worldZ) + 0.5, 8.0)) {
                    if (closest->getSquaredDistance(static_cast<double>(worldX) + 0.5,
                            static_cast<double>(localY) + 0.5, static_cast<double>(worldZ) + 0.5) > 4.0) {
                        playSound(static_cast<double>(worldX) + 0.5, static_cast<double>(localY) + 0.5,
                            static_cast<double>(worldZ) + 0.5, "ambient.cave.cave", 0.7f,
                            0.8f + random_.nextFloat() * 0.2f);
                        ambientSoundCounter_ = random_.nextInt(12000) + 6000;
                    }
                }
            }
        }

        if (random_.nextInt(100000) == 0 && isRaining() && isThundering()) {
            lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
            const int randomValue = lcgBlockSeed_ >> 2;
            const int worldX = chunkOriginX + (randomValue & 0xF);
            const int worldZ = chunkOriginZ + ((randomValue >> 8) & 0xF);
            const int worldY = getTopSolidBlockY(worldX, worldZ);
            if (isRaining(worldX, worldY, worldZ)) {
                spawnGlobalEntity(new LightningEntity(this, worldX, worldY, worldZ));
                ticksSinceLightning_ = 2;
            }
        }

        if (random_.nextInt(16) == 0) {
            lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
            const int randomValue = lcgBlockSeed_ >> 2;
            const int localX = randomValue & 0xF;
            const int localZ = (randomValue >> 8) & 0xF;
            const int worldX = localX + chunkOriginX;
            const int worldZ = localZ + chunkOriginZ;
            const int worldY = getTopSolidBlockY(worldX, worldZ);
            if (getBiomeSource() != nullptr
                && Biomes::byId(getBiomeSource()->getBiome(worldX, worldZ).id).canSnow() && worldY >= 0
                && worldY < Chunk::height && chunk.getLight(LightType::Block, localX, worldY, localZ) < 10) {
                const int belowId = chunk.getBlockId(localX, worldY - 1, localZ);
                const int currentId = chunk.getBlockId(localX, worldY, localZ);
                if (isRaining() && currentId == 0 && Block::SNOW != nullptr
                    && Block::SNOW->canPlaceAt(this, worldX, worldY, worldZ) && belowId != 0
                    && Block::BLOCKS[static_cast<std::size_t>(belowId)] != nullptr
                    && Block::BLOCKS[static_cast<std::size_t>(belowId)]->material.blocksMovement()) {
                    setBlock(worldX, worldY, worldZ, Block::SNOW->id);
                }
                if (Block::WATER != nullptr && belowId == Block::WATER->id && chunk.getBlockMeta(localX, worldY - 1, localZ) == 0) {
                    setBlock(worldX, worldY - 1, worldZ, Block::ICE->id);
                }
            }
        }

        for (int tickIndex = 0; tickIndex < 80; ++tickIndex) {
            lcgBlockSeed_ = lcgBlockSeed_ * 3 + 1013904223;
            const int randomValue = lcgBlockSeed_ >> 2;
            const int localX = randomValue & 0xF;
            const int localZ = (randomValue >> 8) & 0xF;
            const int localY = (randomValue >> 16) & 0x7F;
            const int blockId = chunk.getBlockId(localX, localY, localZ);
            if (blockId <= 0 || blockId >= Block::BLOCK_COUNT || !Block::BLOCKS_RANDOM_TICK[static_cast<std::size_t>(blockId)]) {
                continue;
            }
            Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
            if (block != nullptr) {
                block->onTick(this, localX + chunkOriginX, localY, localZ + chunkOriginZ, random_);
            }
        }
    }
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

float World::getVisibilityRatio(const Vec3d& vec, const Box& box) const
{
    const double stepX = 1.0 / ((box.maxX - box.minX) * 2.0 + 1.0);
    const double stepY = 1.0 / ((box.maxY - box.minY) * 2.0 + 1.0);
    const double stepZ = 1.0 / ((box.maxZ - box.minZ) * 2.0 + 1.0);
    int visible = 0;
    int total = 0;
    for (float fx = 0.0f; fx <= 1.0f; fx = static_cast<float>(static_cast<double>(fx) + stepX)) {
        for (float fy = 0.0f; fy <= 1.0f; fy = static_cast<float>(static_cast<double>(fy) + stepY)) {
            for (float fz = 0.0f; fz <= 1.0f; fz = static_cast<float>(static_cast<double>(fz) + stepZ)) {
                const double sampleX = box.minX + (box.maxX - box.minX) * static_cast<double>(fx);
                const double sampleY = box.minY + (box.maxY - box.minY) * static_cast<double>(fy);
                const double sampleZ = box.minZ + (box.maxZ - box.minZ) * static_cast<double>(fz);
                if (!raycast(Vec3d{sampleX, sampleY, sampleZ}, vec).has_value()) {
                    ++visible;
                }
                ++total;
            }
        }
    }
    if (total == 0) {
        return 0.0f;
    }
    return static_cast<float>(visible) / static_cast<float>(total);
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

void World::tick()
{
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
    time_ = nextTime;
    if (hasStorageBackedProperties_) {
        properties_.setTime(nextTime);
    }
    updateSkyBrightness();
    processScheduledTicks(false);
    manageChunkUpdatesAndEvents();
}

} // namespace net::minecraft
