/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.world.chunk.MultiplayerChunkCache;
import net.minecraft.entity.Entity;
import net.minecraft.network.packet.play.DisconnectPacket;
import net.minecraft.util.IntHashMap;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.World;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.event.listener.GameEventListener;
import net.minecraft.world.storage.EmptyWorldStorage;
import net.minecraft.world.storage.WorldStorage;

@Environment(value=EnvType.CLIENT)
public class ClientWorld
extends World {
    private LinkedList blockResets = new LinkedList();
    private ClientNetworkHandler networkHandler;
    private MultiplayerChunkCache chunkCache;
    private IntHashMap entitiesByNetworkId = new IntHashMap();
    private Set forcedEntities = new HashSet();
    private Set pendingEntities = new HashSet();

    public ClientWorld(ClientNetworkHandler networkHandler, long seed, int dimensionId) {
        super((WorldStorage)new EmptyWorldStorage(), "MpServer", Dimension.fromId(dimensionId), seed);
        this.networkHandler = networkHandler;
        this.setSpawnPos(new Vec3i(8, 64, 8));
        this.persistentStateManager = networkHandler.clientPersistentStateManager;
    }

    public void tick() {
        Object object;
        int n;
        this.setTime(this.getTime() + 1L);
        int n2 = this.getAmbientDarkness(1.0f);
        if (n2 != this.ambientDarkness) {
            this.ambientDarkness = n2;
            for (n = 0; n < this.eventListeners.size(); ++n) {
                ((GameEventListener)this.eventListeners.get(n)).notifyAmbientDarknessChanged();
            }
        }
        for (n = 0; n < 10 && !this.pendingEntities.isEmpty(); ++n) {
            object = (Entity)this.pendingEntities.iterator().next();
            if (this.entities.contains(object)) continue;
            this.spawnEntity((Entity)object);
        }
        this.networkHandler.tick();
        for (n = 0; n < this.blockResets.size(); ++n) {
            object = (BlockReset)this.blockResets.get(n);
            if (--((BlockReset)object).delay != 0) continue;
            super.setBlockWithoutNotifyingNeighbors(((BlockReset)object).x, ((BlockReset)object).y, ((BlockReset)object).z, ((BlockReset)object).block, ((BlockReset)object).meta);
            super.blockUpdateEvent(((BlockReset)object).x, ((BlockReset)object).y, ((BlockReset)object).z);
            this.blockResets.remove(n--);
        }
    }

    public void clearBlockResets(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        for (int i = 0; i < this.blockResets.size(); ++i) {
            BlockReset blockReset = (BlockReset)this.blockResets.get(i);
            if (blockReset.x < minX || blockReset.y < minY || blockReset.z < minZ || blockReset.x > maxX || blockReset.y > maxY || blockReset.z > maxZ) continue;
            this.blockResets.remove(i--);
        }
    }

    protected ChunkSource createChunkCache() {
        this.chunkCache = new MultiplayerChunkCache(this);
        return this.chunkCache;
    }

    public void updateSpawnPosition() {
        this.setSpawnPos(new Vec3i(8, 64, 8));
    }

    protected void manageChunkUpdatesAndEvents() {
    }

    public void scheduleBlockUpdate(int x, int y, int z, int id, int tickRate) {
    }

    public boolean processScheduledTicks(boolean flush) {
        return false;
    }

    public void updateChunk(int chunkX, int chunkZ, boolean load) {
        if (load) {
            this.chunkCache.loadChunk(chunkX, chunkZ);
        } else {
            this.chunkCache.unloadChunk(chunkX, chunkZ);
        }
        if (!load) {
            this.setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, 128, chunkZ * 16 + 15);
        }
    }

    public boolean spawnEntity(Entity entity) {
        boolean bl = super.spawnEntity(entity);
        this.forcedEntities.add(entity);
        if (!bl) {
            this.pendingEntities.add(entity);
        }
        return bl;
    }

    public void remove(Entity entity) {
        super.remove(entity);
        this.forcedEntities.remove(entity);
    }

    protected void notifyEntityAdded(Entity entity) {
        super.notifyEntityAdded(entity);
        if (this.pendingEntities.contains(entity)) {
            this.pendingEntities.remove(entity);
        }
    }

    protected void notifyEntityRemoved(Entity entity) {
        super.notifyEntityRemoved(entity);
        if (this.forcedEntities.contains(entity)) {
            this.pendingEntities.add(entity);
        }
    }

    public void forceEntity(int id, Entity entity) {
        Entity entity2 = this.getEntity(id);
        if (entity2 != null) {
            this.remove(entity2);
        }
        this.forcedEntities.add(entity);
        entity.id = id;
        if (!this.spawnEntity(entity)) {
            this.pendingEntities.add(entity);
        }
        this.entitiesByNetworkId.put(id, entity);
    }

    public Entity getEntity(int id) {
        return (Entity)this.entitiesByNetworkId.get(id);
    }

    public Entity removeEntity(int id) {
        Entity entity = (Entity)this.entitiesByNetworkId.remove(id);
        if (entity != null) {
            this.forcedEntities.remove(entity);
            this.remove(entity);
        }
        return entity;
    }

    public boolean setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) {
        int n = this.getBlockId(x, y, z);
        int n2 = this.getBlockMeta(x, y, z);
        if (super.setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta)) {
            this.blockResets.add(new BlockReset(x, y, z, n, n2));
            return true;
        }
        return false;
    }

    public boolean setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta) {
        int n = this.getBlockId(x, y, z);
        int n2 = this.getBlockMeta(x, y, z);
        if (super.setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta)) {
            this.blockResets.add(new BlockReset(x, y, z, n, n2));
            return true;
        }
        return false;
    }

    public boolean setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId) {
        int n = this.getBlockId(x, y, z);
        int n2 = this.getBlockMeta(x, y, z);
        if (super.setBlockWithoutNotifyingNeighbors(x, y, z, blockId)) {
            this.blockResets.add(new BlockReset(x, y, z, n, n2));
            return true;
        }
        return false;
    }

    public boolean setBlockWithMetaFromPacket(int x, int y, int z, int blockId, int meta) {
        this.clearBlockResets(x, y, z, x, y, z);
        if (super.setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta)) {
            this.blockUpdate(x, y, z, blockId);
            return true;
        }
        return false;
    }

    public void disconnect() {
        this.networkHandler.sendPacketAndDisconnect(new DisconnectPacket("Quitting"));
    }

    protected void updateWeatherCycles() {
        if (this.dimension.hasCeiling) {
            return;
        }
        if (this.ticksSinceLightning > 0) {
            --this.ticksSinceLightning;
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

    @Environment(value=EnvType.CLIENT)
    class BlockReset {
        int x;
        int y;
        int z;
        int delay;
        int block;
        int meta;

        public BlockReset(int x, int y, int z, int block, int meta) {
            this.x = x;
            this.y = y;
            this.z = z;
            this.delay = 80;
            this.block = block;
            this.meta = meta;
        }
    }
}

