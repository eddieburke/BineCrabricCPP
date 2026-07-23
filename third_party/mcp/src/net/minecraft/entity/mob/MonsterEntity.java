/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.Entity;
import net.minecraft.entity.MobEntity;
import net.minecraft.entity.Monster;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.LightType;
import net.minecraft.world.World;

public class MonsterEntity
extends MobEntity
implements Monster {
    protected int attackDamage = 2;

    public MonsterEntity(World arg) {
        super(arg);
        this.health = 20;
    }

    public void tickMovement() {
        float f = this.getBrightnessAtEyes(1.0f);
        if (f > 0.5f) {
            this.despawnCounter += 2;
        }
        super.tickMovement();
    }

    public void tick() {
        super.tick();
        if (!this.world.isRemote && this.world.difficulty == 0) {
            this.markDead();
        }
    }

    protected Entity getTargetInRange() {
        PlayerEntity playerEntity = this.world.getClosestPlayer(this, 16.0);
        if (playerEntity != null && this.canSee(playerEntity)) {
            return playerEntity;
        }
        return null;
    }

    public boolean damage(Entity damageSource, int amount) {
        if (super.damage(damageSource, amount)) {
            if (this.passenger == damageSource || this.vehicle == damageSource) {
                return true;
            }
            if (damageSource != this) {
                this.target = damageSource;
            }
            return true;
        }
        return false;
    }

    protected void attack(Entity other, float distance) {
        if (this.attackCooldown <= 0 && distance < 2.0f && other.boundingBox.maxY > this.boundingBox.minY && other.boundingBox.minY < this.boundingBox.maxY) {
            this.attackCooldown = 20;
            other.damage(this, this.attackDamage);
        }
    }

    protected float getPathfindingFavor(int x, int y, int z) {
        return 0.5f - this.world.getLightBrightness(x, y, z);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    public boolean canSpawn() {
        int n;
        int n2;
        int n3 = MathHelper.floor(this.x);
        if (this.world.getBrightness(LightType.SKY, n3, n2 = MathHelper.floor(this.boundingBox.minY), n = MathHelper.floor(this.z)) > this.random.nextInt(32)) {
            return false;
        }
        int n4 = this.world.getLightLevel(n3, n2, n);
        if (this.world.isThundering()) {
            int n5 = this.world.ambientDarkness;
            this.world.ambientDarkness = 10;
            n4 = this.world.getLightLevel(n3, n2, n);
            this.world.ambientDarkness = n5;
        }
        return n4 <= this.random.nextInt(8) && super.canSpawn();
    }
}

