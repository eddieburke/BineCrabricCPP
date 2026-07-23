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
import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class SkeletonEntity
extends MonsterEntity {
    private static final ItemStack heldItem = new ItemStack(Item.BOW, 1);

    public SkeletonEntity(World arg) {
        super(arg);
        this.texture = "/mob/skeleton.png";
    }

    protected String getRandomSound() {
        return "mob.skeleton";
    }

    protected String getHurtSound() {
        return "mob.skeletonhurt";
    }

    protected String getDeathSound() {
        return "mob.skeletonhurt";
    }

    public void tickMovement() {
        float f;
        if (this.world.canMonsterSpawn() && (f = this.getBrightnessAtEyes(1.0f)) > 0.5f && this.world.hasSkyLight(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)) && this.random.nextFloat() * 30.0f < (f - 0.4f) * 2.0f) {
            this.fireTicks = 300;
        }
        super.tickMovement();
    }

    protected void attack(Entity other, float distance) {
        if (distance < 10.0f) {
            double d = other.x - this.x;
            double d2 = other.z - this.z;
            if (this.attackCooldown == 0) {
                ArrowEntity arrowEntity = new ArrowEntity(this.world, this);
                arrowEntity.y += (double)1.4f;
                double d3 = other.y + (double)other.getEyeHeight() - (double)0.2f - arrowEntity.y;
                float f = MathHelper.sqrt(d * d + d2 * d2) * 0.2f;
                this.world.playSound(this, "random.bow", 1.0f, 1.0f / (this.random.nextFloat() * 0.4f + 0.8f));
                this.world.spawnEntity(arrowEntity);
                arrowEntity.setVelocity(d, d3 + (double)f, d2, 0.6f, 12.0f);
                this.attackCooldown = 30;
            }
            this.yaw = (float)(Math.atan2(d2, d) * 180.0 / 3.1415927410125732) - 90.0f;
            this.movementBlocked = true;
        }
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    protected int getDroppedItemId() {
        return Item.ARROW.id;
    }

    protected void dropItems() {
        int n;
        int n2 = this.random.nextInt(3);
        for (n = 0; n < n2; ++n) {
            this.dropItem(Item.ARROW.id, 1);
        }
        n2 = this.random.nextInt(3);
        for (n = 0; n < n2; ++n) {
            this.dropItem(Item.BONE.id, 1);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public ItemStack getHeldItem() {
        return heldItem;
    }
}

