/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.passive;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class SheepEntity
extends AnimalEntity {
    public static final float[][] COLORS = new float[][]{{1.0f, 1.0f, 1.0f}, {0.95f, 0.7f, 0.2f}, {0.9f, 0.5f, 0.85f}, {0.6f, 0.7f, 0.95f}, {0.9f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f}, {0.95f, 0.7f, 0.8f}, {0.3f, 0.3f, 0.3f}, {0.6f, 0.6f, 0.6f}, {0.3f, 0.6f, 0.7f}, {0.7f, 0.4f, 0.9f}, {0.2f, 0.4f, 0.8f}, {0.5f, 0.4f, 0.3f}, {0.4f, 0.5f, 0.2f}, {0.8f, 0.3f, 0.3f}, {0.1f, 0.1f, 0.1f}};

    public SheepEntity(World arg) {
        super(arg);
        this.texture = "/mob/sheep.png";
        this.setBoundingBoxSpacing(0.9f, 1.3f);
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, new Byte(0));
    }

    public boolean damage(Entity damageSource, int amount) {
        return super.damage(damageSource, amount);
    }

    protected void dropItems() {
        if (!this.isSheared()) {
            this.dropItem(new ItemStack(Block.WOOL.id, 1, this.getColor()), 0.0f);
        }
    }

    protected int getDroppedItemId() {
        return Block.WOOL.id;
    }

    public boolean interact(PlayerEntity player) {
        ItemStack itemStack = player.inventory.getSelectedItem();
        if (itemStack != null && itemStack.itemId == Item.SHEARS.id && !this.isSheared()) {
            if (!this.world.isRemote) {
                this.setSheared(true);
                int n = 2 + this.random.nextInt(3);
                for (int i = 0; i < n; ++i) {
                    ItemEntity itemEntity = this.dropItem(new ItemStack(Block.WOOL.id, 1, this.getColor()), 1.0f);
                    itemEntity.velocityY += (double)(this.random.nextFloat() * 0.05f);
                    itemEntity.velocityX += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.1f);
                    itemEntity.velocityZ += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.1f);
                }
            }
            itemStack.damage(1, player);
        }
        return false;
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putBoolean("Sheared", this.isSheared());
        nbt.putByte("Color", (byte)this.getColor());
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.setSheared(nbt.getBoolean("Sheared"));
        this.setColor(nbt.getByte("Color"));
    }

    protected String getRandomSound() {
        return "mob.sheep";
    }

    protected String getHurtSound() {
        return "mob.sheep";
    }

    protected String getDeathSound() {
        return "mob.sheep";
    }

    public int getColor() {
        return this.dataTracker.getByte(16) & 0xF;
    }

    public void setColor(int color) {
        byte by = this.dataTracker.getByte(16);
        this.dataTracker.set(16, (byte)(by & 0xF0 | color & 0xF));
    }

    public boolean isSheared() {
        return (this.dataTracker.getByte(16) & 0x10) != 0;
    }

    public void setSheared(boolean sheared) {
        byte by = this.dataTracker.getByte(16);
        if (sheared) {
            this.dataTracker.set(16, (byte)(by | 0x10));
        } else {
            this.dataTracker.set(16, (byte)(by & 0xFFFFFFEF));
        }
    }

    public static int generateDefaultColor(Random random) {
        int n = random.nextInt(100);
        if (n < 5) {
            return 15;
        }
        if (n < 10) {
            return 7;
        }
        if (n < 15) {
            return 8;
        }
        if (n < 18) {
            return 12;
        }
        if (random.nextInt(500) == 0) {
            return 6;
        }
        return 0;
    }
}

