/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.passive;

import net.minecraft.achievement.Achievements;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.mob.PigZombieEntity;
import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class PigEntity
extends AnimalEntity {
    public PigEntity(World arg) {
        super(arg);
        this.texture = "/mob/pig.png";
        this.setBoundingBoxSpacing(0.9f, 0.9f);
    }

    protected void initDataTracker() {
        this.dataTracker.startTracking(16, (byte)0);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putBoolean("Saddle", this.isSaddled());
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.setSaddled(nbt.getBoolean("Saddle"));
    }

    protected String getRandomSound() {
        return "mob.pig";
    }

    protected String getHurtSound() {
        return "mob.pig";
    }

    protected String getDeathSound() {
        return "mob.pigdeath";
    }

    public boolean interact(PlayerEntity player) {
        if (this.isSaddled() && !this.world.isRemote && (this.passenger == null || this.passenger == player)) {
            player.setVehicle(this);
            return true;
        }
        return false;
    }

    protected int getDroppedItemId() {
        if (this.fireTicks > 0) {
            return Item.COOKED_PORKCHOP.id;
        }
        return Item.RAW_PORKCHOP.id;
    }

    public boolean isSaddled() {
        return (this.dataTracker.getByte(16) & 1) != 0;
    }

    public void setSaddled(boolean saddled) {
        if (saddled) {
            this.dataTracker.set(16, (byte)1);
        } else {
            this.dataTracker.set(16, (byte)0);
        }
    }

    public void onStruckByLightning(LightningEntity lightning) {
        if (this.world.isRemote) {
            return;
        }
        PigZombieEntity pigZombieEntity = new PigZombieEntity(this.world);
        pigZombieEntity.setPositionAndAnglesKeepPrevAngles(this.x, this.y, this.z, this.yaw, this.pitch);
        this.world.spawnEntity(pigZombieEntity);
        this.markDead();
    }

    protected void onLanding(float fallDistance) {
        super.onLanding(fallDistance);
        if (fallDistance > 5.0f && this.passenger instanceof PlayerEntity) {
            ((PlayerEntity)this.passenger).incrementStat(Achievements.FLY_PIG);
        }
    }
}

