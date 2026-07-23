/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity;

import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class ItemEntity
extends Entity {
    public ItemStack stack;
    private int itemTicks;
    public int itemAge = 0;
    public int pickupDelay;
    private int health = 5;
    public float initialRotationAngle = (float)(Math.random() * Math.PI * 2.0);

    public ItemEntity(World world, double x, double y, double z, ItemStack stack) {
        super(world);
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.standingEyeHeight = this.height / 2.0f;
        this.setPosition(x, y, z);
        this.stack = stack;
        this.yaw = (float)(Math.random() * 360.0);
        this.velocityX = (float)(Math.random() * (double)0.2f - (double)0.1f);
        this.velocityY = 0.2f;
        this.velocityZ = (float)(Math.random() * (double)0.2f - (double)0.1f);
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    public ItemEntity(World arg) {
        super(arg);
        this.setBoundingBoxSpacing(0.25f, 0.25f);
        this.standingEyeHeight = this.height / 2.0f;
    }

    protected void initDataTracker() {
    }

    public void tick() {
        super.tick();
        if (this.pickupDelay > 0) {
            --this.pickupDelay;
        }
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.velocityY -= (double)0.04f;
        if (this.world.getMaterial(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)) == Material.LAVA) {
            this.velocityY = 0.2f;
            this.velocityX = (this.random.nextFloat() - this.random.nextFloat()) * 0.2f;
            this.velocityZ = (this.random.nextFloat() - this.random.nextFloat()) * 0.2f;
            this.world.playSound(this, "random.fizz", 0.4f, 2.0f + this.random.nextFloat() * 0.4f);
        }
        this.pushOutOfBlock(this.x, (this.boundingBox.minY + this.boundingBox.maxY) / 2.0, this.z);
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        float f = 0.98f;
        if (this.onGround) {
            f = 0.58800006f;
            int n = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.boundingBox.minY) - 1, MathHelper.floor(this.z));
            if (n > 0) {
                f = Block.BLOCKS[n].slipperiness * 0.98f;
            }
        }
        this.velocityX *= (double)f;
        this.velocityY *= (double)0.98f;
        this.velocityZ *= (double)f;
        if (this.onGround) {
            this.velocityY *= -0.5;
        }
        ++this.itemTicks;
        ++this.itemAge;
        if (this.itemAge >= 6000) {
            this.markDead();
        }
    }

    public boolean checkWaterCollisions() {
        return this.world.updateMovementInFluid(this.boundingBox, Material.WATER, this);
    }

    protected void damage(int amount) {
        this.damage(null, amount);
    }

    public boolean damage(Entity damageSource, int amount) {
        this.scheduleVelocityUpdate();
        this.health -= amount;
        if (this.health <= 0) {
            this.markDead();
        }
        return false;
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putShort("Health", (byte)this.health);
        nbt.putShort("Age", (short)this.itemAge);
        nbt.put("Item", this.stack.writeNbt(new NbtCompound()));
    }

    public void readNbt(NbtCompound nbt) {
        this.health = nbt.getShort("Health") & 0xFF;
        this.itemAge = nbt.getShort("Age");
        NbtCompound nbtCompound = nbt.getCompound("Item");
        this.stack = new ItemStack(nbtCompound);
    }

    public void onPlayerInteraction(PlayerEntity player) {
        if (this.world.isRemote) {
            return;
        }
        int n = this.stack.count;
        if (this.pickupDelay == 0 && player.inventory.addStack(this.stack)) {
            if (this.stack.itemId == Block.LOG.id) {
                player.incrementStat(Achievements.MINE_WOOD);
            }
            if (this.stack.itemId == Item.LEATHER.id) {
                player.incrementStat(Achievements.KILL_COW);
            }
            this.world.playSound(this, "random.pop", 0.2f, ((this.random.nextFloat() - this.random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
            player.sendPickup(this, n);
            if (this.stack.count <= 0) {
                this.markDead();
            }
        }
    }
}

