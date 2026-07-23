/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.EntityRegistry;
import net.minecraft.entity.LivingEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.Box;

public class MobSpawnerBlockEntity
extends BlockEntity {
    public int spawnDelay = 20;
    private String spawnedEntityId = "Pig";
    public double rotation;
    public double lastRotation = 0.0;

    @Environment(value=EnvType.CLIENT)
    public String getSpawnedEntityId() {
        return this.spawnedEntityId;
    }

    public void setSpawnedEntityId(String spawnedEntityId) {
        this.spawnedEntityId = spawnedEntityId;
    }

    public boolean isPlayerInRange() {
        return this.world.getClosestPlayer((double)this.x + 0.5, (double)this.y + 0.5, (double)this.z + 0.5, 16.0) != null;
    }

    public void tick() {
        this.lastRotation = this.rotation;
        if (!this.isPlayerInRange()) {
            return;
        }
        double d = (float)this.x + this.world.random.nextFloat();
        double d2 = (float)this.y + this.world.random.nextFloat();
        double d3 = (float)this.z + this.world.random.nextFloat();
        this.world.addParticle("smoke", d, d2, d3, 0.0, 0.0, 0.0);
        this.world.addParticle("flame", d, d2, d3, 0.0, 0.0, 0.0);
        this.rotation += (double)(1000.0f / ((float)this.spawnDelay + 200.0f));
        while (this.rotation > 360.0) {
            this.rotation -= 360.0;
            this.lastRotation -= 360.0;
        }
        if (!this.world.isRemote) {
            if (this.spawnDelay == -1) {
                this.resetDelay();
            }
            if (this.spawnDelay > 0) {
                --this.spawnDelay;
                return;
            }
            int n = 4;
            for (int i = 0; i < n; ++i) {
                LivingEntity livingEntity = (LivingEntity)EntityRegistry.create(this.spawnedEntityId, this.world);
                if (livingEntity == null) {
                    return;
                }
                int n2 = this.world.collectEntitiesByClass(livingEntity.getClass(), Box.createCached(this.x, this.y, this.z, this.x + 1, this.y + 1, this.z + 1).expand(8.0, 4.0, 8.0)).size();
                if (n2 >= 6) {
                    this.resetDelay();
                    return;
                }
                if (livingEntity == null) continue;
                double d4 = (double)this.x + (this.world.random.nextDouble() - this.world.random.nextDouble()) * 4.0;
                double d5 = this.y + this.world.random.nextInt(3) - 1;
                double d6 = (double)this.z + (this.world.random.nextDouble() - this.world.random.nextDouble()) * 4.0;
                livingEntity.setPositionAndAnglesKeepPrevAngles(d4, d5, d6, this.world.random.nextFloat() * 360.0f, 0.0f);
                if (!livingEntity.canSpawn()) continue;
                this.world.spawnEntity(livingEntity);
                for (int j = 0; j < 20; ++j) {
                    d = (double)this.x + 0.5 + ((double)this.world.random.nextFloat() - 0.5) * 2.0;
                    d2 = (double)this.y + 0.5 + ((double)this.world.random.nextFloat() - 0.5) * 2.0;
                    d3 = (double)this.z + 0.5 + ((double)this.world.random.nextFloat() - 0.5) * 2.0;
                    this.world.addParticle("smoke", d, d2, d3, 0.0, 0.0, 0.0);
                    this.world.addParticle("flame", d, d2, d3, 0.0, 0.0, 0.0);
                }
                livingEntity.animateSpawn();
                this.resetDelay();
            }
        }
        super.tick();
    }

    private void resetDelay() {
        this.spawnDelay = 200 + this.world.random.nextInt(600);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.spawnedEntityId = nbt.getString("EntityId");
        this.spawnDelay = nbt.getShort("Delay");
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putString("EntityId", this.spawnedEntityId);
        nbt.putShort("Delay", (short)this.spawnDelay);
    }
}

