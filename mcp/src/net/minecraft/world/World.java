/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.TreeSet;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.ai.pathing.Path;
import net.minecraft.entity.ai.pathing.PathNodeNavigator;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.BlockEvent;
import net.minecraft.world.BlockView;
import net.minecraft.world.LightType;
import net.minecraft.world.NaturalSpawner;
import net.minecraft.world.PersistentState;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.WorldRegion;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.source.BiomeSource;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkCache;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.LegacyChunkCache;
import net.minecraft.world.chunk.light.LightUpdate;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.event.listener.GameEventListener;
import net.minecraft.world.explosion.Explosion;
import net.minecraft.world.storage.SavedDataStorage;
import net.minecraft.world.storage.WorldStorage;

public class World
implements BlockView {
    public boolean instantBlockUpdateEnabled = false;
    private List lightingQueue = new ArrayList();
    public List entities = new ArrayList();
    private List entitiesToUnload = new ArrayList();
    private TreeSet scheduledUpdates = new TreeSet();
    private Set scheduledUpdateSet = new HashSet();
    public List blockEntities = new ArrayList();
    private List blockEntityUpdateQueue = new ArrayList();
    public List players = new ArrayList();
    public List globalEntities = new ArrayList();
    private long worldTimeMask = 0xFFFFFFL;
    public int ambientDarkness = 0;
    protected int lcgBlockSeed = new Random().nextInt();
    protected final int lcgBlockSeedIncrement = 1013904223;
    protected float rainGradientPrev;
    protected float rainGradient;
    protected float thunderGradientPrev;
    protected float thunderGradient;
    protected int ticksSinceLightning = 0;
    public int lightningTicksLeft = 0;
    public boolean pauseTicking = false;
    private long timeLoaded = System.currentTimeMillis();
    protected int saveInterval = 40;
    public int difficulty;
    public Random random = new Random();
    public boolean newWorld = false;
    public final Dimension dimension;
    protected List eventListeners = new ArrayList();
    protected ChunkSource chunkSource;
    protected final WorldStorage dimensionData;
    protected WorldProperties properties;
    public boolean eventProcessingEnabled;
    private boolean allPlayersSleeping;
    public SavedDataStorage persistentStateManager;
    private ArrayList tempCollisionBoxes = new ArrayList();
    private boolean processingDeferred;
    private int lightingUpdateCount = 0;
    private boolean allowMonsterSpawning = true;
    private boolean allowMobSpawning = true;
    static int lightingQueueCount = 0;
    private Set activeChunks = new HashSet();
    private int ambientSoundCounter = this.random.nextInt(12000);
    private List tempEntityList = new ArrayList();
    public boolean isRemote = false;

    public BiomeSource getBiomeSource() {
        return this.dimension.biomeSource;
    }

    @Environment(value=EnvType.CLIENT)
    public World(WorldStorage dimensionData, String name, Dimension dimension, long seed) {
        this.dimensionData = dimensionData;
        this.properties = new WorldProperties(seed, name);
        this.dimension = dimension;
        this.persistentStateManager = new SavedDataStorage(dimensionData);
        dimension.setWorld(this);
        this.chunkSource = this.createChunkCache();
        this.updateSkyBrightness();
        this.prepareWeather();
    }

    @Environment(value=EnvType.CLIENT)
    public World(World world, Dimension dimension) {
        this.timeLoaded = world.timeLoaded;
        this.dimensionData = world.dimensionData;
        this.properties = new WorldProperties(world.properties);
        this.persistentStateManager = new SavedDataStorage(this.dimensionData);
        this.dimension = dimension;
        dimension.setWorld(this);
        this.chunkSource = this.createChunkCache();
        this.updateSkyBrightness();
        this.prepareWeather();
    }

    @Environment(value=EnvType.CLIENT)
    public World(WorldStorage dimensionData, String name, long seed) {
        this(dimensionData, name, seed, null);
    }

    public World(WorldStorage dimensionData, String name, long seed, Dimension dimension) {
        this.dimensionData = dimensionData;
        this.persistentStateManager = new SavedDataStorage(dimensionData);
        this.properties = dimensionData.loadProperties();
        boolean bl = this.newWorld = this.properties == null;
        this.dimension = dimension != null ? dimension : (this.properties != null && this.properties.getDimensionId() == -1 ? Dimension.fromId(-1) : Dimension.fromId(0));
        boolean bl2 = false;
        if (this.properties == null) {
            this.properties = new WorldProperties(seed, name);
            bl2 = true;
        } else {
            this.properties.setName(name);
        }
        this.dimension.setWorld(this);
        this.chunkSource = this.createChunkCache();
        if (bl2) {
            this.initializeSpawnPoint();
        }
        this.updateSkyBrightness();
        this.prepareWeather();
    }

    protected ChunkSource createChunkCache() {
        ChunkStorage chunkStorage = this.dimensionData.getChunkStorage(this.dimension);
        return new ChunkCache(this, chunkStorage, this.dimension.createChunkGenerator());
    }

    protected void initializeSpawnPoint() {
        this.eventProcessingEnabled = true;
        int n = 0;
        int n2 = 64;
        int n3 = 0;
        while (!this.dimension.isValidSpawnPoint(n, n3)) {
            n += this.random.nextInt(64) - this.random.nextInt(64);
            n3 += this.random.nextInt(64) - this.random.nextInt(64);
        }
        this.properties.setSpawn(n, n2, n3);
        this.eventProcessingEnabled = false;
    }

    @Environment(value=EnvType.CLIENT)
    public void updateSpawnPosition() {
        if (this.properties.getSpawnY() <= 0) {
            this.properties.setSpawnY(64);
        }
        int n = this.properties.getSpawnX();
        int n2 = this.properties.getSpawnZ();
        while (this.getSpawnBlockId(n, n2) == 0) {
            n += this.random.nextInt(8) - this.random.nextInt(8);
            n2 += this.random.nextInt(8) - this.random.nextInt(8);
        }
        this.properties.setSpawnX(n);
        this.properties.setSpawnZ(n2);
    }

    public int getSpawnBlockId(int x, int z) {
        int n = 63;
        while (!this.isAir(x, n + 1, z)) {
            ++n;
        }
        return this.getBlockId(x, n, z);
    }

    @Environment(value=EnvType.CLIENT)
    public void saveWorldData() {
    }

    @Environment(value=EnvType.CLIENT)
    public void addPlayer(PlayerEntity player) {
        try {
            NbtCompound nbtCompound = this.properties.getPlayerNbt();
            if (nbtCompound != null) {
                player.read(nbtCompound);
                this.properties.setPlayerNbt(null);
            }
            if (this.chunkSource instanceof LegacyChunkCache) {
                LegacyChunkCache legacyChunkCache = (LegacyChunkCache)this.chunkSource;
                int n = MathHelper.floor((int)player.x) >> 4;
                int n2 = MathHelper.floor((int)player.z) >> 4;
                legacyChunkCache.setSpawnPoint(n, n2);
            }
            this.spawnEntity(player);
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public void saveWithLoadingDisplay(boolean bl, LoadingDisplay arg) {
        if (!this.chunkSource.canSave()) {
            return;
        }
        if (arg != null) {
            arg.progressStartNoAbort("Saving level");
        }
        this.save();
        if (arg != null) {
            arg.progressStage("Saving chunks");
        }
        this.chunkSource.save(bl, arg);
    }

    private void save() {
        this.checkSessionLock();
        this.dimensionData.save(this.properties, this.players);
        this.persistentStateManager.save();
    }

    @Environment(value=EnvType.CLIENT)
    public boolean attemptSaving(int i) {
        if (!this.chunkSource.canSave()) {
            return true;
        }
        if (i == 0) {
            this.save();
        }
        return this.chunkSource.save(false, null);
    }

    public int getBlockId(int x, int y, int z) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            return 0;
        }
        return this.getChunk(x >> 4, z >> 4).getBlockId(x & 0xF, y, z & 0xF);
    }

    public boolean isAir(int x, int y, int z) {
        return this.getBlockId(x, y, z) == 0;
    }

    public boolean isPosLoaded(int x, int y, int z) {
        if (y < 0 || y >= 128) {
            return false;
        }
        return this.hasChunk(x >> 4, z >> 4);
    }

    public boolean isRegionLoaded(int x, int y, int z, int range) {
        return this.isRegionLoaded(x - range, y - range, z - range, x + range, y + range, z + range);
    }

    public boolean isRegionLoaded(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        if (maxY < 0 || minY >= 128) {
            return false;
        }
        minX >>= 4;
        minY >>= 4;
        minZ >>= 4;
        maxX >>= 4;
        maxY >>= 4;
        maxZ >>= 4;
        for (int i = minX; i <= maxX; ++i) {
            for (int j = minZ; j <= maxZ; ++j) {
                if (this.hasChunk(i, j)) continue;
                return false;
            }
        }
        return true;
    }

    private boolean hasChunk(int x, int z) {
        return this.chunkSource.isChunkLoaded(x, z);
    }

    public Chunk getChunkFromPos(int x, int z) {
        return this.getChunk(x >> 4, z >> 4);
    }

    public Chunk getChunk(int chunkX, int chunkZ) {
        return this.chunkSource.getChunk(chunkX, chunkZ);
    }

    public boolean setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return false;
        }
        if (y < 0) {
            return false;
        }
        if (y >= 128) {
            return false;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.setBlock(x & 0xF, y, z & 0xF, blockId, meta);
    }

    public boolean setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return false;
        }
        if (y < 0) {
            return false;
        }
        if (y >= 128) {
            return false;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.setBlock(x & 0xF, y, z & 0xF, blockId);
    }

    public Material getMaterial(int x, int y, int z) {
        int n = this.getBlockId(x, y, z);
        if (n == 0) {
            return Material.AIR;
        }
        return Block.BLOCKS[n].material;
    }

    public int getBlockMeta(int x, int y, int z) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            return 0;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.getBlockMeta(x &= 0xF, y, z &= 0xF);
    }

    public void setBlockMeta(int x, int y, int z, int meta) {
        if (this.setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta)) {
            int n = this.getBlockId(x, y, z);
            if (Block.BLOCKS_IGNORE_META_UPDATE[n & 0xFF]) {
                this.blockUpdate(x, y, z, n);
            } else {
                this.notifyNeighbors(x, y, z, n);
            }
        }
    }

    public boolean setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return false;
        }
        if (y < 0) {
            return false;
        }
        if (y >= 128) {
            return false;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        chunk.setBlockMeta(x &= 0xF, y, z &= 0xF, meta);
        return true;
    }

    public boolean setBlock(int x, int y, int z, int blockId) {
        if (this.setBlockWithoutNotifyingNeighbors(x, y, z, blockId)) {
            this.blockUpdate(x, y, z, blockId);
            return true;
        }
        return false;
    }

    public boolean setBlock(int x, int y, int z, int blockId, int meta) {
        if (this.setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta)) {
            this.blockUpdate(x, y, z, blockId);
            return true;
        }
        return false;
    }

    public void blockUpdateEvent(int x, int y, int z) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).blockUpdate(x, y, z);
        }
    }

    protected void blockUpdate(int x, int y, int z, int blockId) {
        this.blockUpdateEvent(x, y, z);
        this.notifyNeighbors(x, y, z, blockId);
    }

    public void setBlocksDirty(int x, int z, int minY, int maxY) {
        if (minY > maxY) {
            int n = maxY;
            maxY = minY;
            minY = n;
        }
        this.setBlocksDirty(x, minY, z, x, maxY, z);
    }

    public void setBlockDirty(int x, int y, int z) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).setBlocksDirty(x, y, z, x, y, z);
        }
    }

    public void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).setBlocksDirty(minX, minY, minZ, maxX, maxY, maxZ);
        }
    }

    public void notifyNeighbors(int x, int y, int z, int blockId) {
        this.neighborUpdate(x - 1, y, z, blockId);
        this.neighborUpdate(x + 1, y, z, blockId);
        this.neighborUpdate(x, y - 1, z, blockId);
        this.neighborUpdate(x, y + 1, z, blockId);
        this.neighborUpdate(x, y, z - 1, blockId);
        this.neighborUpdate(x, y, z + 1, blockId);
    }

    private void neighborUpdate(int x, int y, int z, int blockId) {
        if (this.pauseTicking || this.isRemote) {
            return;
        }
        Block block = Block.BLOCKS[this.getBlockId(x, y, z)];
        if (block != null) {
            block.neighborUpdate(this, x, y, z, blockId);
        }
    }

    public boolean hasSkyLight(int x, int y, int z) {
        return this.getChunk(x >> 4, z >> 4).isAboveMaxHeight(x & 0xF, y, z & 0xF);
    }

    public int getBrightness(int x, int y, int z) {
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            y = 127;
        }
        return this.getChunk(x >> 4, z >> 4).getLight(x & 0xF, y, z & 0xF, 0);
    }

    public int getLightLevel(int i, int j, int k) {
        return this.getLightLevel(i, j, k, true);
    }

    public int getLightLevel(int x, int y, int z, boolean bl) {
        int n;
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 15;
        }
        if (bl && ((n = this.getBlockId(x, y, z)) == Block.SLAB.id || n == Block.FARMLAND.id || n == Block.COBBLESTONE_STAIRS.id || n == Block.WOODEN_STAIRS.id)) {
            int n2 = this.getLightLevel(x, y + 1, z, false);
            int n3 = this.getLightLevel(x + 1, y, z, false);
            int n4 = this.getLightLevel(x - 1, y, z, false);
            int n5 = this.getLightLevel(x, y, z + 1, false);
            int n6 = this.getLightLevel(x, y, z - 1, false);
            if (n3 > n2) {
                n2 = n3;
            }
            if (n4 > n2) {
                n2 = n4;
            }
            if (n5 > n2) {
                n2 = n5;
            }
            if (n6 > n2) {
                n2 = n6;
            }
            return n2;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            y = 127;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.getLight(x &= 0xF, y, z &= 0xF, this.ambientDarkness);
    }

    public boolean isTopY(int x, int y, int z) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return false;
        }
        if (y < 0) {
            return false;
        }
        if (y >= 128) {
            return true;
        }
        if (!this.hasChunk(x >> 4, z >> 4)) {
            return false;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.isAboveMaxHeight(x &= 0xF, y, z &= 0xF);
    }

    public int getTopY(int x, int z) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 0;
        }
        if (!this.hasChunk(x >> 4, z >> 4)) {
            return 0;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        return chunk.getHeight(x & 0xF, z & 0xF);
    }

    public void updateLight(LightType lightType, int x, int y, int z, int l) {
        int n;
        if (this.dimension.hasCeiling && lightType == LightType.SKY) {
            return;
        }
        if (!this.isPosLoaded(x, y, z)) {
            return;
        }
        if (lightType == LightType.SKY) {
            if (this.isTopY(x, y, z)) {
                l = 15;
            }
        } else if (lightType == LightType.BLOCK && Block.BLOCKS_LIGHT_LUMINANCE[n = this.getBlockId(x, y, z)] > l) {
            l = Block.BLOCKS_LIGHT_LUMINANCE[n];
        }
        if (this.getBrightness(lightType, x, y, z) != l) {
            this.queueLightUpdate(lightType, x, y, z, x, y, z);
        }
    }

    public int getBrightness(LightType type, int x, int y, int z) {
        if (y < 0) {
            y = 0;
        }
        if (y >= 128) {
            y = 127;
        }
        if (y < 0 || y >= 128 || x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return type.lightValue;
        }
        int n = x >> 4;
        int n2 = z >> 4;
        if (!this.hasChunk(n, n2)) {
            return 0;
        }
        Chunk chunk = this.getChunk(n, n2);
        return chunk.getLight(type, x & 0xF, y, z & 0xF);
    }

    public void setLight(LightType lightType, int x, int y, int z, int value) {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return;
        }
        if (y < 0) {
            return;
        }
        if (y >= 128) {
            return;
        }
        if (!this.hasChunk(x >> 4, z >> 4)) {
            return;
        }
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        chunk.setLight(lightType, x & 0xF, y, z & 0xF, value);
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).blockUpdate(x, y, z);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public float getNaturalBrightness(int x, int y, int z, int blockLight) {
        int n = this.getLightLevel(x, y, z);
        if (n < blockLight) {
            n = blockLight;
        }
        return this.dimension.lightLevelToLuminance[n];
    }

    public float getLightBrightness(int i, int j, int k) {
        return this.dimension.lightLevelToLuminance[this.getLightLevel(i, j, k)];
    }

    public boolean canMonsterSpawn() {
        return this.ambientDarkness < 4;
    }

    public HitResult raycast(Vec3d start, Vec3d end) {
        return this.raycast(start, end, false, false);
    }

    public HitResult raycast(Vec3d start, Vec3d end, boolean bl) {
        return this.raycast(start, end, bl, false);
    }

    public HitResult raycast(Vec3d start, Vec3d end, boolean bl, boolean bl2) {
        HitResult hitResult;
        if (Double.isNaN(start.x) || Double.isNaN(start.y) || Double.isNaN(start.z)) {
            return null;
        }
        if (Double.isNaN(end.x) || Double.isNaN(end.y) || Double.isNaN(end.z)) {
            return null;
        }
        int n = MathHelper.floor(end.x);
        int n2 = MathHelper.floor(end.y);
        int n3 = MathHelper.floor(end.z);
        int n4 = MathHelper.floor(start.x);
        int n5 = MathHelper.floor(start.y);
        int n6 = MathHelper.floor(start.z);
        int n7 = this.getBlockId(n4, n5, n6);
        int n8 = this.getBlockMeta(n4, n5, n6);
        Block block = Block.BLOCKS[n7];
        if ((!bl2 || block == null || block.getCollisionShape(this, n4, n5, n6) != null) && n7 > 0 && block.hasCollision(n8, bl) && (hitResult = block.raycast(this, n4, n5, n6, start, end)) != null) {
            return hitResult;
        }
        n7 = 200;
        while (n7-- >= 0) {
            HitResult hitResult2;
            if (Double.isNaN(start.x) || Double.isNaN(start.y) || Double.isNaN(start.z)) {
                return null;
            }
            if (n4 == n && n5 == n2 && n6 == n3) {
                return null;
            }
            n8 = 1;
            boolean bl3 = true;
            boolean bl4 = true;
            double d = 999.0;
            double d2 = 999.0;
            double d3 = 999.0;
            if (n > n4) {
                d = (double)n4 + 1.0;
            } else if (n < n4) {
                d = (double)n4 + 0.0;
            } else {
                n8 = 0;
            }
            if (n2 > n5) {
                d2 = (double)n5 + 1.0;
            } else if (n2 < n5) {
                d2 = (double)n5 + 0.0;
            } else {
                bl3 = false;
            }
            if (n3 > n6) {
                d3 = (double)n6 + 1.0;
            } else if (n3 < n6) {
                d3 = (double)n6 + 0.0;
            } else {
                bl4 = false;
            }
            double d4 = 999.0;
            double d5 = 999.0;
            double d6 = 999.0;
            double d7 = end.x - start.x;
            double d8 = end.y - start.y;
            double d9 = end.z - start.z;
            if (n8 != 0) {
                d4 = (d - start.x) / d7;
            }
            if (bl3) {
                d5 = (d2 - start.y) / d8;
            }
            if (bl4) {
                d6 = (d3 - start.z) / d9;
            }
            int n9 = 0;
            if (d4 < d5 && d4 < d6) {
                n9 = n > n4 ? 4 : 5;
                start.x = d;
                start.y += d8 * d4;
                start.z += d9 * d4;
            } else if (d5 < d6) {
                n9 = n2 > n5 ? 0 : 1;
                start.x += d7 * d5;
                start.y = d2;
                start.z += d9 * d5;
            } else {
                n9 = n3 > n6 ? 2 : 3;
                start.x += d7 * d6;
                start.y += d8 * d6;
                start.z = d3;
            }
            Vec3d vec3d = Vec3d.createCached(start.x, start.y, start.z);
            vec3d.x = MathHelper.floor(start.x);
            n4 = (int)vec3d.x;
            if (n9 == 5) {
                --n4;
                vec3d.x += 1.0;
            }
            vec3d.y = MathHelper.floor(start.y);
            n5 = (int)vec3d.y;
            if (n9 == 1) {
                --n5;
                vec3d.y += 1.0;
            }
            vec3d.z = MathHelper.floor(start.z);
            n6 = (int)vec3d.z;
            if (n9 == 3) {
                --n6;
                vec3d.z += 1.0;
            }
            int n10 = this.getBlockId(n4, n5, n6);
            int n11 = this.getBlockMeta(n4, n5, n6);
            Block block2 = Block.BLOCKS[n10];
            if (bl2 && block2 != null && block2.getCollisionShape(this, n4, n5, n6) == null || n10 <= 0 || !block2.hasCollision(n11, bl) || (hitResult2 = block2.raycast(this, n4, n5, n6, start, end)) == null) continue;
            return hitResult2;
        }
        return null;
    }

    public void playSound(Entity source, String sound, float f, float g) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).playSound(sound, source.x, source.y - (double)source.standingEyeHeight, source.z, f, g);
        }
    }

    public void playSound(double x, double y, double z, String sound, float g, float h) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).playSound(sound, x, y, z, g, h);
        }
    }

    public void playStreaming(String music, int x, int y, int z) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).playStreaming(music, x, y, z);
        }
    }

    public void addParticle(String particle, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).addParticle(particle, x, y, z, velocityX, velocityY, velocityZ);
        }
    }

    public boolean spawnGlobalEntity(Entity entity) {
        this.globalEntities.add(entity);
        return true;
    }

    public boolean spawnEntity(Entity entity) {
        int n = MathHelper.floor(entity.x / 16.0);
        int n2 = MathHelper.floor(entity.z / 16.0);
        boolean bl = false;
        if (entity instanceof PlayerEntity) {
            bl = true;
        }
        if (bl || this.hasChunk(n, n2)) {
            if (entity instanceof PlayerEntity) {
                PlayerEntity playerEntity = (PlayerEntity)entity;
                this.players.add(playerEntity);
                this.updateSleepingPlayers();
            }
            this.getChunk(n, n2).addEntity(entity);
            this.entities.add(entity);
            this.notifyEntityAdded(entity);
            return true;
        }
        return false;
    }

    protected void notifyEntityAdded(Entity entity) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).notifyEntityAdded(entity);
        }
    }

    protected void notifyEntityRemoved(Entity entity) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).notifyEntityRemoved(entity);
        }
    }

    public void remove(Entity entity) {
        if (entity.passenger != null) {
            entity.passenger.setVehicle(null);
        }
        if (entity.vehicle != null) {
            entity.setVehicle(null);
        }
        entity.markDead();
        if (entity instanceof PlayerEntity) {
            this.players.remove((PlayerEntity)entity);
            this.updateSleepingPlayers();
        }
    }

    @Environment(value=EnvType.SERVER)
    public void serverRemove(Entity entity) {
        entity.markDead();
        if (entity instanceof PlayerEntity) {
            this.players.remove((PlayerEntity)entity);
            this.updateSleepingPlayers();
        }
        int n = entity.chunkX;
        int n2 = entity.chunkZ;
        if (entity.isPersistent && this.hasChunk(n, n2)) {
            this.getChunk(n, n2).removeEntity(entity);
        }
        this.entities.remove(entity);
        this.notifyEntityRemoved(entity);
    }

    public void addEventListener(GameEventListener eventListener) {
        this.eventListeners.add(eventListener);
    }

    @Environment(value=EnvType.CLIENT)
    public void removeEventListener(GameEventListener eventListener) {
        this.eventListeners.remove(eventListener);
    }

    public List getEntityCollisions(Entity entity, Box box) {
        this.tempCollisionBoxes.clear();
        int n = MathHelper.floor(box.minX);
        int n2 = MathHelper.floor(box.maxX + 1.0);
        int n3 = MathHelper.floor(box.minY);
        int n4 = MathHelper.floor(box.maxY + 1.0);
        int n5 = MathHelper.floor(box.minZ);
        int n6 = MathHelper.floor(box.maxZ + 1.0);
        for (int i = n; i < n2; ++i) {
            for (int j = n5; j < n6; ++j) {
                if (!this.isPosLoaded(i, 64, j)) continue;
                for (int k = n3 - 1; k < n4; ++k) {
                    Block block = Block.BLOCKS[this.getBlockId(i, k, j)];
                    if (block == null) continue;
                    block.addIntersectingBoundingBox(this, i, k, j, box, this.tempCollisionBoxes);
                }
            }
        }
        double d = 0.25;
        List list = this.getEntities(entity, box.expand(d, d, d));
        for (int i = 0; i < list.size(); ++i) {
            Box box2 = ((Entity)list.get(i)).getBoundingBox();
            if (box2 != null && box2.intersects(box)) {
                this.tempCollisionBoxes.add(box2);
            }
            if ((box2 = entity.getCollisionAgainstShape((Entity)list.get(i))) == null || !box2.intersects(box)) continue;
            this.tempCollisionBoxes.add(box2);
        }
        return this.tempCollisionBoxes;
    }

    public int getAmbientDarkness(float partialTicks) {
        float f = this.getTime(partialTicks);
        float f2 = 1.0f - (MathHelper.cos(f * (float)Math.PI * 2.0f) * 2.0f + 0.5f);
        if (f2 < 0.0f) {
            f2 = 0.0f;
        }
        if (f2 > 1.0f) {
            f2 = 1.0f;
        }
        f2 = 1.0f - f2;
        f2 = (float)((double)f2 * (1.0 - (double)(this.getRainGradient(partialTicks) * 5.0f) / 16.0));
        f2 = (float)((double)f2 * (1.0 - (double)(this.getThunderGradient(partialTicks) * 5.0f) / 16.0));
        f2 = 1.0f - f2;
        return (int)(f2 * 11.0f);
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getSkyColor(Entity entity, float partialTicks) {
        float f;
        float f2;
        float f3 = this.getTime(partialTicks);
        float f4 = MathHelper.cos(f3 * (float)Math.PI * 2.0f) * 2.0f + 0.5f;
        if (f4 < 0.0f) {
            f4 = 0.0f;
        }
        if (f4 > 1.0f) {
            f4 = 1.0f;
        }
        int n = MathHelper.floor(entity.x);
        int n2 = MathHelper.floor(entity.z);
        float f5 = (float)this.getBiomeSource().getTemperature(n, n2);
        int n3 = this.getBiomeSource().getBiome(n, n2).getSkyColor(f5);
        float f6 = (float)(n3 >> 16 & 0xFF) / 255.0f;
        float f7 = (float)(n3 >> 8 & 0xFF) / 255.0f;
        float f8 = (float)(n3 & 0xFF) / 255.0f;
        f6 *= f4;
        f7 *= f4;
        f8 *= f4;
        float f9 = this.getRainGradient(partialTicks);
        if (f9 > 0.0f) {
            f2 = (f6 * 0.3f + f7 * 0.59f + f8 * 0.11f) * 0.6f;
            f = 1.0f - f9 * 0.75f;
            f6 = f6 * f + f2 * (1.0f - f);
            f7 = f7 * f + f2 * (1.0f - f);
            f8 = f8 * f + f2 * (1.0f - f);
        }
        if ((f2 = this.getThunderGradient(partialTicks)) > 0.0f) {
            f = (f6 * 0.3f + f7 * 0.59f + f8 * 0.11f) * 0.2f;
            float f10 = 1.0f - f2 * 0.75f;
            f6 = f6 * f10 + f * (1.0f - f10);
            f7 = f7 * f10 + f * (1.0f - f10);
            f8 = f8 * f10 + f * (1.0f - f10);
        }
        if (this.lightningTicksLeft > 0) {
            f = (float)this.lightningTicksLeft - partialTicks;
            if (f > 1.0f) {
                f = 1.0f;
            }
            f6 = f6 * (1.0f - (f *= 0.45f)) + 0.8f * f;
            f7 = f7 * (1.0f - f) + 0.8f * f;
            f8 = f8 * (1.0f - f) + 1.0f * f;
        }
        return Vec3d.createCached(f6, f7, f8);
    }

    public float getTime(float partialTicks) {
        return this.dimension.getTimeOfDay(this.properties.getTime(), partialTicks);
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getCloudColor(float partialTicks) {
        float f;
        float f2;
        float f3 = this.getTime(partialTicks);
        float f4 = MathHelper.cos(f3 * (float)Math.PI * 2.0f) * 2.0f + 0.5f;
        if (f4 < 0.0f) {
            f4 = 0.0f;
        }
        if (f4 > 1.0f) {
            f4 = 1.0f;
        }
        float f5 = (float)(this.worldTimeMask >> 16 & 0xFFL) / 255.0f;
        float f6 = (float)(this.worldTimeMask >> 8 & 0xFFL) / 255.0f;
        float f7 = (float)(this.worldTimeMask & 0xFFL) / 255.0f;
        float f8 = this.getRainGradient(partialTicks);
        if (f8 > 0.0f) {
            f2 = (f5 * 0.3f + f6 * 0.59f + f7 * 0.11f) * 0.6f;
            f = 1.0f - f8 * 0.95f;
            f5 = f5 * f + f2 * (1.0f - f);
            f6 = f6 * f + f2 * (1.0f - f);
            f7 = f7 * f + f2 * (1.0f - f);
        }
        f5 *= f4 * 0.9f + 0.1f;
        f6 *= f4 * 0.9f + 0.1f;
        f7 *= f4 * 0.85f + 0.15f;
        f2 = this.getThunderGradient(partialTicks);
        if (f2 > 0.0f) {
            f = (f5 * 0.3f + f6 * 0.59f + f7 * 0.11f) * 0.2f;
            float f9 = 1.0f - f2 * 0.95f;
            f5 = f5 * f9 + f * (1.0f - f9);
            f6 = f6 * f9 + f * (1.0f - f9);
            f7 = f7 * f9 + f * (1.0f - f9);
        }
        return Vec3d.createCached(f5, f6, f7);
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getFogColor(float partialTicks) {
        float f = this.getTime(partialTicks);
        return this.dimension.getFogColor(f, partialTicks);
    }

    public int getTopSolidBlockY(int x, int z) {
        Chunk chunk = this.getChunkFromPos(x, z);
        x &= 0xF;
        z &= 0xF;
        for (int i = 127; i > 0; --i) {
            Material material;
            int n = chunk.getBlockId(x, i, z);
            Material material2 = material = n == 0 ? Material.AIR : Block.BLOCKS[n].material;
            if (!material.blocksMovement() && !material.isFluid()) {
                continue;
            }
            return i + 1;
        }
        return -1;
    }

    @Environment(value=EnvType.CLIENT)
    public float calculateSkyLightIntensity(float partialTicks) {
        float f = this.getTime(partialTicks);
        float f2 = 1.0f - (MathHelper.cos(f * (float)Math.PI * 2.0f) * 2.0f + 0.75f);
        if (f2 < 0.0f) {
            f2 = 0.0f;
        }
        if (f2 > 1.0f) {
            f2 = 1.0f;
        }
        return f2 * f2 * 0.5f;
    }

    @Environment(value=EnvType.SERVER)
    public int getSpawnPositionValidityY(int x, int z) {
        Chunk chunk = this.getChunkFromPos(x, z);
        x &= 0xF;
        z &= 0xF;
        for (int i = 127; i > 0; --i) {
            int n = chunk.getBlockId(x, i, z);
            if (n == 0 || !Block.BLOCKS[n].material.blocksMovement()) {
                continue;
            }
            return i + 1;
        }
        return -1;
    }

    public void scheduleBlockUpdate(int x, int y, int z, int id, int tickRate) {
        BlockEvent blockEvent = new BlockEvent(x, y, z, id);
        int n = 8;
        if (this.instantBlockUpdateEnabled) {
            int n2;
            if (this.isRegionLoaded(blockEvent.x - n, blockEvent.y - n, blockEvent.z - n, blockEvent.x + n, blockEvent.y + n, blockEvent.z + n) && (n2 = this.getBlockId(blockEvent.x, blockEvent.y, blockEvent.z)) == blockEvent.blockId && n2 > 0) {
                Block.BLOCKS[n2].onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, this.random);
            }
            return;
        }
        if (this.isRegionLoaded(x - n, y - n, z - n, x + n, y + n, z + n)) {
            if (id > 0) {
                blockEvent.get((long)tickRate + this.properties.getTime());
            }
            if (!this.scheduledUpdateSet.contains(blockEvent)) {
                this.scheduledUpdateSet.add(blockEvent);
                this.scheduledUpdates.add(blockEvent);
            }
        }
    }

    public void tickEntities() {
        int n;
        int n2;
        Object object;
        int n3;
        for (n3 = 0; n3 < this.globalEntities.size(); ++n3) {
            object = (Entity)this.globalEntities.get(n3);
            ((Entity)object).tick();
            if (!((Entity)object).dead) continue;
            this.globalEntities.remove(n3--);
        }
        this.entities.removeAll(this.entitiesToUnload);
        for (n3 = 0; n3 < this.entitiesToUnload.size(); ++n3) {
            object = (Entity)this.entitiesToUnload.get(n3);
            n2 = ((Entity)object).chunkX;
            n = ((Entity)object).chunkZ;
            if (!((Entity)object).isPersistent || !this.hasChunk(n2, n)) continue;
            this.getChunk(n2, n).removeEntity((Entity)object);
        }
        for (n3 = 0; n3 < this.entitiesToUnload.size(); ++n3) {
            this.notifyEntityRemoved((Entity)this.entitiesToUnload.get(n3));
        }
        this.entitiesToUnload.clear();
        for (n3 = 0; n3 < this.entities.size(); ++n3) {
            object = (Entity)this.entities.get(n3);
            if (((Entity)object).vehicle != null) {
                if (!((Entity)object).vehicle.dead && ((Entity)object).vehicle.passenger == object) continue;
                ((Entity)object).vehicle.passenger = null;
                ((Entity)object).vehicle = null;
            }
            if (!((Entity)object).dead) {
                this.updateEntity((Entity)object);
            }
            if (!((Entity)object).dead) continue;
            n2 = ((Entity)object).chunkX;
            n = ((Entity)object).chunkZ;
            if (((Entity)object).isPersistent && this.hasChunk(n2, n)) {
                this.getChunk(n2, n).removeEntity((Entity)object);
            }
            this.entities.remove(n3--);
            this.notifyEntityRemoved((Entity)object);
        }
        this.processingDeferred = true;
        Iterator iterator = this.blockEntities.iterator();
        while (iterator.hasNext()) {
            object = (BlockEntity)iterator.next();
            if (!((BlockEntity)object).isRemoved()) {
                ((BlockEntity)object).tick();
            }
            if (!((BlockEntity)object).isRemoved()) continue;
            iterator.remove();
            Chunk chunk = this.getChunk(((BlockEntity)object).x >> 4, ((BlockEntity)object).z >> 4);
            if (chunk == null) continue;
            chunk.removeBlockEntityAt(((BlockEntity)object).x & 0xF, ((BlockEntity)object).y, ((BlockEntity)object).z & 0xF);
        }
        this.processingDeferred = false;
        if (!this.blockEntityUpdateQueue.isEmpty()) {
            for (BlockEntity blockEntity : this.blockEntityUpdateQueue) {
                Chunk chunk;
                if (blockEntity.isRemoved()) continue;
                if (!this.blockEntities.contains(blockEntity)) {
                    this.blockEntities.add(blockEntity);
                }
                if ((chunk = this.getChunk(blockEntity.x >> 4, blockEntity.z >> 4)) != null) {
                    chunk.setBlockEntity(blockEntity.x & 0xF, blockEntity.y, blockEntity.z & 0xF, blockEntity);
                }
                this.blockUpdateEvent(blockEntity.x, blockEntity.y, blockEntity.z);
            }
            this.blockEntityUpdateQueue.clear();
        }
    }

    public void processBlockUpdates(Collection blockUpdates) {
        if (this.processingDeferred) {
            this.blockEntityUpdateQueue.addAll(blockUpdates);
        } else {
            this.blockEntities.addAll(blockUpdates);
        }
    }

    public void updateEntity(Entity entity) {
        this.updateEntity(entity, true);
    }

    public void updateEntity(Entity entity, boolean requireLoaded) {
        int n = MathHelper.floor(entity.x);
        int n2 = MathHelper.floor(entity.z);
        int n3 = 32;
        if (requireLoaded && !this.isRegionLoaded(n - n3, 0, n2 - n3, n + n3, 128, n2 + n3)) {
            return;
        }
        entity.lastTickX = entity.x;
        entity.lastTickY = entity.y;
        entity.lastTickZ = entity.z;
        entity.prevYaw = entity.yaw;
        entity.prevPitch = entity.pitch;
        if (requireLoaded && entity.isPersistent) {
            if (entity.vehicle != null) {
                entity.tickRiding();
            } else {
                entity.tick();
            }
        }
        if (Double.isNaN(entity.x) || Double.isInfinite(entity.x)) {
            entity.x = entity.lastTickX;
        }
        if (Double.isNaN(entity.y) || Double.isInfinite(entity.y)) {
            entity.y = entity.lastTickY;
        }
        if (Double.isNaN(entity.z) || Double.isInfinite(entity.z)) {
            entity.z = entity.lastTickZ;
        }
        if (Double.isNaN(entity.pitch) || Double.isInfinite(entity.pitch)) {
            entity.pitch = entity.prevPitch;
        }
        if (Double.isNaN(entity.yaw) || Double.isInfinite(entity.yaw)) {
            entity.yaw = entity.prevYaw;
        }
        int n4 = MathHelper.floor(entity.x / 16.0);
        int n5 = MathHelper.floor(entity.y / 16.0);
        int n6 = MathHelper.floor(entity.z / 16.0);
        if (!entity.isPersistent || entity.chunkX != n4 || entity.chunkSlice != n5 || entity.chunkZ != n6) {
            if (entity.isPersistent && this.hasChunk(entity.chunkX, entity.chunkZ)) {
                this.getChunk(entity.chunkX, entity.chunkZ).removeEntity(entity, entity.chunkSlice);
            }
            if (this.hasChunk(n4, n6)) {
                entity.isPersistent = true;
                this.getChunk(n4, n6).addEntity(entity);
            } else {
                entity.isPersistent = false;
            }
        }
        if (requireLoaded && entity.isPersistent && entity.passenger != null) {
            if (entity.passenger.dead || entity.passenger.vehicle != entity) {
                entity.passenger.vehicle = null;
                entity.passenger = null;
            } else {
                this.updateEntity(entity.passenger);
            }
        }
    }

    public boolean canSpawnEntity(Box box) {
        List list = this.getEntities(null, box);
        for (int i = 0; i < list.size(); ++i) {
            Entity entity = (Entity)list.get(i);
            if (entity.dead || !entity.blocksSameBlockSpawning) continue;
            return false;
        }
        return true;
    }

    @Environment(value=EnvType.SERVER)
    public boolean isAnyBlockInBox(Box box) {
        int n = MathHelper.floor(box.minX);
        int n2 = MathHelper.floor(box.maxX + 1.0);
        int n3 = MathHelper.floor(box.minY);
        int n4 = MathHelper.floor(box.maxY + 1.0);
        int n5 = MathHelper.floor(box.minZ);
        int n6 = MathHelper.floor(box.maxZ + 1.0);
        if (box.minX < 0.0) {
            --n;
        }
        if (box.minY < 0.0) {
            --n3;
        }
        if (box.minZ < 0.0) {
            --n5;
        }
        for (int i = n; i < n2; ++i) {
            for (int j = n3; j < n4; ++j) {
                for (int k = n5; k < n6; ++k) {
                    Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
                    if (block == null) continue;
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isBoxSubmergedInFluid(Box box) {
        int n = MathHelper.floor(box.minX);
        int n2 = MathHelper.floor(box.maxX + 1.0);
        int n3 = MathHelper.floor(box.minY);
        int n4 = MathHelper.floor(box.maxY + 1.0);
        int n5 = MathHelper.floor(box.minZ);
        int n6 = MathHelper.floor(box.maxZ + 1.0);
        if (box.minX < 0.0) {
            --n;
        }
        if (box.minY < 0.0) {
            --n3;
        }
        if (box.minZ < 0.0) {
            --n5;
        }
        for (int i = n; i < n2; ++i) {
            for (int j = n3; j < n4; ++j) {
                for (int k = n5; k < n6; ++k) {
                    Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
                    if (block == null || !block.material.isFluid()) continue;
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isFireOrLavaInBox(Box box) {
        int n;
        int n2 = MathHelper.floor(box.minX);
        int n3 = MathHelper.floor(box.maxX + 1.0);
        int n4 = MathHelper.floor(box.minY);
        int n5 = MathHelper.floor(box.maxY + 1.0);
        int n6 = MathHelper.floor(box.minZ);
        if (this.isRegionLoaded(n2, n4, n6, n3, n5, n = MathHelper.floor(box.maxZ + 1.0))) {
            for (int i = n2; i < n3; ++i) {
                for (int j = n4; j < n5; ++j) {
                    for (int k = n6; k < n; ++k) {
                        int n7 = this.getBlockId(i, j, k);
                        if (n7 != Block.FIRE.id && n7 != Block.FLOWING_LAVA.id && n7 != Block.LAVA.id) continue;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    public boolean updateMovementInFluid(Box entityBoundingBox, Material fluidMaterial, Entity entity) {
        int n;
        int n2 = MathHelper.floor(entityBoundingBox.minX);
        int n3 = MathHelper.floor(entityBoundingBox.maxX + 1.0);
        int n4 = MathHelper.floor(entityBoundingBox.minY);
        int n5 = MathHelper.floor(entityBoundingBox.maxY + 1.0);
        int n6 = MathHelper.floor(entityBoundingBox.minZ);
        if (!this.isRegionLoaded(n2, n4, n6, n3, n5, n = MathHelper.floor(entityBoundingBox.maxZ + 1.0))) {
            return false;
        }
        boolean bl = false;
        Vec3d vec3d = Vec3d.createCached(0.0, 0.0, 0.0);
        for (int i = n2; i < n3; ++i) {
            for (int j = n4; j < n5; ++j) {
                for (int k = n6; k < n; ++k) {
                    double d;
                    Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
                    if (block == null || block.material != fluidMaterial || !((double)n5 >= (d = (double)((float)(j + 1) - LiquidBlock.getFluidHeightFromMeta(this.getBlockMeta(i, j, k)))))) continue;
                    bl = true;
                    block.applyVelocity(this, i, j, k, entity, vec3d);
                }
            }
        }
        if (vec3d.length() > 0.0) {
            vec3d = vec3d.normalize();
            double d = 0.014;
            entity.velocityX += vec3d.x * d;
            entity.velocityY += vec3d.y * d;
            entity.velocityZ += vec3d.z * d;
        }
        return bl;
    }

    public boolean isMaterialInBox(Box boundingBox, Material material) {
        int n = MathHelper.floor(boundingBox.minX);
        int n2 = MathHelper.floor(boundingBox.maxX + 1.0);
        int n3 = MathHelper.floor(boundingBox.minY);
        int n4 = MathHelper.floor(boundingBox.maxY + 1.0);
        int n5 = MathHelper.floor(boundingBox.minZ);
        int n6 = MathHelper.floor(boundingBox.maxZ + 1.0);
        for (int i = n; i < n2; ++i) {
            for (int j = n3; j < n4; ++j) {
                for (int k = n5; k < n6; ++k) {
                    Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
                    if (block == null || block.material != material) continue;
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isFluidInBox(Box boundingBox, Material fluid) {
        int n = MathHelper.floor(boundingBox.minX);
        int n2 = MathHelper.floor(boundingBox.maxX + 1.0);
        int n3 = MathHelper.floor(boundingBox.minY);
        int n4 = MathHelper.floor(boundingBox.maxY + 1.0);
        int n5 = MathHelper.floor(boundingBox.minZ);
        int n6 = MathHelper.floor(boundingBox.maxZ + 1.0);
        for (int i = n; i < n2; ++i) {
            for (int j = n3; j < n4; ++j) {
                for (int k = n5; k < n6; ++k) {
                    Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
                    if (block == null || block.material != fluid) continue;
                    int n7 = this.getBlockMeta(i, j, k);
                    double d = j + 1;
                    if (n7 < 8) {
                        d = (double)(j + 1) - (double)n7 / 8.0;
                    }
                    if (!(d >= boundingBox.minY)) continue;
                    return true;
                }
            }
        }
        return false;
    }

    public Explosion createExplosion(Entity source, double x, double y, double z, float power) {
        return this.createExplosion(source, x, y, z, power, false);
    }

    public Explosion createExplosion(Entity source, double x, double y, double z, float power, boolean fire) {
        Explosion explosion = new Explosion(this, source, x, y, z, power);
        explosion.fire = fire;
        explosion.explode();
        explosion.playExplosionSound(true);
        return explosion;
    }

    public float getVisibilityRatio(Vec3d vec, Box box) {
        double d = 1.0 / ((box.maxX - box.minX) * 2.0 + 1.0);
        double d2 = 1.0 / ((box.maxY - box.minY) * 2.0 + 1.0);
        double d3 = 1.0 / ((box.maxZ - box.minZ) * 2.0 + 1.0);
        int n = 0;
        int n2 = 0;
        float f = 0.0f;
        while (f <= 1.0f) {
            float f2 = 0.0f;
            while (f2 <= 1.0f) {
                float f3 = 0.0f;
                while (f3 <= 1.0f) {
                    double d4 = box.minX + (box.maxX - box.minX) * (double)f;
                    double d5 = box.minY + (box.maxY - box.minY) * (double)f2;
                    double d6 = box.minZ + (box.maxZ - box.minZ) * (double)f3;
                    if (this.raycast(Vec3d.createCached(d4, d5, d6), vec) == null) {
                        ++n;
                    }
                    ++n2;
                    f3 = (float)((double)f3 + d3);
                }
                f2 = (float)((double)f2 + d2);
            }
            f = (float)((double)f + d);
        }
        return (float)n / (float)n2;
    }

    public void extinguishFire(PlayerEntity player, int x, int y, int z, int direction) {
        if (direction == 0) {
            --y;
        }
        if (direction == 1) {
            ++y;
        }
        if (direction == 2) {
            --z;
        }
        if (direction == 3) {
            ++z;
        }
        if (direction == 4) {
            --x;
        }
        if (direction == 5) {
            ++x;
        }
        if (this.getBlockId(x, y, z) == Block.FIRE.id) {
            this.worldEvent(player, 1004, x, y, z, 0);
            this.setBlock(x, y, z, 0);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public Entity getPlayerForProxy(Class playerClass) {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public String getEntityCount() {
        return "All: " + this.entities.size();
    }

    @Environment(value=EnvType.CLIENT)
    public String getDebugInfo() {
        return this.chunkSource.getDebugInfo();
    }

    public BlockEntity getBlockEntity(int x, int y, int z) {
        Chunk chunk = this.getChunk(x >> 4, z >> 4);
        if (chunk != null) {
            return chunk.getBlockEntity(x & 0xF, y, z & 0xF);
        }
        return null;
    }

    public void setBlockEntity(int x, int y, int z, BlockEntity blockEntity) {
        if (!blockEntity.isRemoved()) {
            if (this.processingDeferred) {
                blockEntity.x = x;
                blockEntity.y = y;
                blockEntity.z = z;
                this.blockEntityUpdateQueue.add(blockEntity);
            } else {
                this.blockEntities.add(blockEntity);
                Chunk chunk = this.getChunk(x >> 4, z >> 4);
                if (chunk != null) {
                    chunk.setBlockEntity(x & 0xF, y, z & 0xF, blockEntity);
                }
            }
        }
    }

    public void removeBlockEntity(int x, int y, int z) {
        BlockEntity blockEntity = this.getBlockEntity(x, y, z);
        if (blockEntity != null && this.processingDeferred) {
            blockEntity.markRemoved();
        } else {
            Chunk chunk;
            if (blockEntity != null) {
                this.blockEntities.remove(blockEntity);
            }
            if ((chunk = this.getChunk(x >> 4, z >> 4)) != null) {
                chunk.removeBlockEntityAt(x & 0xF, y, z & 0xF);
            }
        }
    }

    public boolean isBlockOpaqueCube(int i, int j, int k) {
        Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
        if (block == null) {
            return false;
        }
        return block.isOpaque();
    }

    public boolean shouldSuffocate(int x, int y, int z) {
        Block block = Block.BLOCKS[this.getBlockId(x, y, z)];
        if (block == null) {
            return false;
        }
        return block.material.suffocates() && block.isFullCube();
    }

    @Environment(value=EnvType.CLIENT)
    public void savingProgress(LoadingDisplay display) {
        this.saveWithLoadingDisplay(true, display);
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public boolean doLightingUpdates() {
        if (this.lightingUpdateCount >= 50) {
            return false;
        }
        ++this.lightingUpdateCount;
        try {
            int n = 500;
            while (this.lightingQueue.size() > 0) {
                if (--n <= 0) {
                    boolean bl = true;
                    return bl;
                }
                ((LightUpdate)this.lightingQueue.remove(this.lightingQueue.size() - 1)).updateLight(this);
            }
            boolean bl = false;
            return bl;
        }
        finally {
            --this.lightingUpdateCount;
        }
    }

    public void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        this.queueLightUpdate(type, minX, minY, minZ, maxX, maxY, maxZ, true);
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void queueLightUpdate(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, boolean bl) {
        if (this.dimension.hasCeiling && type == LightType.SKY) {
            return;
        }
        ++lightingQueueCount;
        try {
            int n;
            if (lightingQueueCount == 50) {
                return;
            }
            int n2 = (maxX + minX) / 2;
            int n3 = (maxZ + minZ) / 2;
            if (!this.isPosLoaded(n2, 64, n3)) {
                return;
            }
            if (this.getChunkFromPos(n2, n3).isEmpty()) {
                return;
            }
            int n4 = this.lightingQueue.size();
            if (bl) {
                n = 5;
                if (n > n4) {
                    n = n4;
                }
                for (int i = 0; i < n; ++i) {
                    LightUpdate lightUpdate = (LightUpdate)this.lightingQueue.get(this.lightingQueue.size() - i - 1);
                    if (lightUpdate.lightType != type || !lightUpdate.expand(minX, minY, minZ, maxX, maxY, maxZ)) continue;
                    return;
                }
            }
            this.lightingQueue.add(new LightUpdate(type, minX, minY, minZ, maxX, maxY, maxZ));
            n = 1000000;
            if (this.lightingQueue.size() > 1000000) {
                System.out.println("More than " + n + " updates, aborting lighting updates");
                this.lightingQueue.clear();
            }
        }
        finally {
            --lightingQueueCount;
        }
    }

    public void updateSkyBrightness() {
        int n = this.getAmbientDarkness(1.0f);
        if (n != this.ambientDarkness) {
            this.ambientDarkness = n;
        }
    }

    public void allowSpawning(boolean allowMonsterSpawning, boolean allowMobSpawning) {
        this.allowMonsterSpawning = allowMonsterSpawning;
        this.allowMobSpawning = allowMobSpawning;
    }

    public void tick() {
        long l;
        int n;
        this.updateWeatherCycles();
        if (this.canSkipNight()) {
            n = 0;
            if (this.allowMonsterSpawning && this.difficulty >= 1) {
                n = NaturalSpawner.spawnMonstersAndWakePlayers(this, this.players);
            }
            if (n == 0) {
                l = this.properties.getTime() + 24000L;
                this.properties.setTime(l - l % 24000L);
                this.afterSkipNight();
            }
        }
        NaturalSpawner.tick(this, this.allowMonsterSpawning, this.allowMobSpawning);
        this.chunkSource.tick();
        n = this.getAmbientDarkness(1.0f);
        if (n != this.ambientDarkness) {
            this.ambientDarkness = n;
            for (int i = 0; i < this.eventListeners.size(); ++i) {
                ((GameEventListener)this.eventListeners.get(i)).notifyAmbientDarknessChanged();
            }
        }
        if ((l = this.properties.getTime() + 1L) % (long)this.saveInterval == 0L) {
            this.saveWithLoadingDisplay(false, null);
        }
        this.properties.setTime(l);
        this.processScheduledTicks(false);
        this.manageChunkUpdatesAndEvents();
    }

    private void prepareWeather() {
        if (this.properties.getRaining()) {
            this.rainGradient = 1.0f;
            if (this.properties.getThundering()) {
                this.thunderGradient = 1.0f;
            }
        }
    }

    protected void updateWeatherCycles() {
        int n;
        if (this.dimension.hasCeiling) {
            return;
        }
        if (this.ticksSinceLightning > 0) {
            --this.ticksSinceLightning;
        }
        if ((n = this.properties.getThunderTime()) <= 0) {
            if (this.properties.getThundering()) {
                this.properties.setThunderTime(this.random.nextInt(12000) + 3600);
            } else {
                this.properties.setThunderTime(this.random.nextInt(168000) + 12000);
            }
        } else {
            this.properties.setThunderTime(--n);
            if (n <= 0) {
                this.properties.setThundering(!this.properties.getThundering());
            }
        }
        int n2 = this.properties.getRainTime();
        if (n2 <= 0) {
            if (this.properties.getRaining()) {
                this.properties.setRainTime(this.random.nextInt(12000) + 12000);
            } else {
                this.properties.setRainTime(this.random.nextInt(168000) + 12000);
            }
        } else {
            this.properties.setRainTime(--n2);
            if (n2 <= 0) {
                this.properties.setRaining(!this.properties.getRaining());
            }
        }
        this.rainGradientPrev = this.rainGradient;
        this.rainGradient = this.properties.getRaining() ? (float)((double)this.rainGradient + 0.01) : (float)((double)this.rainGradient - 0.01);
        if (this.rainGradient < 0.0f) {
            this.rainGradient = 0.0f;
        }
        if (this.rainGradient > 1.0f) {
            this.rainGradient = 1.0f;
        }
        this.thunderGradientPrev = this.thunderGradient;
        this.thunderGradient = this.properties.getThundering() ? (float)((double)this.thunderGradient + 0.01) : (float)((double)this.thunderGradient - 0.01);
        if (this.thunderGradient < 0.0f) {
            this.thunderGradient = 0.0f;
        }
        if (this.thunderGradient > 1.0f) {
            this.thunderGradient = 1.0f;
        }
    }

    private void clearWeather() {
        this.properties.setRainTime(0);
        this.properties.setRaining(false);
        this.properties.setThunderTime(0);
        this.properties.setThundering(false);
    }

    protected void manageChunkUpdatesAndEvents() {
        int n;
        int n2;
        int n3;
        int n4;
        this.activeChunks.clear();
        for (int i = 0; i < this.players.size(); ++i) {
            Object object = (PlayerEntity)this.players.get(i);
            n4 = MathHelper.floor(((PlayerEntity)object).x / 16.0);
            n3 = MathHelper.floor(((PlayerEntity)object).z / 16.0);
            int n5 = 9;
            for (n2 = -n5; n2 <= n5; ++n2) {
                for (n = -n5; n <= n5; ++n) {
                    this.activeChunks.add(new ChunkPos(n2 + n4, n + n3));
                }
            }
        }
        if (this.ambientSoundCounter > 0) {
            --this.ambientSoundCounter;
        }
        for (Object object : this.activeChunks) {
            int n6;
            int n7;
            int n8;
            n4 = ((ChunkPos)object).x * 16;
            n3 = ((ChunkPos)object).z * 16;
            Chunk chunk = this.getChunk(((ChunkPos)object).x, ((ChunkPos)object).z);
            if (this.ambientSoundCounter == 0) {
                PlayerEntity playerEntity;
                this.lcgBlockSeed = this.lcgBlockSeed * 3 + 1013904223;
                n2 = this.lcgBlockSeed >> 2;
                n = n2 & 0xF;
                n8 = n2 >> 8 & 0xF;
                n7 = n2 >> 16 & 0x7F;
                n6 = chunk.getBlockId(n, n7, n8);
                if (n6 == 0 && this.getBrightness(n += n4, n7, n8 += n3) <= this.random.nextInt(8) && this.getBrightness(LightType.SKY, n, n7, n8) <= 0 && (playerEntity = this.getClosestPlayer((double)n + 0.5, (double)n7 + 0.5, (double)n8 + 0.5, 8.0)) != null && playerEntity.getSquaredDistance((double)n + 0.5, (double)n7 + 0.5, (double)n8 + 0.5) > 4.0) {
                    this.playSound((double)n + 0.5, (double)n7 + 0.5, (double)n8 + 0.5, "ambient.cave.cave", 0.7f, 0.8f + this.random.nextFloat() * 0.2f);
                    this.ambientSoundCounter = this.random.nextInt(12000) + 6000;
                }
            }
            if (this.random.nextInt(100000) == 0 && this.isRaining() && this.isThundering()) {
                this.lcgBlockSeed = this.lcgBlockSeed * 3 + 1013904223;
                n2 = this.lcgBlockSeed >> 2;
                n = n4 + (n2 & 0xF);
                n8 = n3 + (n2 >> 8 & 0xF);
                n7 = this.getTopSolidBlockY(n, n8);
                if (this.isRaining(n, n7, n8)) {
                    this.spawnGlobalEntity(new LightningEntity(this, n, n7, n8));
                    this.ticksSinceLightning = 2;
                }
            }
            if (this.random.nextInt(16) == 0) {
                this.lcgBlockSeed = this.lcgBlockSeed * 3 + 1013904223;
                n2 = this.lcgBlockSeed >> 2;
                n = n2 & 0xF;
                n8 = n2 >> 8 & 0xF;
                n7 = this.getTopSolidBlockY(n + n4, n8 + n3);
                if (this.getBiomeSource().getBiome(n + n4, n8 + n3).canSnow() && n7 >= 0 && n7 < 128 && chunk.getLight(LightType.BLOCK, n, n7, n8) < 10) {
                    n6 = chunk.getBlockId(n, n7 - 1, n8);
                    int n9 = chunk.getBlockId(n, n7, n8);
                    if (this.isRaining() && n9 == 0 && Block.SNOW.canPlaceAt(this, n + n4, n7, n8 + n3) && n6 != 0 && n6 != Block.ICE.id && Block.BLOCKS[n6].material.blocksMovement()) {
                        this.setBlock(n + n4, n7, n8 + n3, Block.SNOW.id);
                    }
                    if (n6 == Block.WATER.id && chunk.getBlockMeta(n, n7 - 1, n8) == 0) {
                        this.setBlock(n + n4, n7 - 1, n8 + n3, Block.ICE.id);
                    }
                }
            }
            for (n2 = 0; n2 < 80; ++n2) {
                this.lcgBlockSeed = this.lcgBlockSeed * 3 + 1013904223;
                n = this.lcgBlockSeed >> 2;
                n8 = n & 0xF;
                n7 = n >> 8 & 0xF;
                n6 = n >> 16 & 0x7F;
                int n10 = chunk.blocks[n8 << 11 | n7 << 7 | n6] & 0xFF;
                if (!Block.BLOCKS_RANDOM_TICK[n10]) continue;
                Block.BLOCKS[n10].onTick(this, n8 + n4, n6, n7 + n3, this.random);
            }
        }
    }

    public boolean processScheduledTicks(boolean flush) {
        int n = this.scheduledUpdates.size();
        if (n != this.scheduledUpdateSet.size()) {
            throw new IllegalStateException("TickNextTick list out of synch");
        }
        if (n > 1000) {
            n = 1000;
        }
        for (int i = 0; i < n; ++i) {
            int n2;
            BlockEvent blockEvent = (BlockEvent)this.scheduledUpdates.first();
            if (!flush && blockEvent.ticks > this.properties.getTime()) break;
            this.scheduledUpdates.remove(blockEvent);
            this.scheduledUpdateSet.remove(blockEvent);
            int n3 = 8;
            if (!this.isRegionLoaded(blockEvent.x - n3, blockEvent.y - n3, blockEvent.z - n3, blockEvent.x + n3, blockEvent.y + n3, blockEvent.z + n3) || (n2 = this.getBlockId(blockEvent.x, blockEvent.y, blockEvent.z)) != blockEvent.blockId || n2 <= 0) continue;
            Block.BLOCKS[n2].onTick(this, blockEvent.x, blockEvent.y, blockEvent.z, this.random);
        }
        return this.scheduledUpdates.size() != 0;
    }

    @Environment(value=EnvType.CLIENT)
    public void displayTick(int x, int y, int z) {
        int n = 16;
        Random random = new Random();
        for (int i = 0; i < 1000; ++i) {
            int n2;
            int n3;
            int n4 = x + this.random.nextInt(n) - this.random.nextInt(n);
            int n5 = this.getBlockId(n4, n3 = y + this.random.nextInt(n) - this.random.nextInt(n), n2 = z + this.random.nextInt(n) - this.random.nextInt(n));
            if (n5 <= 0) continue;
            Block.BLOCKS[n5].randomDisplayTick(this, n4, n3, n2, random);
        }
    }

    public List getEntities(Entity entity, Box box) {
        this.tempEntityList.clear();
        int n = MathHelper.floor((box.minX - 2.0) / 16.0);
        int n2 = MathHelper.floor((box.maxX + 2.0) / 16.0);
        int n3 = MathHelper.floor((box.minZ - 2.0) / 16.0);
        int n4 = MathHelper.floor((box.maxZ + 2.0) / 16.0);
        for (int i = n; i <= n2; ++i) {
            for (int j = n3; j <= n4; ++j) {
                if (!this.hasChunk(i, j)) continue;
                this.getChunk(i, j).collectOtherEntities(entity, box, this.tempEntityList);
            }
        }
        return this.tempEntityList;
    }

    public List collectEntitiesByClass(Class entityClass, Box box) {
        int n = MathHelper.floor((box.minX - 2.0) / 16.0);
        int n2 = MathHelper.floor((box.maxX + 2.0) / 16.0);
        int n3 = MathHelper.floor((box.minZ - 2.0) / 16.0);
        int n4 = MathHelper.floor((box.maxZ + 2.0) / 16.0);
        ArrayList arrayList = new ArrayList();
        for (int i = n; i <= n2; ++i) {
            for (int j = n3; j <= n4; ++j) {
                if (!this.hasChunk(i, j)) continue;
                this.getChunk(i, j).collectEntitiesByClass(entityClass, box, arrayList);
            }
        }
        return arrayList;
    }

    @Environment(value=EnvType.CLIENT)
    public List getEntities() {
        return this.entities;
    }

    public void updateBlockEntity(int x, int y, int z, BlockEntity blockEntity) {
        if (this.isPosLoaded(x, y, z)) {
            this.getChunkFromPos(x, z).markDirty();
        }
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).updateBlockEntity(x, y, z, blockEntity);
        }
    }

    public int countEntities(Class entityClass) {
        int n = 0;
        for (int i = 0; i < this.entities.size(); ++i) {
            Entity entity = (Entity)this.entities.get(i);
            if (!entityClass.isAssignableFrom(entity.getClass())) continue;
            ++n;
        }
        return n;
    }

    public void addEntities(List entities) {
        this.entities.addAll(entities);
        for (int i = 0; i < entities.size(); ++i) {
            this.notifyEntityAdded((Entity)entities.get(i));
        }
    }

    public void unloadEntities(List entities) {
        this.entitiesToUnload.addAll(entities);
    }

    @Environment(value=EnvType.CLIENT)
    public void tickChunks() {
        while (this.chunkSource.tick()) {
        }
    }

    public boolean canPlace(int blockId, int x, int y, int z, boolean fallingBlock, int side) {
        int n = this.getBlockId(x, y, z);
        Block block = Block.BLOCKS[n];
        Block block2 = Block.BLOCKS[blockId];
        Box box = block2.getCollisionShape(this, x, y, z);
        if (fallingBlock) {
            box = null;
        }
        if (box != null && !this.canSpawnEntity(box)) {
            return false;
        }
        if (block == Block.FLOWING_WATER || block == Block.WATER || block == Block.FLOWING_LAVA || block == Block.LAVA || block == Block.FIRE || block == Block.SNOW) {
            block = null;
        }
        return blockId > 0 && block == null && block2.canPlaceAt(this, x, y, z, side);
    }

    public Path findPath(Entity entity, Entity target, float range) {
        int n = MathHelper.floor(entity.x);
        int n2 = MathHelper.floor(entity.y);
        int n3 = MathHelper.floor(entity.z);
        int n4 = (int)(range + 16.0f);
        int n5 = n - n4;
        int n6 = n2 - n4;
        int n7 = n3 - n4;
        int n8 = n + n4;
        int n9 = n2 + n4;
        int n10 = n3 + n4;
        WorldRegion worldRegion = new WorldRegion(this, n5, n6, n7, n8, n9, n10);
        return new PathNodeNavigator(worldRegion).findPath(entity, target, range);
    }

    public Path findPath(Entity entity, int x, int y, int z, float range) {
        int n = MathHelper.floor(entity.x);
        int n2 = MathHelper.floor(entity.y);
        int n3 = MathHelper.floor(entity.z);
        int n4 = (int)(range + 8.0f);
        int n5 = n - n4;
        int n6 = n2 - n4;
        int n7 = n3 - n4;
        int n8 = n + n4;
        int n9 = n2 + n4;
        int n10 = n3 + n4;
        WorldRegion worldRegion = new WorldRegion(this, n5, n6, n7, n8, n9, n10);
        return new PathNodeNavigator(worldRegion).findPath(entity, x, y, z, range);
    }

    public boolean canTransferPowerInDirection(int x, int y, int z, int direction) {
        int n = this.getBlockId(x, y, z);
        if (n == 0) {
            return false;
        }
        return Block.BLOCKS[n].canTransferPowerInDirection(this, x, y, z, direction);
    }

    public boolean canTransferPower(int x, int y, int z) {
        if (this.canTransferPowerInDirection(x, y - 1, z, 0)) {
            return true;
        }
        if (this.canTransferPowerInDirection(x, y + 1, z, 1)) {
            return true;
        }
        if (this.canTransferPowerInDirection(x, y, z - 1, 2)) {
            return true;
        }
        if (this.canTransferPowerInDirection(x, y, z + 1, 3)) {
            return true;
        }
        if (this.canTransferPowerInDirection(x - 1, y, z, 4)) {
            return true;
        }
        return this.canTransferPowerInDirection(x + 1, y, z, 5);
    }

    public boolean isEmittingRedstonePowerInDirection(int x, int y, int z, int direction) {
        if (this.shouldSuffocate(x, y, z)) {
            return this.canTransferPower(x, y, z);
        }
        int n = this.getBlockId(x, y, z);
        if (n == 0) {
            return false;
        }
        return Block.BLOCKS[n].isEmittingRedstonePowerInDirection(this, x, y, z, direction);
    }

    public boolean isEmittingRedstonePower(int x, int y, int z) {
        if (this.isEmittingRedstonePowerInDirection(x, y - 1, z, 0)) {
            return true;
        }
        if (this.isEmittingRedstonePowerInDirection(x, y + 1, z, 1)) {
            return true;
        }
        if (this.isEmittingRedstonePowerInDirection(x, y, z - 1, 2)) {
            return true;
        }
        if (this.isEmittingRedstonePowerInDirection(x, y, z + 1, 3)) {
            return true;
        }
        if (this.isEmittingRedstonePowerInDirection(x - 1, y, z, 4)) {
            return true;
        }
        return this.isEmittingRedstonePowerInDirection(x + 1, y, z, 5);
    }

    public PlayerEntity getClosestPlayer(Entity entity, double range) {
        return this.getClosestPlayer(entity.x, entity.y, entity.z, range);
    }

    public PlayerEntity getClosestPlayer(double x, double y, double z, double range) {
        double d = -1.0;
        PlayerEntity playerEntity = null;
        for (int i = 0; i < this.players.size(); ++i) {
            PlayerEntity playerEntity2 = (PlayerEntity)this.players.get(i);
            double d2 = playerEntity2.getSquaredDistance(x, y, z);
            if (!(range < 0.0) && !(d2 < range * range) || d != -1.0 && !(d2 < d)) continue;
            d = d2;
            playerEntity = playerEntity2;
        }
        return playerEntity;
    }

    public PlayerEntity getPlayer(String name) {
        for (int i = 0; i < this.players.size(); ++i) {
            if (!name.equals(((PlayerEntity)this.players.get((int)i)).name)) continue;
            return (PlayerEntity)this.players.get(i);
        }
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public void handleChunkDataUpdate(int x, int y, int z, int sizeX, int sizeY, int sizeZ, byte[] chunkData) {
        int n = x >> 4;
        int n2 = z >> 4;
        int n3 = x + sizeX - 1 >> 4;
        int n4 = z + sizeZ - 1 >> 4;
        int n5 = 0;
        int n6 = y;
        int n7 = y + sizeY;
        if (n6 < 0) {
            n6 = 0;
        }
        if (n7 > 128) {
            n7 = 128;
        }
        for (int i = n; i <= n3; ++i) {
            int n8 = x - i * 16;
            int n9 = x + sizeX - i * 16;
            if (n8 < 0) {
                n8 = 0;
            }
            if (n9 > 16) {
                n9 = 16;
            }
            for (int j = n2; j <= n4; ++j) {
                int n10 = z - j * 16;
                int n11 = z + sizeZ - j * 16;
                if (n10 < 0) {
                    n10 = 0;
                }
                if (n11 > 16) {
                    n11 = 16;
                }
                n5 = this.getChunk(i, j).loadFromPacket(chunkData, n8, n6, n10, n9, n7, n11, n5);
                this.setBlocksDirty(i * 16 + n8, n6, j * 16 + n10, i * 16 + n9, n7, j * 16 + n11);
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void disconnect() {
    }

    @Environment(value=EnvType.SERVER)
    public byte[] getChunkData(int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
        byte[] byArray = new byte[sizeX * sizeY * sizeZ * 5 / 2];
        int n = x >> 4;
        int n2 = z >> 4;
        int n3 = x + sizeX - 1 >> 4;
        int n4 = z + sizeZ - 1 >> 4;
        int n5 = 0;
        int n6 = y;
        int n7 = y + sizeY;
        if (n6 < 0) {
            n6 = 0;
        }
        if (n7 > 128) {
            n7 = 128;
        }
        for (int i = n; i <= n3; ++i) {
            int n8 = x - i * 16;
            int n9 = x + sizeX - i * 16;
            if (n8 < 0) {
                n8 = 0;
            }
            if (n9 > 16) {
                n9 = 16;
            }
            for (int j = n2; j <= n4; ++j) {
                int n10 = z - j * 16;
                int n11 = z + sizeZ - j * 16;
                if (n10 < 0) {
                    n10 = 0;
                }
                if (n11 > 16) {
                    n11 = 16;
                }
                n5 = this.getChunk(i, j).toPacket(byArray, n8, n6, n10, n9, n7, n11, n5);
            }
        }
        return byArray;
    }

    public void checkSessionLock() {
        this.dimensionData.checkSessionLock();
    }

    public void setTime(long time) {
        this.properties.setTime(time);
    }

    @Environment(value=EnvType.SERVER)
    public void synchronizeTimeAndUpdates(long time) {
        long l = time - this.properties.getTime();
        for (BlockEvent blockEvent : this.scheduledUpdateSet) {
            blockEvent.ticks += l;
        }
        this.setTime(time);
    }

    public long getSeed() {
        return this.properties.getSeed();
    }

    public long getTime() {
        return this.properties.getTime();
    }

    public Vec3i getSpawnPos() {
        return new Vec3i(this.properties.getSpawnX(), this.properties.getSpawnY(), this.properties.getSpawnZ());
    }

    @Environment(value=EnvType.CLIENT)
    public void setSpawnPos(Vec3i pos) {
        this.properties.setSpawn(pos.x, pos.y, pos.z);
    }

    @Environment(value=EnvType.CLIENT)
    public void loadChunksNearEntity(Entity entity) {
        int n = MathHelper.floor(entity.x / 16.0);
        int n2 = MathHelper.floor(entity.z / 16.0);
        int n3 = 2;
        for (int i = n - n3; i <= n + n3; ++i) {
            for (int j = n2 - n3; j <= n2 + n3; ++j) {
                this.getChunk(i, j);
            }
        }
        if (!this.entities.contains(entity)) {
            this.entities.add(entity);
        }
    }

    public boolean canInteract(PlayerEntity player, int x, int y, int z) {
        return true;
    }

    public void broadcastEntityEvent(Entity entity, byte event) {
    }

    @Environment(value=EnvType.CLIENT)
    public void updateEntityLists() {
        int n;
        int n2;
        Entity entity;
        int n3;
        this.entities.removeAll(this.entitiesToUnload);
        for (n3 = 0; n3 < this.entitiesToUnload.size(); ++n3) {
            entity = (Entity)this.entitiesToUnload.get(n3);
            n2 = entity.chunkX;
            n = entity.chunkZ;
            if (!entity.isPersistent || !this.hasChunk(n2, n)) continue;
            this.getChunk(n2, n).removeEntity(entity);
        }
        for (n3 = 0; n3 < this.entitiesToUnload.size(); ++n3) {
            this.notifyEntityRemoved((Entity)this.entitiesToUnload.get(n3));
        }
        this.entitiesToUnload.clear();
        for (n3 = 0; n3 < this.entities.size(); ++n3) {
            entity = (Entity)this.entities.get(n3);
            if (entity.vehicle != null) {
                if (!entity.vehicle.dead && entity.vehicle.passenger == entity) continue;
                entity.vehicle.passenger = null;
                entity.vehicle = null;
            }
            if (!entity.dead) continue;
            n2 = entity.chunkX;
            n = entity.chunkZ;
            if (entity.isPersistent && this.hasChunk(n2, n)) {
                this.getChunk(n2, n).removeEntity(entity);
            }
            this.entities.remove(n3--);
            this.notifyEntityRemoved(entity);
        }
    }

    public ChunkSource getChunkSource() {
        return this.chunkSource;
    }

    public void playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch) {
        int n = this.getBlockId(x, y, z);
        if (n > 0) {
            Block.BLOCKS[n].onBlockAction(this, x, y, z, soundType, pitch);
        }
    }

    @Environment(value=EnvType.SERVER)
    public WorldStorage getDimensionData() {
        return this.dimensionData;
    }

    public WorldProperties getProperties() {
        return this.properties;
    }

    public void updateSleepingPlayers() {
        this.allPlayersSleeping = !this.players.isEmpty();
        for (PlayerEntity playerEntity : this.players) {
            if (playerEntity.isSleeping()) continue;
            this.allPlayersSleeping = false;
            break;
        }
    }

    protected void afterSkipNight() {
        this.allPlayersSleeping = false;
        for (PlayerEntity playerEntity : this.players) {
            if (!playerEntity.isSleeping()) continue;
            playerEntity.wakeUp(false, false, true);
        }
        this.clearWeather();
    }

    public boolean canSkipNight() {
        if (this.allPlayersSleeping && !this.isRemote) {
            for (PlayerEntity playerEntity : this.players) {
                if (playerEntity.isFullyAsleep()) continue;
                return false;
            }
            return true;
        }
        return false;
    }

    public float getThunderGradient(float delta) {
        return (this.thunderGradientPrev + (this.thunderGradient - this.thunderGradientPrev) * delta) * this.getRainGradient(delta);
    }

    public float getRainGradient(float delta) {
        return this.rainGradientPrev + (this.rainGradient - this.rainGradientPrev) * delta;
    }

    @Environment(value=EnvType.CLIENT)
    public void setRainGradient(float rainGradient) {
        this.rainGradientPrev = rainGradient;
        this.rainGradient = rainGradient;
    }

    public boolean isThundering() {
        return (double)this.getThunderGradient(1.0f) > 0.9;
    }

    public boolean isRaining() {
        return (double)this.getRainGradient(1.0f) > 0.2;
    }

    public boolean isRaining(int x, int y, int z) {
        if (!this.isRaining()) {
            return false;
        }
        if (!this.hasSkyLight(x, y, z)) {
            return false;
        }
        if (this.getTopSolidBlockY(x, z) > y) {
            return false;
        }
        Biome biome = this.getBiomeSource().getBiome(x, z);
        if (biome.canSnow()) {
            return false;
        }
        return biome.canRain();
    }

    public void setState(String id, PersistentState state) {
        this.persistentStateManager.set(id, state);
    }

    public PersistentState getOrCreateState(Class stateClass, String id) {
        return this.persistentStateManager.getOrCreate(stateClass, id);
    }

    public int getIdCount(String id) {
        return this.persistentStateManager.getIdCount(id);
    }

    public void worldEvent(int event, int x, int y, int z, int data) {
        this.worldEvent(null, event, x, y, z, data);
    }

    public void worldEvent(PlayerEntity player, int event, int x, int y, int z, int data) {
        for (int i = 0; i < this.eventListeners.size(); ++i) {
            ((GameEventListener)this.eventListeners.get(i)).worldEvent(player, event, x, y, z, data);
        }
    }
}

