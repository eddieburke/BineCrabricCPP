/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.entity;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.SpawnableEntity;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.data.DataTracker;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.s2c.play.EntityDestroyS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityEquipmentUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityMoveRelativeS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityRotateAndMoveRelativeS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityRotateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityTrackerUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.LivingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.PaintingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSleepUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnS2CPacket;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.SERVER)
public class EntityTrackerEntry {
    public Entity currentTrackedEntity;
    public int trackedDistance;
    public int trackingFrequency;
    public int lastX;
    public int lastY;
    public int lastZ;
    public int lastYaw;
    public int lastPitch;
    public double velocityX;
    public double velocityY;
    public double velocityZ;
    public int ticks = 0;
    private double x;
    private double y;
    private double z;
    private boolean isInitialized = false;
    private boolean alwaysUpdateVelocity;
    private int ticksSinceLastDismount = 0;
    public boolean newPlayerDataUpdated = false;
    public Set listeners = new HashSet();

    public EntityTrackerEntry(Entity entity, int trackedDistance, int trackedFrequency, boolean alwaysUpdateVelocity) {
        this.currentTrackedEntity = entity;
        this.trackedDistance = trackedDistance;
        this.trackingFrequency = trackedFrequency;
        this.alwaysUpdateVelocity = alwaysUpdateVelocity;
        this.lastX = MathHelper.floor(entity.x * 32.0);
        this.lastY = MathHelper.floor(entity.y * 32.0);
        this.lastZ = MathHelper.floor(entity.z * 32.0);
        this.lastYaw = MathHelper.floor(entity.yaw * 256.0f / 360.0f);
        this.lastPitch = MathHelper.floor(entity.pitch * 256.0f / 360.0f);
    }

    public boolean equals(Object object) {
        if (object instanceof EntityTrackerEntry) {
            return ((EntityTrackerEntry)object).currentTrackedEntity.id == this.currentTrackedEntity.id;
        }
        return false;
    }

    public int hashCode() {
        return this.currentTrackedEntity.id;
    }

    public void notifyNewLocation(List players) {
        this.newPlayerDataUpdated = false;
        if (!this.isInitialized || this.currentTrackedEntity.getSquaredDistance(this.x, this.y, this.z) > 16.0) {
            this.x = this.currentTrackedEntity.x;
            this.y = this.currentTrackedEntity.y;
            this.z = this.currentTrackedEntity.z;
            this.isInitialized = true;
            this.newPlayerDataUpdated = true;
            this.updateListeners(players);
        }
        ++this.ticksSinceLastDismount;
        if (++this.ticks % this.trackingFrequency == 0) {
            DataTracker dataTracker;
            double d;
            double d2;
            double d3;
            double d4;
            double d5;
            boolean bl;
            int n = MathHelper.floor(this.currentTrackedEntity.x * 32.0);
            int n2 = MathHelper.floor(this.currentTrackedEntity.y * 32.0);
            int n3 = MathHelper.floor(this.currentTrackedEntity.z * 32.0);
            int n4 = MathHelper.floor(this.currentTrackedEntity.yaw * 256.0f / 360.0f);
            int n5 = MathHelper.floor(this.currentTrackedEntity.pitch * 256.0f / 360.0f);
            int n6 = n - this.lastX;
            int n7 = n2 - this.lastY;
            int n8 = n3 - this.lastZ;
            Packet packet = null;
            boolean bl2 = Math.abs(n) >= 8 || Math.abs(n2) >= 8 || Math.abs(n3) >= 8;
            boolean bl3 = bl = Math.abs(n4 - this.lastYaw) >= 8 || Math.abs(n5 - this.lastPitch) >= 8;
            if (n6 < -128 || n6 >= 128 || n7 < -128 || n7 >= 128 || n8 < -128 || n8 >= 128 || this.ticksSinceLastDismount > 400) {
                this.ticksSinceLastDismount = 0;
                this.currentTrackedEntity.x = (double)n / 32.0;
                this.currentTrackedEntity.y = (double)n2 / 32.0;
                this.currentTrackedEntity.z = (double)n3 / 32.0;
                packet = new EntityPositionS2CPacket(this.currentTrackedEntity.id, n, n2, n3, (byte)n4, (byte)n5);
            } else if (bl2 && bl) {
                packet = new EntityRotateAndMoveRelativeS2CPacket(this.currentTrackedEntity.id, (byte)n6, (byte)n7, (byte)n8, (byte)n4, (byte)n5);
            } else if (bl2) {
                packet = new EntityMoveRelativeS2CPacket(this.currentTrackedEntity.id, (byte)n6, (byte)n7, (byte)n8);
            } else if (bl) {
                packet = new EntityRotateS2CPacket(this.currentTrackedEntity.id, (byte)n4, (byte)n5);
            }
            if (this.alwaysUpdateVelocity && ((d5 = (d4 = this.currentTrackedEntity.velocityX - this.velocityX) * d4 + (d3 = this.currentTrackedEntity.velocityY - this.velocityY) * d3 + (d2 = this.currentTrackedEntity.velocityZ - this.velocityZ) * d2) > (d = 0.02) * d || d5 > 0.0 && this.currentTrackedEntity.velocityX == 0.0 && this.currentTrackedEntity.velocityY == 0.0 && this.currentTrackedEntity.velocityZ == 0.0)) {
                this.velocityX = this.currentTrackedEntity.velocityX;
                this.velocityY = this.currentTrackedEntity.velocityY;
                this.velocityZ = this.currentTrackedEntity.velocityZ;
                this.sendToListeners(new EntityVelocityUpdateS2CPacket(this.currentTrackedEntity.id, this.velocityX, this.velocityY, this.velocityZ));
            }
            if (packet != null) {
                this.sendToListeners(packet);
            }
            if ((dataTracker = this.currentTrackedEntity.getDataTracker()).isDirty()) {
                this.sendToAround(new EntityTrackerUpdateS2CPacket(this.currentTrackedEntity.id, dataTracker));
            }
            if (bl2) {
                this.lastX = n;
                this.lastY = n2;
                this.lastZ = n3;
            }
            if (bl) {
                this.lastYaw = n4;
                this.lastPitch = n5;
            }
        }
        if (this.currentTrackedEntity.velocityModified) {
            this.sendToAround(new EntityVelocityUpdateS2CPacket(this.currentTrackedEntity));
            this.currentTrackedEntity.velocityModified = false;
        }
    }

    public void sendToListeners(Packet packet) {
        for (ServerPlayerEntity serverPlayerEntity : this.listeners) {
            serverPlayerEntity.networkHandler.sendPacket(packet);
        }
    }

    public void sendToAround(Packet packet) {
        this.sendToListeners(packet);
        if (this.currentTrackedEntity instanceof ServerPlayerEntity) {
            ((ServerPlayerEntity)this.currentTrackedEntity).networkHandler.sendPacket(packet);
        }
    }

    public void notifyEntityRemoved() {
        this.sendToListeners(new EntityDestroyS2CPacket(this.currentTrackedEntity.id));
    }

    public void notifyEntityRemoved(ServerPlayerEntity player) {
        if (this.listeners.contains(player)) {
            this.listeners.remove(player);
        }
    }

    public void updateListener(ServerPlayerEntity player) {
        if (player == this.currentTrackedEntity) {
            return;
        }
        double d = player.x - (double)(this.lastX / 32);
        double d2 = player.z - (double)(this.lastZ / 32);
        if (d >= (double)(-this.trackedDistance) && d <= (double)this.trackedDistance && d2 >= (double)(-this.trackedDistance) && d2 <= (double)this.trackedDistance) {
            if (!this.listeners.contains(player)) {
                PlayerEntity playerEntity;
                ItemStack[] itemStackArray;
                this.listeners.add(player);
                player.networkHandler.sendPacket(this.createAddEntityPacket());
                if (this.alwaysUpdateVelocity) {
                    player.networkHandler.sendPacket(new EntityVelocityUpdateS2CPacket(this.currentTrackedEntity.id, this.currentTrackedEntity.velocityX, this.currentTrackedEntity.velocityY, this.currentTrackedEntity.velocityZ));
                }
                if ((itemStackArray = this.currentTrackedEntity.getEquipment()) != null) {
                    for (int i = 0; i < itemStackArray.length; ++i) {
                        player.networkHandler.sendPacket(new EntityEquipmentUpdateS2CPacket(this.currentTrackedEntity.id, i, itemStackArray[i]));
                    }
                }
                if (this.currentTrackedEntity instanceof PlayerEntity && (playerEntity = (PlayerEntity)this.currentTrackedEntity).isSleeping()) {
                    player.networkHandler.sendPacket(new PlayerSleepUpdateS2CPacket(this.currentTrackedEntity, 0, MathHelper.floor(this.currentTrackedEntity.x), MathHelper.floor(this.currentTrackedEntity.y), MathHelper.floor(this.currentTrackedEntity.z)));
                }
            }
        } else if (this.listeners.contains(player)) {
            this.listeners.remove(player);
            player.networkHandler.sendPacket(new EntityDestroyS2CPacket(this.currentTrackedEntity.id));
        }
    }

    public void updateListeners(List players) {
        for (int i = 0; i < players.size(); ++i) {
            this.updateListener((ServerPlayerEntity)players.get(i));
        }
    }

    private Packet createAddEntityPacket() {
        Entity entity;
        if (this.currentTrackedEntity instanceof ItemEntity) {
            ItemEntity itemEntity = (ItemEntity)this.currentTrackedEntity;
            ItemEntitySpawnS2CPacket itemEntitySpawnS2CPacket = new ItemEntitySpawnS2CPacket(itemEntity);
            itemEntity.x = (double)itemEntitySpawnS2CPacket.x / 32.0;
            itemEntity.y = (double)itemEntitySpawnS2CPacket.y / 32.0;
            itemEntity.z = (double)itemEntitySpawnS2CPacket.z / 32.0;
            return itemEntitySpawnS2CPacket;
        }
        if (this.currentTrackedEntity instanceof ServerPlayerEntity) {
            return new PlayerSpawnS2CPacket((PlayerEntity)this.currentTrackedEntity);
        }
        if (this.currentTrackedEntity instanceof MinecartEntity) {
            entity = (MinecartEntity)this.currentTrackedEntity;
            if (entity.type == 0) {
                return new EntitySpawnS2CPacket(this.currentTrackedEntity, 10);
            }
            if (entity.type == 1) {
                return new EntitySpawnS2CPacket(this.currentTrackedEntity, 11);
            }
            if (entity.type == 2) {
                return new EntitySpawnS2CPacket(this.currentTrackedEntity, 12);
            }
        }
        if (this.currentTrackedEntity instanceof BoatEntity) {
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 1);
        }
        if (this.currentTrackedEntity instanceof SpawnableEntity) {
            return new LivingEntitySpawnS2CPacket((LivingEntity)this.currentTrackedEntity);
        }
        if (this.currentTrackedEntity instanceof FishingBobberEntity) {
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 90);
        }
        if (this.currentTrackedEntity instanceof ArrowEntity) {
            entity = ((ArrowEntity)this.currentTrackedEntity).owner;
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 60, entity != null ? ((LivingEntity)entity).id : this.currentTrackedEntity.id);
        }
        if (this.currentTrackedEntity instanceof SnowballEntity) {
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 61);
        }
        if (this.currentTrackedEntity instanceof FireballEntity) {
            entity = (FireballEntity)this.currentTrackedEntity;
            EntitySpawnS2CPacket entitySpawnS2CPacket = new EntitySpawnS2CPacket(this.currentTrackedEntity, 63, ((FireballEntity)this.currentTrackedEntity).owner.id);
            entitySpawnS2CPacket.velocityX = (int)(((FireballEntity)entity).powerX * 8000.0);
            entitySpawnS2CPacket.velocityY = (int)(((FireballEntity)entity).powerY * 8000.0);
            entitySpawnS2CPacket.velocityZ = (int)(((FireballEntity)entity).powerZ * 8000.0);
            return entitySpawnS2CPacket;
        }
        if (this.currentTrackedEntity instanceof EggEntity) {
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 62);
        }
        if (this.currentTrackedEntity instanceof TntEntity) {
            return new EntitySpawnS2CPacket(this.currentTrackedEntity, 50);
        }
        if (this.currentTrackedEntity instanceof FallingBlockEntity) {
            entity = (FallingBlockEntity)this.currentTrackedEntity;
            if (((FallingBlockEntity)entity).blockId == Block.SAND.id) {
                return new EntitySpawnS2CPacket(this.currentTrackedEntity, 70);
            }
            if (((FallingBlockEntity)entity).blockId == Block.GRAVEL.id) {
                return new EntitySpawnS2CPacket(this.currentTrackedEntity, 71);
            }
        }
        if (this.currentTrackedEntity instanceof PaintingEntity) {
            return new PaintingEntitySpawnS2CPacket((PaintingEntity)this.currentTrackedEntity);
        }
        throw new IllegalArgumentException("Don't know how to add " + this.currentTrackedEntity.getClass() + "!");
    }

    public void removeListener(ServerPlayerEntity player) {
        if (this.listeners.contains(player)) {
            this.listeners.remove(player);
            player.networkHandler.sendPacket(new EntityDestroyS2CPacket(this.currentTrackedEntity.id));
        }
    }
}

