/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.entity;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.SpawnableEntity;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.passive.SquidEntity;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.network.packet.Packet;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.entity.EntityTrackerEntry;
import net.minecraft.util.IntHashMap;

@Environment(value=EnvType.SERVER)
public class EntityTracker {
    private Set entries = new HashSet();
    private IntHashMap entriesById = new IntHashMap();
    private MinecraftServer world;
    private int viewDistance;
    private int dimensionId;

    public EntityTracker(MinecraftServer server, int dimensionId) {
        this.world = server;
        this.dimensionId = dimensionId;
        this.viewDistance = server.playerManager.getBlockViewDistance();
    }

    public void onEntityAdded(Entity entity) {
        if (entity instanceof ServerPlayerEntity) {
            this.startTracking(entity, 512, 2);
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)entity;
            for (EntityTrackerEntry entityTrackerEntry : this.entries) {
                if (entityTrackerEntry.currentTrackedEntity == serverPlayerEntity) continue;
                entityTrackerEntry.updateListener(serverPlayerEntity);
            }
        } else if (entity instanceof FishingBobberEntity) {
            this.startTracking(entity, 64, 5, true);
        } else if (entity instanceof ArrowEntity) {
            this.startTracking(entity, 64, 20, false);
        } else if (entity instanceof FireballEntity) {
            this.startTracking(entity, 64, 10, false);
        } else if (entity instanceof SnowballEntity) {
            this.startTracking(entity, 64, 10, true);
        } else if (entity instanceof EggEntity) {
            this.startTracking(entity, 64, 10, true);
        } else if (entity instanceof ItemEntity) {
            this.startTracking(entity, 64, 20, true);
        } else if (entity instanceof MinecartEntity) {
            this.startTracking(entity, 160, 5, true);
        } else if (entity instanceof BoatEntity) {
            this.startTracking(entity, 160, 5, true);
        } else if (entity instanceof SquidEntity) {
            this.startTracking(entity, 160, 3, true);
        } else if (entity instanceof SpawnableEntity) {
            this.startTracking(entity, 160, 3);
        } else if (entity instanceof TntEntity) {
            this.startTracking(entity, 160, 10, true);
        } else if (entity instanceof FallingBlockEntity) {
            this.startTracking(entity, 160, 20, true);
        } else if (entity instanceof PaintingEntity) {
            this.startTracking(entity, 160, Integer.MAX_VALUE, false);
        }
    }

    public void startTracking(Entity entity, int trackedDistance, int tracingFrequency) {
        this.startTracking(entity, trackedDistance, tracingFrequency, false);
    }

    public void startTracking(Entity entity, int trackedDistance, int tracingFrequency, boolean alwaysUpdateVelocity) {
        if (trackedDistance > this.viewDistance) {
            trackedDistance = this.viewDistance;
        }
        if (this.entriesById.containsKey(entity.id)) {
            throw new IllegalStateException("Entity is already tracked!");
        }
        EntityTrackerEntry entityTrackerEntry = new EntityTrackerEntry(entity, trackedDistance, tracingFrequency, alwaysUpdateVelocity);
        this.entries.add(entityTrackerEntry);
        this.entriesById.put(entity.id, entityTrackerEntry);
        entityTrackerEntry.updateListeners(this.world.getWorld((int)this.dimensionId).players);
    }

    public void onEntityRemoved(Entity entity) {
        Object object;
        if (entity instanceof ServerPlayerEntity) {
            object = (ServerPlayerEntity)entity;
            for (EntityTrackerEntry entityTrackerEntry : this.entries) {
                entityTrackerEntry.notifyEntityRemoved((ServerPlayerEntity)object);
            }
        }
        if ((object = (EntityTrackerEntry)this.entriesById.remove(entity.id)) != null) {
            this.entries.remove(object);
            ((EntityTrackerEntry)object).notifyEntityRemoved();
        }
    }

    public void tick() {
        ArrayList<ServerPlayerEntity> arrayList = new ArrayList<ServerPlayerEntity>();
        for (Object object : this.entries) {
            ((EntityTrackerEntry)object).notifyNewLocation(this.world.getWorld((int)this.dimensionId).players);
            if (!((EntityTrackerEntry)object).newPlayerDataUpdated || !(((EntityTrackerEntry)object).currentTrackedEntity instanceof ServerPlayerEntity)) continue;
            arrayList.add((ServerPlayerEntity)((EntityTrackerEntry)object).currentTrackedEntity);
        }
        for (int i = 0; i < arrayList.size(); ++i) {
            Object object;
            object = (ServerPlayerEntity)arrayList.get(i);
            for (EntityTrackerEntry entityTrackerEntry : this.entries) {
                if (entityTrackerEntry.currentTrackedEntity == object) continue;
                entityTrackerEntry.updateListener((ServerPlayerEntity)object);
            }
        }
    }

    public void sendToListeners(Entity entity, Packet packet) {
        EntityTrackerEntry entityTrackerEntry = (EntityTrackerEntry)this.entriesById.get(entity.id);
        if (entityTrackerEntry != null) {
            entityTrackerEntry.sendToListeners(packet);
        }
    }

    public void sendToAround(Entity entity, Packet packet) {
        EntityTrackerEntry entityTrackerEntry = (EntityTrackerEntry)this.entriesById.get(entity.id);
        if (entityTrackerEntry != null) {
            entityTrackerEntry.sendToAround(packet);
        }
    }

    public void removeListener(ServerPlayerEntity player) {
        for (EntityTrackerEntry entityTrackerEntry : this.entries) {
            entityTrackerEntry.removeListener(player);
        }
    }
}

