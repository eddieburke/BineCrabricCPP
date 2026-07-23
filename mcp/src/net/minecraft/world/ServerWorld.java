/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.WaterCreatureEntity;
import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import net.minecraft.network.packet.s2c.play.GameStateChangeS2CPacket;
import net.minecraft.network.packet.s2c.play.GlobalEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayNoteSoundS2CPacket;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.world.chunk.ServerChunkCache;
import net.minecraft.util.IntHashMap;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.storage.ChunkStorage;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.explosion.Explosion;
import net.minecraft.world.storage.WorldStorage;

@Environment(value=EnvType.SERVER)
public class ServerWorld
extends World {
    public ServerChunkCache chunkCache;
    public boolean bypassSpawnProtection = false;
    public boolean savingDisabled;
    private MinecraftServer server;
    private IntHashMap entitiesById = new IntHashMap();

    public ServerWorld(MinecraftServer server, WorldStorage storage, String name, int dimensionId, long seed) {
        super(storage, name, seed, Dimension.fromId(dimensionId));
        this.server = server;
    }

    public void updateEntity(Entity entity, boolean requireLoaded) {
        if (!this.server.spawnAnimals && (entity instanceof AnimalEntity || entity instanceof WaterCreatureEntity)) {
            entity.markDead();
        }
        if (entity.passenger == null || !(entity.passenger instanceof PlayerEntity)) {
            super.updateEntity(entity, requireLoaded);
        }
    }

    public void tickVehicle(Entity vehicle, boolean requireLoaded) {
        super.updateEntity(vehicle, requireLoaded);
    }

    protected ChunkSource createChunkCache() {
        ChunkStorage chunkStorage = this.dimensionData.getChunkStorage(this.dimension);
        this.chunkCache = new ServerChunkCache(this, chunkStorage, this.dimension.createChunkGenerator());
        return this.chunkCache;
    }

    public List getBlockEntities(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        ArrayList<BlockEntity> arrayList = new ArrayList<BlockEntity>();
        for (int i = 0; i < this.blockEntities.size(); ++i) {
            BlockEntity blockEntity = (BlockEntity)this.blockEntities.get(i);
            if (blockEntity.x < minX || blockEntity.y < minY || blockEntity.z < minZ || blockEntity.x >= maxX || blockEntity.y >= maxY || blockEntity.z >= maxZ) continue;
            arrayList.add(blockEntity);
        }
        return arrayList;
    }

    public boolean canInteract(PlayerEntity player, int x, int y, int z) {
        int n;
        int n2 = (int)MathHelper.abs(x - this.properties.getSpawnX());
        if (n2 > (n = (int)MathHelper.abs(z - this.properties.getSpawnZ()))) {
            n = n2;
        }
        return n > 16 || this.server.playerManager.isOperator(player.name);
    }

    protected void notifyEntityAdded(Entity entity) {
        super.notifyEntityAdded(entity);
        this.entitiesById.put(entity.id, entity);
    }

    protected void notifyEntityRemoved(Entity entity) {
        super.notifyEntityRemoved(entity);
        this.entitiesById.remove(entity.id);
    }

    public Entity getEntity(int id) {
        return (Entity)this.entitiesById.get(id);
    }

    public boolean spawnGlobalEntity(Entity entity) {
        if (super.spawnGlobalEntity(entity)) {
            this.server.playerManager.sendToAround(entity.x, entity.y, entity.z, 512.0, this.dimension.id, new GlobalEntitySpawnS2CPacket(entity));
            return true;
        }
        return false;
    }

    public void broadcastEntityEvent(Entity entity, byte event) {
        EntityStatusS2CPacket entityStatusS2CPacket = new EntityStatusS2CPacket(entity.id, event);
        this.server.getEntityTracker(this.dimension.id).sendToAround(entity, entityStatusS2CPacket);
    }

    public Explosion createExplosion(Entity source, double x, double y, double z, float power, boolean fire) {
        Explosion explosion = new Explosion(this, source, x, y, z, power);
        explosion.fire = fire;
        explosion.explode();
        explosion.playExplosionSound(false);
        this.server.playerManager.sendToAround(x, y, z, 64.0, this.dimension.id, new ExplosionS2CPacket(x, y, z, power, explosion.damagedBlocks));
        return explosion;
    }

    public void playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch) {
        super.playNoteBlockActionAt(x, y, z, soundType, pitch);
        this.server.playerManager.sendToAround(x, y, z, 64.0, this.dimension.id, new PlayNoteSoundS2CPacket(x, y, z, soundType, pitch));
    }

    public void forceSave() {
        this.dimensionData.forceSave();
    }

    protected void updateWeatherCycles() {
        boolean bl = this.isRaining();
        super.updateWeatherCycles();
        if (bl != this.isRaining()) {
            if (bl) {
                this.server.playerManager.sendToAll(new GameStateChangeS2CPacket(2));
            } else {
                this.server.playerManager.sendToAll(new GameStateChangeS2CPacket(1));
            }
        }
    }
}

