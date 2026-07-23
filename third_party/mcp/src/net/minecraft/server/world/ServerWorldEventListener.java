/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.world;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.network.packet.s2c.play.WorldEventS2CPacket;
import net.minecraft.server.MinecraftServer;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.event.listener.GameEventListener;

@Environment(value=EnvType.SERVER)
public class ServerWorldEventListener
implements GameEventListener {
    private MinecraftServer server;
    private ServerWorld world;

    public ServerWorldEventListener(MinecraftServer server, ServerWorld world) {
        this.server = server;
        this.world = world;
    }

    public void addParticle(String particle, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
    }

    public void notifyEntityAdded(Entity entity) {
        this.server.getEntityTracker(this.world.dimension.id).onEntityAdded(entity);
    }

    public void notifyEntityRemoved(Entity entity) {
        this.server.getEntityTracker(this.world.dimension.id).onEntityRemoved(entity);
    }

    public void playSound(String sound, double x, double y, double z, float volume, float pitch) {
    }

    public void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
    }

    public void notifyAmbientDarknessChanged() {
    }

    public void blockUpdate(int x, int y, int z) {
        this.server.playerManager.markDirty(x, y, z, this.world.dimension.id);
    }

    public void playStreaming(String stream, int x, int y, int z) {
    }

    public void updateBlockEntity(int x, int y, int z, BlockEntity blockEntity) {
        this.server.playerManager.updateBlockEntity(x, y, z, blockEntity);
    }

    public void worldEvent(PlayerEntity player, int event, int x, int y, int z, int data) {
        this.server.playerManager.sendToAround(player, x, y, z, 64.0, this.world.dimension.id, new WorldEventS2CPacket(event, x, y, z, data));
    }
}

