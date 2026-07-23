/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.mob;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.entity.mob.SkeletonEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class CreeperEntity
extends MonsterEntity {
    int fuseTime;
    int lastFuseTime;

    public CreeperEntity(World arg) {
        super(arg);
        this.texture = "/mob/creeper.png";
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, (byte)-1);
        this.dataTracker.startTracking(17, (byte)0);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        if (this.dataTracker.getByte(17) == 1) {
            nbt.putBoolean("powered", true);
        }
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.dataTracker.set(17, (byte)(nbt.getBoolean("powered") ? 1 : 0));
    }

    protected void resetAttack(Entity other, float distance) {
        if (this.world.isRemote) {
            return;
        }
        if (this.fuseTime > 0) {
            this.setFuseSpeed(-1);
            --this.fuseTime;
            if (this.fuseTime < 0) {
                this.fuseTime = 0;
            }
        }
    }

    public void tick() {
        this.lastFuseTime = this.fuseTime;
        if (this.world.isRemote) {
            int n = this.getFuseSpeed();
            if (n > 0 && this.fuseTime == 0) {
                this.world.playSound(this, "random.fuse", 1.0f, 0.5f);
            }
            this.fuseTime += n;
            if (this.fuseTime < 0) {
                this.fuseTime = 0;
            }
            if (this.fuseTime >= 30) {
                this.fuseTime = 30;
            }
        }
        super.tick();
        if (this.target == null && this.fuseTime > 0) {
            this.setFuseSpeed(-1);
            --this.fuseTime;
            if (this.fuseTime < 0) {
                this.fuseTime = 0;
            }
        }
    }

    protected String getHurtSound() {
        return "mob.creeper";
    }

    protected String getDeathSound() {
        return "mob.creeperdeath";
    }

    public void onKilledBy(Entity adversary) {
        super.onKilledBy(adversary);
        if (adversary instanceof SkeletonEntity) {
            this.dropItem(Item.RECORD_THIRTEEN.id + this.random.nextInt(2), 1);
        }
    }

    protected void attack(Entity other, float distance) {
        if (this.world.isRemote) {
            return;
        }
        int n = this.getFuseSpeed();
        if (n <= 0 && distance < 3.0f || n > 0 && distance < 7.0f) {
            if (this.fuseTime == 0) {
                this.world.playSound(this, "random.fuse", 1.0f, 0.5f);
            }
            this.setFuseSpeed(1);
            ++this.fuseTime;
            if (this.fuseTime >= 30) {
                if (this.isCharged()) {
                    this.world.createExplosion(this, this.x, this.y, this.z, 6.0f);
                } else {
                    this.world.createExplosion(this, this.x, this.y, this.z, 3.0f);
                }
                this.markDead();
            }
            this.movementBlocked = true;
        } else {
            this.setFuseSpeed(-1);
            --this.fuseTime;
            if (this.fuseTime < 0) {
                this.fuseTime = 0;
            }
        }
    }

    public boolean isCharged() {
        return this.dataTracker.getByte(17) == 1;
    }

    @Environment(value=EnvType.CLIENT)
    public float getScale(float delta) {
        return ((float)this.lastFuseTime + (float)(this.fuseTime - this.lastFuseTime) * delta) / 28.0f;
    }

    protected int getDroppedItemId() {
        return Item.GUNPOWDER.id;
    }

    private int getFuseSpeed() {
        return this.dataTracker.getByte(16);
    }

    private void setFuseSpeed(int fuseSpeed) {
        this.dataTracker.set(16, (byte)fuseSpeed);
    }

    public void onStruckByLightning(LightningEntity lightning) {
        super.onStruckByLightning(lightning);
        this.dataTracker.set(17, (byte)1);
    }
}

