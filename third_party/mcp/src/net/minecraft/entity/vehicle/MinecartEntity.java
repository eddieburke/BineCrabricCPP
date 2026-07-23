/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.vehicle;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.RailBlock;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtList;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class MinecartEntity
extends Entity
implements Inventory {
    private ItemStack[] inventory = new ItemStack[36];
    public int damageWobbleStrength = 0;
    public int damageWobbleTicks = 0;
    public int damageWobbleSide = 1;
    private boolean yawFlipped = false;
    public int type;
    public int fuel;
    public double pushX;
    public double pushZ;
    private static final int[][][] ADJACENT_RAIL_POSITIONS_BY_SHAPE = new int[][][]{new int[][]{{0, 0, -1}, {0, 0, 1}}, new int[][]{{-1, 0, 0}, {1, 0, 0}}, new int[][]{{-1, -1, 0}, {1, 0, 0}}, new int[][]{{-1, 0, 0}, {1, -1, 0}}, new int[][]{{0, 0, -1}, {0, -1, 1}}, new int[][]{{0, -1, -1}, {0, 0, 1}}, new int[][]{{0, 0, 1}, {1, 0, 0}}, new int[][]{{0, 0, 1}, {-1, 0, 0}}, new int[][]{{0, 0, -1}, {-1, 0, 0}}, new int[][]{{0, 0, -1}, {1, 0, 0}}};
    private int clientInterpolationSteps;
    private double clientX;
    private double clientY;
    private double clientZ;
    private double clientPitch;
    private double clientYaw;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityX;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityY;
    @Environment(value=EnvType.CLIENT)
    private double clientVelocityZ;

    public MinecartEntity(World arg) {
        super(arg);
        this.blocksSameBlockSpawning = true;
        this.setBoundingBoxSpacing(0.98f, 0.7f);
        this.standingEyeHeight = this.height / 2.0f;
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    protected void initDataTracker() {
    }

    public Box getCollisionAgainstShape(Entity other) {
        return other.boundingBox;
    }

    public Box getBoundingBox() {
        return null;
    }

    public boolean isPushable() {
        return true;
    }

    public MinecartEntity(World world, double x, double y, double z, int type) {
        this(world);
        this.setPosition(x, y + (double)this.standingEyeHeight, z);
        this.velocityX = 0.0;
        this.velocityY = 0.0;
        this.velocityZ = 0.0;
        this.prevX = x;
        this.prevY = y;
        this.prevZ = z;
        this.type = type;
    }

    public double getPassengerRidingHeight() {
        return (double)this.height * 0.0 - (double)0.3f;
    }

    public boolean damage(Entity damageSource, int amount) {
        if (this.world.isRemote || this.dead) {
            return true;
        }
        this.damageWobbleSide = -this.damageWobbleSide;
        this.damageWobbleTicks = 10;
        this.scheduleVelocityUpdate();
        this.damageWobbleStrength += amount * 10;
        if (this.damageWobbleStrength > 40) {
            if (this.passenger != null) {
                this.passenger.setVehicle(this);
            }
            this.markDead();
            this.dropItem(Item.MINECART.id, 1, 0.0f);
            if (this.type == 1) {
                MinecartEntity minecartEntity = this;
                for (int i = 0; i < minecartEntity.size(); ++i) {
                    ItemStack itemStack = minecartEntity.getStack(i);
                    if (itemStack == null) continue;
                    float f = this.random.nextFloat() * 0.8f + 0.1f;
                    float f2 = this.random.nextFloat() * 0.8f + 0.1f;
                    float f3 = this.random.nextFloat() * 0.8f + 0.1f;
                    while (itemStack.count > 0) {
                        int n = this.random.nextInt(21) + 10;
                        if (n > itemStack.count) {
                            n = itemStack.count;
                        }
                        itemStack.count -= n;
                        ItemEntity itemEntity = new ItemEntity(this.world, this.x + (double)f, this.y + (double)f2, this.z + (double)f3, new ItemStack(itemStack.itemId, n, itemStack.getDamage()));
                        float f4 = 0.05f;
                        itemEntity.velocityX = (float)this.random.nextGaussian() * f4;
                        itemEntity.velocityY = (float)this.random.nextGaussian() * f4 + 0.2f;
                        itemEntity.velocityZ = (float)this.random.nextGaussian() * f4;
                        this.world.spawnEntity(itemEntity);
                    }
                }
                this.dropItem(Block.CHEST.id, 1, 0.0f);
            } else if (this.type == 2) {
                this.dropItem(Block.FURNACE.id, 1, 0.0f);
            }
        }
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public void animateHurt() {
        System.out.println("Animating hurt");
        this.damageWobbleSide = -this.damageWobbleSide;
        this.damageWobbleTicks = 10;
        this.damageWobbleStrength += this.damageWobbleStrength * 10;
    }

    public boolean isCollidable() {
        return !this.dead;
    }

    public void markDead() {
        for (int i = 0; i < this.size(); ++i) {
            ItemStack itemStack = this.getStack(i);
            if (itemStack == null) continue;
            float f = this.random.nextFloat() * 0.8f + 0.1f;
            float f2 = this.random.nextFloat() * 0.8f + 0.1f;
            float f3 = this.random.nextFloat() * 0.8f + 0.1f;
            while (itemStack.count > 0) {
                int n = this.random.nextInt(21) + 10;
                if (n > itemStack.count) {
                    n = itemStack.count;
                }
                itemStack.count -= n;
                ItemEntity itemEntity = new ItemEntity(this.world, this.x + (double)f, this.y + (double)f2, this.z + (double)f3, new ItemStack(itemStack.itemId, n, itemStack.getDamage()));
                float f4 = 0.05f;
                itemEntity.velocityX = (float)this.random.nextGaussian() * f4;
                itemEntity.velocityY = (float)this.random.nextGaussian() * f4 + 0.2f;
                itemEntity.velocityZ = (float)this.random.nextGaussian() * f4;
                this.world.spawnEntity(itemEntity);
            }
        }
        super.markDead();
    }

    public void tick() {
        double d;
        int n;
        int n2;
        if (this.damageWobbleTicks > 0) {
            --this.damageWobbleTicks;
        }
        if (this.damageWobbleStrength > 0) {
            --this.damageWobbleStrength;
        }
        if (this.world.isRemote && this.clientInterpolationSteps > 0) {
            if (this.clientInterpolationSteps > 0) {
                double d2;
                double d3 = this.x + (this.clientX - this.x) / (double)this.clientInterpolationSteps;
                double d4 = this.y + (this.clientY - this.y) / (double)this.clientInterpolationSteps;
                double d5 = this.z + (this.clientZ - this.z) / (double)this.clientInterpolationSteps;
                for (d2 = this.clientPitch - (double)this.yaw; d2 < -180.0; d2 += 360.0) {
                }
                while (d2 >= 180.0) {
                    d2 -= 360.0;
                }
                this.yaw = (float)((double)this.yaw + d2 / (double)this.clientInterpolationSteps);
                this.pitch = (float)((double)this.pitch + (this.clientYaw - (double)this.pitch) / (double)this.clientInterpolationSteps);
                --this.clientInterpolationSteps;
                this.setPosition(d3, d4, d5);
                this.setRotation(this.yaw, this.pitch);
            } else {
                this.setPosition(this.x, this.y, this.z);
                this.setRotation(this.yaw, this.pitch);
            }
            return;
        }
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.velocityY -= (double)0.04f;
        int n3 = MathHelper.floor(this.x);
        if (RailBlock.isRail(this.world, n3, (n2 = MathHelper.floor(this.y)) - 1, n = MathHelper.floor(this.z))) {
            --n2;
        }
        double d6 = 0.4;
        boolean bl = false;
        double d7 = 0.0078125;
        int n4 = this.world.getBlockId(n3, n2, n);
        if (RailBlock.isRail(n4)) {
            double d8;
            double d9;
            double d10;
            double d11;
            double d12;
            Vec3d vec3d = this.snapPositionToRail(this.x, this.y, this.z);
            int n5 = this.world.getBlockMeta(n3, n2, n);
            this.y = n2;
            boolean bl2 = false;
            boolean bl3 = false;
            if (n4 == Block.POWERED_RAIL.id) {
                bl2 = (n5 & 8) != 0;
                boolean bl4 = bl3 = !bl2;
            }
            if (((RailBlock)Block.BLOCKS[n4]).isAlwaysStraight()) {
                n5 &= 7;
            }
            if (n5 >= 2 && n5 <= 5) {
                this.y = n2 + 1;
            }
            if (n5 == 2) {
                this.velocityX -= d7;
            }
            if (n5 == 3) {
                this.velocityX += d7;
            }
            if (n5 == 4) {
                this.velocityZ += d7;
            }
            if (n5 == 5) {
                this.velocityZ -= d7;
            }
            int[][] nArray = ADJACENT_RAIL_POSITIONS_BY_SHAPE[n5];
            double d13 = nArray[1][0] - nArray[0][0];
            double d14 = nArray[1][2] - nArray[0][2];
            double d15 = Math.sqrt(d13 * d13 + d14 * d14);
            double d16 = this.velocityX * d13 + this.velocityZ * d14;
            if (d16 < 0.0) {
                d13 = -d13;
                d14 = -d14;
            }
            double d17 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
            this.velocityX = d17 * d13 / d15;
            this.velocityZ = d17 * d14 / d15;
            if (bl3) {
                d12 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
                if (d12 < 0.03) {
                    this.velocityX *= 0.0;
                    this.velocityY *= 0.0;
                    this.velocityZ *= 0.0;
                } else {
                    this.velocityX *= 0.5;
                    this.velocityY *= 0.0;
                    this.velocityZ *= 0.5;
                }
            }
            d12 = 0.0;
            double d18 = (double)n3 + 0.5 + (double)nArray[0][0] * 0.5;
            double d19 = (double)n + 0.5 + (double)nArray[0][2] * 0.5;
            double d20 = (double)n3 + 0.5 + (double)nArray[1][0] * 0.5;
            double d21 = (double)n + 0.5 + (double)nArray[1][2] * 0.5;
            d13 = d20 - d18;
            d14 = d21 - d19;
            if (d13 == 0.0) {
                this.x = (double)n3 + 0.5;
                d12 = this.z - (double)n;
            } else if (d14 == 0.0) {
                this.z = (double)n + 0.5;
                d12 = this.x - (double)n3;
            } else {
                d11 = this.x - d18;
                d10 = this.z - d19;
                d12 = d9 = (d11 * d13 + d10 * d14) * 2.0;
            }
            this.x = d18 + d13 * d12;
            this.z = d19 + d14 * d12;
            this.setPosition(this.x, this.y + (double)this.standingEyeHeight, this.z);
            d11 = this.velocityX;
            d10 = this.velocityZ;
            if (this.passenger != null) {
                d11 *= 0.75;
                d10 *= 0.75;
            }
            if (d11 < -d6) {
                d11 = -d6;
            }
            if (d11 > d6) {
                d11 = d6;
            }
            if (d10 < -d6) {
                d10 = -d6;
            }
            if (d10 > d6) {
                d10 = d6;
            }
            this.move(d11, 0.0, d10);
            if (nArray[0][1] != 0 && MathHelper.floor(this.x) - n3 == nArray[0][0] && MathHelper.floor(this.z) - n == nArray[0][2]) {
                this.setPosition(this.x, this.y + (double)nArray[0][1], this.z);
            } else if (nArray[1][1] != 0 && MathHelper.floor(this.x) - n3 == nArray[1][0] && MathHelper.floor(this.z) - n == nArray[1][2]) {
                this.setPosition(this.x, this.y + (double)nArray[1][1], this.z);
            }
            if (this.passenger != null) {
                this.velocityX *= (double)0.997f;
                this.velocityY *= 0.0;
                this.velocityZ *= (double)0.997f;
            } else {
                if (this.type == 2) {
                    d9 = MathHelper.sqrt(this.pushX * this.pushX + this.pushZ * this.pushZ);
                    if (d9 > 0.01) {
                        bl = true;
                        this.pushX /= d9;
                        this.pushZ /= d9;
                        double d22 = 0.04;
                        this.velocityX *= (double)0.8f;
                        this.velocityY *= 0.0;
                        this.velocityZ *= (double)0.8f;
                        this.velocityX += this.pushX * d22;
                        this.velocityZ += this.pushZ * d22;
                    } else {
                        this.velocityX *= (double)0.9f;
                        this.velocityY *= 0.0;
                        this.velocityZ *= (double)0.9f;
                    }
                }
                this.velocityX *= (double)0.96f;
                this.velocityY *= 0.0;
                this.velocityZ *= (double)0.96f;
            }
            Vec3d vec3d2 = this.snapPositionToRail(this.x, this.y, this.z);
            if (vec3d2 != null && vec3d != null) {
                double d23 = (vec3d.y - vec3d2.y) * 0.05;
                d17 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
                if (d17 > 0.0) {
                    this.velocityX = this.velocityX / d17 * (d17 + d23);
                    this.velocityZ = this.velocityZ / d17 * (d17 + d23);
                }
                this.setPosition(this.x, vec3d2.y, this.z);
            }
            int n6 = MathHelper.floor(this.x);
            int n7 = MathHelper.floor(this.z);
            if (n6 != n3 || n7 != n) {
                d17 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
                this.velocityX = d17 * (double)(n6 - n3);
                this.velocityZ = d17 * (double)(n7 - n);
            }
            if (this.type == 2 && (d8 = (double)MathHelper.sqrt(this.pushX * this.pushX + this.pushZ * this.pushZ)) > 0.01 && this.velocityX * this.velocityX + this.velocityZ * this.velocityZ > 0.001) {
                this.pushX /= d8;
                this.pushZ /= d8;
                if (this.pushX * this.velocityX + this.pushZ * this.velocityZ < 0.0) {
                    this.pushX = 0.0;
                    this.pushZ = 0.0;
                } else {
                    this.pushX = this.velocityX;
                    this.pushZ = this.velocityZ;
                }
            }
            if (bl2) {
                d8 = Math.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
                if (d8 > 0.01) {
                    double d24 = 0.06;
                    this.velocityX += this.velocityX / d8 * d24;
                    this.velocityZ += this.velocityZ / d8 * d24;
                } else if (n5 == 1) {
                    if (this.world.shouldSuffocate(n3 - 1, n2, n)) {
                        this.velocityX = 0.02;
                    } else if (this.world.shouldSuffocate(n3 + 1, n2, n)) {
                        this.velocityX = -0.02;
                    }
                } else if (n5 == 0) {
                    if (this.world.shouldSuffocate(n3, n2, n - 1)) {
                        this.velocityZ = 0.02;
                    } else if (this.world.shouldSuffocate(n3, n2, n + 1)) {
                        this.velocityZ = -0.02;
                    }
                }
            }
        } else {
            if (this.velocityX < -d6) {
                this.velocityX = -d6;
            }
            if (this.velocityX > d6) {
                this.velocityX = d6;
            }
            if (this.velocityZ < -d6) {
                this.velocityZ = -d6;
            }
            if (this.velocityZ > d6) {
                this.velocityZ = d6;
            }
            if (this.onGround) {
                this.velocityX *= 0.5;
                this.velocityY *= 0.5;
                this.velocityZ *= 0.5;
            }
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            if (!this.onGround) {
                this.velocityX *= (double)0.95f;
                this.velocityY *= (double)0.95f;
                this.velocityZ *= (double)0.95f;
            }
        }
        this.pitch = 0.0f;
        double d25 = this.prevX - this.x;
        double d26 = this.prevZ - this.z;
        if (d25 * d25 + d26 * d26 > 0.001) {
            this.yaw = (float)(Math.atan2(d26, d25) * 180.0 / Math.PI);
            if (this.yawFlipped) {
                this.yaw += 180.0f;
            }
        }
        for (d = (double)(this.yaw - this.prevYaw); d >= 180.0; d -= 360.0) {
        }
        while (d < -180.0) {
            d += 360.0;
        }
        if (d < -170.0 || d >= 170.0) {
            this.yaw += 180.0f;
            this.yawFlipped = !this.yawFlipped;
        }
        this.setRotation(this.yaw, this.pitch);
        List list = this.world.getEntities(this, this.boundingBox.expand(0.2f, 0.0, 0.2f));
        if (list != null && list.size() > 0) {
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                if (entity == this.passenger || !entity.isPushable() || !(entity instanceof MinecartEntity)) continue;
                entity.onCollision(this);
            }
        }
        if (this.passenger != null && this.passenger.dead) {
            this.passenger = null;
        }
        if (bl && this.random.nextInt(4) == 0) {
            --this.fuel;
            if (this.fuel < 0) {
                this.pushZ = 0.0;
                this.pushX = 0.0;
            }
            this.world.addParticle("largesmoke", this.x, this.y + 0.8, this.z, 0.0, 0.0, 0.0);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d snapPositionToRailWithOffset(double x, double y, double z, double offset) {
        int n;
        int n2;
        int n3;
        int n4 = MathHelper.floor(x);
        if (RailBlock.isRail(this.world, n4, (n3 = MathHelper.floor(y)) - 1, n2 = MathHelper.floor(z))) {
            --n3;
        }
        if (RailBlock.isRail(n = this.world.getBlockId(n4, n3, n2))) {
            int n5 = this.world.getBlockMeta(n4, n3, n2);
            if (((RailBlock)Block.BLOCKS[n]).isAlwaysStraight()) {
                n5 &= 7;
            }
            y = n3;
            if (n5 >= 2 && n5 <= 5) {
                y = n3 + 1;
            }
            int[][] nArray = ADJACENT_RAIL_POSITIONS_BY_SHAPE[n5];
            double d = nArray[1][0] - nArray[0][0];
            double d2 = nArray[1][2] - nArray[0][2];
            double d3 = Math.sqrt(d * d + d2 * d2);
            if (nArray[0][1] != 0 && MathHelper.floor(x += (d /= d3) * offset) - n4 == nArray[0][0] && MathHelper.floor(z += (d2 /= d3) * offset) - n2 == nArray[0][2]) {
                y += (double)nArray[0][1];
            } else if (nArray[1][1] != 0 && MathHelper.floor(x) - n4 == nArray[1][0] && MathHelper.floor(z) - n2 == nArray[1][2]) {
                y += (double)nArray[1][1];
            }
            return this.snapPositionToRail(x, y, z);
        }
        return null;
    }

    public Vec3d snapPositionToRail(double x, double y, double z) {
        int n;
        int n2;
        int n3;
        int n4 = MathHelper.floor(x);
        if (RailBlock.isRail(this.world, n4, (n3 = MathHelper.floor(y)) - 1, n2 = MathHelper.floor(z))) {
            --n3;
        }
        if (RailBlock.isRail(n = this.world.getBlockId(n4, n3, n2))) {
            int n5 = this.world.getBlockMeta(n4, n3, n2);
            y = n3;
            if (((RailBlock)Block.BLOCKS[n]).isAlwaysStraight()) {
                n5 &= 7;
            }
            if (n5 >= 2 && n5 <= 5) {
                y = n3 + 1;
            }
            int[][] nArray = ADJACENT_RAIL_POSITIONS_BY_SHAPE[n5];
            double d = 0.0;
            double d2 = (double)n4 + 0.5 + (double)nArray[0][0] * 0.5;
            double d3 = (double)n3 + 0.5 + (double)nArray[0][1] * 0.5;
            double d4 = (double)n2 + 0.5 + (double)nArray[0][2] * 0.5;
            double d5 = (double)n4 + 0.5 + (double)nArray[1][0] * 0.5;
            double d6 = (double)n3 + 0.5 + (double)nArray[1][1] * 0.5;
            double d7 = (double)n2 + 0.5 + (double)nArray[1][2] * 0.5;
            double d8 = d5 - d2;
            double d9 = (d6 - d3) * 2.0;
            double d10 = d7 - d4;
            if (d8 == 0.0) {
                x = (double)n4 + 0.5;
                d = z - (double)n2;
            } else if (d10 == 0.0) {
                z = (double)n2 + 0.5;
                d = x - (double)n4;
            } else {
                double d11;
                double d12 = x - d2;
                double d13 = z - d4;
                d = d11 = (d12 * d8 + d13 * d10) * 2.0;
            }
            x = d2 + d8 * d;
            y = d3 + d9 * d;
            z = d4 + d10 * d;
            if (d9 < 0.0) {
                y += 1.0;
            }
            if (d9 > 0.0) {
                y += 0.5;
            }
            return Vec3d.createCached(x, y, z);
        }
        return null;
    }

    protected void writeNbt(NbtCompound nbt) {
        nbt.putInt("Type", this.type);
        if (this.type == 2) {
            nbt.putDouble("PushX", this.pushX);
            nbt.putDouble("PushZ", this.pushZ);
            nbt.putShort("Fuel", (short)this.fuel);
        } else if (this.type == 1) {
            NbtList nbtList = new NbtList();
            for (int i = 0; i < this.inventory.length; ++i) {
                if (this.inventory[i] == null) continue;
                NbtCompound nbtCompound = new NbtCompound();
                nbtCompound.putByte("Slot", (byte)i);
                this.inventory[i].writeNbt(nbtCompound);
                nbtList.add(nbtCompound);
            }
            nbt.put("Items", nbtList);
        }
    }

    protected void readNbt(NbtCompound nbt) {
        this.type = nbt.getInt("Type");
        if (this.type == 2) {
            this.pushX = nbt.getDouble("PushX");
            this.pushZ = nbt.getDouble("PushZ");
            this.fuel = nbt.getShort("Fuel");
        } else if (this.type == 1) {
            NbtList nbtList = nbt.getList("Items");
            this.inventory = new ItemStack[this.size()];
            for (int i = 0; i < nbtList.size(); ++i) {
                NbtCompound nbtCompound = (NbtCompound)nbtList.get(i);
                int n = nbtCompound.getByte("Slot") & 0xFF;
                if (n < 0 || n >= this.inventory.length) continue;
                this.inventory[n] = new ItemStack(nbtCompound);
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }

    public void onCollision(Entity otherEntity) {
        double d;
        double d2;
        double d3;
        if (this.world.isRemote) {
            return;
        }
        if (otherEntity == this.passenger) {
            return;
        }
        if (otherEntity instanceof LivingEntity && !(otherEntity instanceof PlayerEntity) && this.type == 0 && this.velocityX * this.velocityX + this.velocityZ * this.velocityZ > 0.01 && this.passenger == null && otherEntity.vehicle == null) {
            otherEntity.setVehicle(this);
        }
        if ((d3 = (d2 = otherEntity.x - this.x) * d2 + (d = otherEntity.z - this.z) * d) >= (double)1.0E-4f) {
            d3 = MathHelper.sqrt(d3);
            d2 /= d3;
            d /= d3;
            double d4 = 1.0 / d3;
            if (d4 > 1.0) {
                d4 = 1.0;
            }
            d2 *= d4;
            d *= d4;
            d2 *= (double)0.1f;
            d *= (double)0.1f;
            d2 *= (double)(1.0f - this.pushSpeedReduction);
            d *= (double)(1.0f - this.pushSpeedReduction);
            d2 *= 0.5;
            d *= 0.5;
            if (otherEntity instanceof MinecartEntity) {
                double d5 = otherEntity.x - this.x;
                double d6 = otherEntity.z - this.z;
                double d7 = d5 * otherEntity.velocityZ + d6 * otherEntity.prevX;
                if ((d7 *= d7) > 5.0) {
                    return;
                }
                double d8 = otherEntity.velocityX + this.velocityX;
                double d9 = otherEntity.velocityZ + this.velocityZ;
                if (((MinecartEntity)otherEntity).type == 2 && this.type != 2) {
                    this.velocityX *= (double)0.2f;
                    this.velocityZ *= (double)0.2f;
                    this.addVelocity(otherEntity.velocityX - d2, 0.0, otherEntity.velocityZ - d);
                    otherEntity.velocityX *= (double)0.7f;
                    otherEntity.velocityZ *= (double)0.7f;
                } else if (((MinecartEntity)otherEntity).type != 2 && this.type == 2) {
                    otherEntity.velocityX *= (double)0.2f;
                    otherEntity.velocityZ *= (double)0.2f;
                    otherEntity.addVelocity(this.velocityX + d2, 0.0, this.velocityZ + d);
                    this.velocityX *= (double)0.7f;
                    this.velocityZ *= (double)0.7f;
                } else {
                    this.velocityX *= (double)0.2f;
                    this.velocityZ *= (double)0.2f;
                    this.addVelocity((d8 /= 2.0) - d2, 0.0, (d9 /= 2.0) - d);
                    otherEntity.velocityX *= (double)0.2f;
                    otherEntity.velocityZ *= (double)0.2f;
                    otherEntity.addVelocity(d8 + d2, 0.0, d9 + d);
                }
            } else {
                this.addVelocity(-d2, 0.0, -d);
                otherEntity.addVelocity(d2 / 4.0, 0.0, d / 4.0);
            }
        }
    }

    public int size() {
        return 27;
    }

    public ItemStack getStack(int slot) {
        return this.inventory[slot];
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.inventory[slot] != null) {
            if (this.inventory[slot].count <= amount) {
                ItemStack itemStack = this.inventory[slot];
                this.inventory[slot] = null;
                return itemStack;
            }
            ItemStack itemStack = this.inventory[slot].split(amount);
            if (this.inventory[slot].count == 0) {
                this.inventory[slot] = null;
            }
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.inventory[slot] = stack;
        if (stack != null && stack.count > this.getMaxCountPerStack()) {
            stack.count = this.getMaxCountPerStack();
        }
    }

    public String getName() {
        return "Minecart";
    }

    public int getMaxCountPerStack() {
        return 64;
    }

    public void markDirty() {
    }

    public boolean interact(PlayerEntity player) {
        if (this.type == 0) {
            if (this.passenger != null && this.passenger instanceof PlayerEntity && this.passenger != player) {
                return true;
            }
            if (!this.world.isRemote) {
                player.setVehicle(this);
            }
        } else if (this.type == 1) {
            if (!this.world.isRemote) {
                player.openChestScreen(this);
            }
        } else if (this.type == 2) {
            ItemStack itemStack = player.inventory.getSelectedItem();
            if (itemStack != null && itemStack.itemId == Item.COAL.id) {
                if (--itemStack.count == 0) {
                    player.inventory.setStack(player.inventory.selectedSlot, null);
                }
                this.fuel += 1200;
            }
            this.pushX = this.x - player.x;
            this.pushZ = this.z - player.z;
        }
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public void setPositionAndAnglesAvoidEntities(double x, double y, double z, float pitch, float yaw, int interpolationSteps) {
        this.clientX = x;
        this.clientY = y;
        this.clientZ = z;
        this.clientPitch = pitch;
        this.clientYaw = yaw;
        this.clientInterpolationSteps = interpolationSteps + 2;
        this.velocityX = this.clientVelocityX;
        this.velocityY = this.clientVelocityY;
        this.velocityZ = this.clientVelocityZ;
    }

    @Environment(value=EnvType.CLIENT)
    public void setVelocityClient(double x, double y, double z) {
        this.clientVelocityX = this.velocityX = x;
        this.clientVelocityY = this.velocityY = y;
        this.clientVelocityZ = this.velocityZ = z;
    }

    public boolean canPlayerUse(PlayerEntity player) {
        if (this.dead) {
            return false;
        }
        return !(player.getSquaredDistance(this) > 64.0);
    }
}

