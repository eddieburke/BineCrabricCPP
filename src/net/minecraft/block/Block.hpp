#pragma once
// Faithful 1:1 port of net.minecraft.block.Block (beta 1.7.3) as a polymorphic
// base class with virtual behavior, replacing the earlier flat data-table.
//
// Cycle handling (mirrors the Item/ItemStack pattern): bodies that reach into
// World / PlayerEntity / Entity / ItemStack / ItemEntity / Stats are declared
// here and defined out-of-line in Block.cpp. Trivial accessors stay inline.
//
// Static registries mirror Java's parallel arrays.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "net/minecraft/block/BlockTypes.hpp"  // net::minecraft::Block alias (was Block.hpp EOF; moved to hub in M4c)
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/sound/BlockSoundGroup.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/Types.hpp"  // Box, Vec3d, JavaRandom

namespace net::minecraft {
class BlockView;
class World;
class ItemStack;
}  // namespace net::minecraft

namespace net::minecraft::block {
// UV bounds for a 16x16 tile in /terrain.png (beta atlas index).
struct TerrainAtlasUv {
    double uMin = 0.0;
    double uMax = 0.0;
    double vMin = 0.0;
    double vMax = 0.0;
};

class Block {
   public:
    using Material = material::Material;
    static constexpr int BLOCK_COUNT = 256;
    static constexpr int FACE_BOTTOM = 0;
    static constexpr int FACE_TOP = 1;
    static constexpr int FACE_EAST = 2;
    static constexpr int FACE_WEST = 3;
    static constexpr int FACE_NORTH = 4;
    static constexpr int FACE_SOUTH = 5;

    [[nodiscard]] static int textureAtlasU(int textureId) {
        return (textureId & 0xF) << 4;
    }

    [[nodiscard]] static int textureAtlasV(int textureId) {
        return textureId & 0xF0;
    }

    [[nodiscard]] static TerrainAtlasUv terrainTileUv(int textureId);
    [[nodiscard]] static TerrainAtlasUv terrainStripUv(int textureId, double scrollU, double stripHeight = 4.0);
    // Per-face tile selection shared by directional blocks (furnace, dispenser, chest, ...).
    // Pass -1 for bottom/top/front to fall back to sides.
    [[nodiscard]] static int textureForSide(
        int side, int sides, int bottom = -1, int top = -1, int frontSide = -1, int front = -1);
    // Java's parallel static arrays.
    static std::array<Block*, BLOCK_COUNT> BLOCKS;
    static std::array<bool, BLOCK_COUNT> BLOCKS_RANDOM_TICK;
    static std::array<bool, BLOCK_COUNT> BLOCKS_OPAQUE;
    static std::array<bool, BLOCK_COUNT> BLOCKS_WITH_ENTITY;
    static std::array<int, BLOCK_COUNT> BLOCKS_LIGHT_OPACITY;
    static std::array<bool, BLOCK_COUNT> BLOCKS_ALLOW_VISION;
    static std::array<int, BLOCK_COUNT> BLOCKS_LIGHT_LUMINANCE;
    static std::array<bool, BLOCK_COUNT> BLOCKS_IGNORE_META_UPDATE;
    // Java's named static block instances. Populated from BLOCKS[id] once
    // Registry::bootstrap() has run.
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* PLANKS;
    static Block* SAPLING;
    static Block* BEDROCK;
    static Block* FLOWING_WATER;
    static Block* WATER;
    static Block* FLOWING_LAVA;
    static Block* LAVA;
    static Block* SAND;
    static Block* GRAVEL;
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* LOG;
    static Block* LEAVES;
    static Block* SPONGE;
    static Block* GLASS;
    static Block* LAPIS_ORE;
    static Block* LAPIS_BLOCK;
    static Block* DISPENSER;
    static Block* SANDSTONE;
    static Block* NOTE_BLOCK;
    static Block* BED;
    static Block* POWERED_RAIL;
    static Block* DETECTOR_RAIL;
    static Block* STICKY_PISTON;
    static Block* COBWEB;
    static Block* GRASS;
    static Block* DEAD_BUSH;
    static Block* PISTON;
    static Block* PISTON_HEAD;
    static Block* WOOL;
    static Block* MOVING_PISTON;
    static Block* DANDELION;
    static Block* ROSE;
    static Block* BROWN_MUSHROOM;
    static Block* RED_MUSHROOM;
    static Block* GOLD_BLOCK;
    static Block* IRON_BLOCK;
    static Block* DOUBLE_SLAB;
    static Block* SLAB;
    static Block* BRICKS;
    static Block* TNT;
    static Block* BOOKSHELF;
    static Block* MOSSY_COBBLESTONE;
    static Block* OBSIDIAN;
    static Block* TORCH;
    static Block* FIRE;
    static Block* SPAWNER;
    static Block* WOODEN_STAIRS;
    static Block* CHEST;
    static Block* REDSTONE_WIRE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;
    static Block* CRAFTING_TABLE;
    static Block* WHEAT;
    static Block* FARMLAND;
    static Block* FURNACE;
    static Block* LIT_FURNACE;
    static Block* SIGN;
    static Block* DOOR;
    static Block* LADDER;
    static Block* RAIL;
    static Block* COBBLESTONE_STAIRS;
    static Block* WALL_SIGN;
    static Block* LEVER;
    static Block* STONE_PRESSURE_PLATE;
    static Block* IRON_DOOR;
    static Block* WOODEN_PRESSURE_PLATE;
    static Block* REDSTONE_ORE;
    static Block* LIT_REDSTONE_ORE;
    static Block* REDSTONE_TORCH;
    static Block* LIT_REDSTONE_TORCH;
    static Block* BUTTON;
    static Block* SNOW;
    static Block* ICE;
    static Block* SNOW_BLOCK;
    static Block* CACTUS;
    static Block* CLAY;
    static Block* SUGAR_CANE;
    static Block* JUKEBOX;
    static Block* FENCE;
    static Block* PUMPKIN;
    static Block* NETHERRACK;
    static Block* SOUL_SAND;
    static Block* GLOWSTONE;
    static Block* NETHER_PORTAL;
    static Block* JACK_O_LANTERN;
    static Block* CAKE;
    static Block* REPEATER;
    static Block* POWERED_REPEATER;
    static Block* LOCKED_CHEST;
    static Block* TRAPDOOR;
    static net::minecraft::BlockSoundGroup DEFAULT_SOUND_GROUP;
    // Beta 1.7.3: glass-family blocks step on stone, break with random.glass.
    static net::minecraft::BlockSoundGroup GLASS_BLOCK_SOUNDS;
    // Beta 1.7.3: sand-family blocks step on sand, break with gravel.
    static net::minecraft::BlockSoundGroup SAND_BLOCK_SOUNDS;
    // --- instance fields (public, faithful to Java) ---
    int textureId = 0;
    const int id;
    double minX = 0.0, minY = 0.0, minZ = 0.0;
    double maxX = 0.0, maxY = 0.0, maxZ = 0.0;
    net::minecraft::BlockSoundGroup* soundGroup = nullptr;
    float particleFallSpeedModifier = 1.0f;
    Material& material;
    float slipperiness = 0.6f;
    // --- constructors ---
    Block(int id, Material& material);
    Block(int id, int textureId, Material& material);
    virtual ~Block() = default;

    // --- builder-style setters (return Block* for chaining) ---
    Block* setSoundGroup(net::minecraft::BlockSoundGroup* group) {
        soundGroup = group;
        return this;
    }

    // Step/break sounds: set in registerClass() via setSoundGroup(&localSoundGroup).
    [[nodiscard]] net::minecraft::BlockSoundGroup* getSoundGroup() const {
        return soundGroup != nullptr ? soundGroup : &DEFAULT_SOUND_GROUP;
    }

    [[nodiscard]] virtual std::string getStepSound() const {
        return getSoundGroup()->getStepSound();
    }

    [[nodiscard]] virtual std::string getMiningSound() const {
        return getSoundGroup()->getMiningSound();
    }

    [[nodiscard]] virtual std::string getBreakSound() const {
        return getSoundGroup()->getBreakSound();
    }

    Block* setOpacity(int i);
    Block* setLuminance(float fractionalValue);
    Block* setResistance(float resistance);
    Block* setHardness(float hardness);
    Block* setUnbreakable();
    Block* setTickRandomly(bool value);
    Block* ignoreMetaUpdates();
    Block* disableTrackingStatistics();
    Block* setTranslationKey(const std::string& name);
    void setBoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

    void setBoundingBox(const net::minecraft::Box& box) {
        minX = box.minX;
        minY = box.minY;
        minZ = box.minZ;
        maxX = box.maxX;
        maxY = box.maxY;
        maxZ = box.maxZ;
    }

    // --- simple/virtual queries (faithful defaults inline) ---
    [[nodiscard]] virtual bool isFullCube() const {
        return true;
    }

    [[nodiscard]] virtual int getRenderType() const {
        return 0;
    }

    [[nodiscard]] float getHardness() const {
        return hardness_;
    }

    [[nodiscard]] float getResistance() const {
        return resistance_;
    }

    [[nodiscard]] virtual bool isOpaque() const {
        return true;
    }

    [[nodiscard]] virtual bool hasCollision() const {
        return true;
    }

    [[nodiscard]] virtual bool hasCollision(int /*meta*/, bool /*allowLiquids*/) const {
        return hasCollision();
    }

    [[nodiscard]] virtual int getTexture(int /*side*/) const {
        return textureId;
    }

    [[nodiscard]] virtual int getTexture(int side, int /*meta*/) const {
        return getTexture(side);
    }

    [[nodiscard]] virtual int getTickRate() const {
        return 10;
    }

    [[nodiscard]] virtual int getDroppedItemCount(net::minecraft::JavaRandom& /*random*/) const {
        return 1;
    }

    [[nodiscard]] virtual int getDroppedItemId(int /*blockMeta*/, net::minecraft::JavaRandom& /*random*/) const {
        return id;
    }

    [[nodiscard]] virtual int getRenderLayer() const {
        return 0;
    }

    [[nodiscard]] virtual int getColor(int /*meta*/) const {
        return 0xFFFFFF;
    }

    [[nodiscard]] virtual bool canEmitRedstonePower() const {
        return false;
    }

    [[nodiscard]] virtual bool isEmittingRedstonePowerInDirection(
        const net::minecraft::BlockView* /*blockView*/, int /*x*/, int /*y*/, int /*z*/, int /*direction*/) const {
        return false;
    }

    [[nodiscard]] virtual bool canTransferPowerInDirection(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*direction*/) const {
        return false;
    }

    [[nodiscard]] bool isTrackingStatistics() const {
        return shouldTrackStatistics_;
    }

    [[nodiscard]] virtual int getPistonBehavior() const {
        return material.getPistonBehavior();
    }

    [[nodiscard]] const std::string& getTranslationKey() const {
        return translationKey_;
    }

    [[nodiscard]] std::string getTranslatedName() const;

    [[nodiscard]] net::minecraft::Box getCollisionShapeLocal() const {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }

    // --- bounding boxes (need net::minecraft::World for offset; bodies in .cpp) ---
    [[nodiscard]] virtual net::minecraft::Box getBoundingBox(net::minecraft::World* world, int x, int y, int z) const;
    [[nodiscard]] virtual std::optional<net::minecraft::Box> getCollisionShape(net::minecraft::World* world,
                                                                               int x,
                                                                               int y,
                                                                               int z) const;
    virtual void addIntersectingBoundingBox(net::minecraft::World* world,
                                            int x,
                                            int y,
                                            int z,
                                            const net::minecraft::Box& box,
                                            std::vector<Box>& boxes) const;

    // --- behavior hooks (faithful empty defaults; overridden by subclasses) ---
    virtual void init() {
    }

    virtual bool hasItemOverride() const {
        return false;
    }

    virtual void registerBlockItem();

    virtual void onTick(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::JavaRandom& /*random*/) {
    }

    virtual void randomDisplayTick(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::JavaRandom& /*random*/) {
    }

    virtual void onMetadataChange(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*meta*/) {
    }

    virtual void neighborUpdate(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*id*/) {
    }

    virtual void onPlaced(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/) {
    }

    virtual void onPlaced(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*direction*/) {
    }

    virtual void onPlaced(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::PlayerEntity* /*player*/) {
    }

    virtual void onBreak(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/) {
    }

    virtual bool onUse(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::PlayerEntity* /*player*/) {
        return false;
    }

    virtual void onSteppedOn(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::Entity* /*entity*/) {
    }

    virtual void onEntityCollision(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::Entity* /*entity*/) {
    }

    virtual void onBlockBreakStart(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::PlayerEntity* /*player*/) {
    }

    virtual void applyVelocity(net::minecraft::World* /*world*/,
                               int /*x*/,
                               int /*y*/,
                               int /*z*/,
                               net::minecraft::Entity* /*entity*/,
                               net::minecraft::Vec3d& /*velocity*/) {
    }

    virtual void afterBreak(
        net::minecraft::World* world, net::minecraft::PlayerEntity* player, int x, int y, int z, int meta);

    virtual void onDestroyedByExplosion(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/) {
    }

    virtual void onBlockAction(
        net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/, int /*data1*/, int /*data2*/) {
    }

    virtual void onLightChanged(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/) {
    }

    [[nodiscard]] virtual bool canGrow(net::minecraft::World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const {
        return true;
    }

    // --- methods needing World/ItemStack/net::minecraft::Entity (bodies in .cpp) ---
    [[nodiscard]] virtual bool canPlaceAt(net::minecraft::World* world, int x, int y, int z) const;
    [[nodiscard]] virtual bool canPlaceAt(net::minecraft::World* world, int x, int y, int z, int side) const;
    void dropStacks(net::minecraft::World* world, int x, int y, int z, int meta);
    virtual void dropStacks(net::minecraft::World* world, int x, int y, int z, int meta, float luck);
    [[nodiscard]] virtual float getBlastResistance(net::minecraft::Entity* entity) const;
    [[nodiscard]] virtual float getHardness(net::minecraft::PlayerEntity* player) const;
    [[nodiscard]] virtual std::optional<net::minecraft::HitResult> raycast(net::minecraft::World* world,
                                                                           int x,
                                                                           int y,
                                                                           int z,
                                                                           net::minecraft::Vec3d startPos,
                                                                           net::minecraft::Vec3d endPos) const;
    [[nodiscard]] static std::optional<net::minecraft::HitResult> raycastLocalBounds(double localMinX,
                                                                                     double localMinY,
                                                                                     double localMinZ,
                                                                                     double localMaxX,
                                                                                     double localMaxY,
                                                                                     double localMaxZ,
                                                                                     int x,
                                                                                     int y,
                                                                                     int z,
                                                                                     net::minecraft::Vec3d startPos,
                                                                                     net::minecraft::Vec3d endPos);
    // Slabs, farmland, stairs, and trapdoors store unreliable light in their own cell.
    [[nodiscard]] static bool usesNeighborLightSampling(int blockId);
    // Client rendering (net.minecraft.block.Block @Environment CLIENT).
    [[nodiscard]] virtual float getLuminance(const net::minecraft::BlockView* blockView, int x, int y, int z) const;
    [[nodiscard]] virtual bool isSideVisible(
        const net::minecraft::BlockView* blockView, int x, int y, int z, int side) const;
    // Render-path culling: inset checks use caller-supplied bounds instead of the
    // Block singleton so chunk mesh workers never read mutated minX..maxZ.
    [[nodiscard]] bool isSideVisibleForBounds(const net::minecraft::BlockView* blockView,
                                              int x,
                                              int y,
                                              int z,
                                              int side,
                                              const net::minecraft::Box& bounds) const;
    [[nodiscard]] virtual int getTextureId(
        const net::minecraft::BlockView* blockView, int x, int y, int z, int side) const;
    virtual void updateBoundingBox(const net::minecraft::BlockView* blockView, int x, int y, int z);

    // Render-path bounds in local block space, computed without mutating any
    // shared Block state so chunk meshing can run on worker threads. Default:
    // the block's static bounds (constructor or setupRenderBoundingBox at registry
    // init via finalizeBlockRegistryProperties). State-dependent blocks override.
    [[nodiscard]] virtual net::minecraft::Box getRenderBounds(const net::minecraft::BlockView* /*blockView*/,
                                                              int /*x*/,
                                                              int /*y*/,
                                                              int /*z*/) const {
        return {minX, minY, minZ, maxX, maxY, maxZ};
    }

    [[nodiscard]] virtual int getColorMultiplier(const net::minecraft::BlockView* blockView, int x, int y, int z) const;
    virtual void setupRenderBoundingBox();

   protected:
    [[nodiscard]] virtual int getDroppedItemMeta(int /*blockMeta*/) const {
        return 0;
    }

    void dropStack(net::minecraft::World* world, int x, int y, int z, const net::minecraft::ItemStack& stack);
    float hardness_ = 0.0f;
    float resistance_ = 0.0f;
    bool shouldTrackStatistics_ = true;
    std::string translationKey_;

   private:
    [[nodiscard]] bool containsInYZPlane(const std::optional<Vec3d>& v) const;
    [[nodiscard]] bool containsInXZPlane(const std::optional<Vec3d>& v) const;
    [[nodiscard]] bool containsInXYPlane(const std::optional<Vec3d>& v) const;
};

// Triggers Registry::bootstrap() (std::call_once; idempotent). Safe from any entry point.
// Call sites: Minecraft ctor/init (client/Minecraft.cpp), World ctors (world/World.cpp),
// Chunk/BlockSource/AlphaChunkStorage (world/chunk/**), runtime world probes,
// tests/block_registry_properties_test.cpp.
void initializeBlocks();
// Recomputes registry fields that Java initializes through virtual dispatch
// from the Block constructor. C++ cannot dispatch to derived overrides during
// base construction, so call this after all vanilla block instances and chained
// setters have run.
void finalizeBlockRegistryProperties();
}  // namespace net::minecraft::block
