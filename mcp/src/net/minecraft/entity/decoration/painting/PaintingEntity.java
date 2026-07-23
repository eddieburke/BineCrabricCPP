/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.decoration.painting;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.decoration.painting.PaintingVariants;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class PaintingEntity
extends Entity {
    private int obstructionCheckCounter = 0;
    public int facing = 0;
    public int attachmentX;
    public int attachmentY;
    public int attachmentZ;
    public PaintingVariants variant;

    public PaintingEntity(World arg) {
        super(arg);
        this.standingEyeHeight = 0.0f;
        this.setBoundingBoxSpacing(0.5f, 0.5f);
    }

    public PaintingEntity(World world, int x, int y, int z, int facing) {
        this(world);
        this.attachmentX = x;
        this.attachmentY = y;
        this.attachmentZ = z;
        ArrayList<PaintingVariants> arrayList = new ArrayList<PaintingVariants>();
        PaintingVariants[] paintingVariantsArray = PaintingVariants.values();
        int n = paintingVariantsArray.length;
        for (int i = 0; i < n; ++i) {
            PaintingVariants paintingVariants;
            this.variant = paintingVariants = paintingVariantsArray[i];
            this.setFacing(facing);
            if (!this.canStayAttached()) continue;
            arrayList.add(paintingVariants);
        }
        if (arrayList.size() > 0) {
            this.variant = (PaintingVariants)((Object)arrayList.get(this.random.nextInt(arrayList.size())));
        }
        this.setFacing(facing);
    }

    @Environment(value=EnvType.CLIENT)
    public PaintingEntity(World world, int x, int y, int z, int facing, String variant) {
        this(world);
        this.attachmentX = x;
        this.attachmentY = y;
        this.attachmentZ = z;
        for (PaintingVariants paintingVariants : PaintingVariants.values()) {
            if (!paintingVariants.id.equals(variant)) continue;
            this.variant = paintingVariants;
            break;
        }
        this.setFacing(facing);
    }

    protected void initDataTracker() {
    }

    public void setFacing(int facing) {
        this.facing = facing;
        this.prevYaw = this.yaw = (float)(facing * 90);
        float f = this.variant.width;
        float f2 = this.variant.height;
        float f3 = this.variant.width;
        if (facing == 0 || facing == 2) {
            f3 = 0.5f;
        } else {
            f = 0.5f;
        }
        f /= 32.0f;
        f2 /= 32.0f;
        f3 /= 32.0f;
        float f4 = (float)this.attachmentX + 0.5f;
        float f5 = (float)this.attachmentY + 0.5f;
        float f6 = (float)this.attachmentZ + 0.5f;
        float f7 = 0.5625f;
        if (facing == 0) {
            f6 -= f7;
        }
        if (facing == 1) {
            f4 -= f7;
        }
        if (facing == 2) {
            f6 += f7;
        }
        if (facing == 3) {
            f4 += f7;
        }
        if (facing == 0) {
            f4 -= this.getHorizontalOffset(this.variant.width);
        }
        if (facing == 1) {
            f6 += this.getHorizontalOffset(this.variant.width);
        }
        if (facing == 2) {
            f4 += this.getHorizontalOffset(this.variant.width);
        }
        if (facing == 3) {
            f6 -= this.getHorizontalOffset(this.variant.width);
        }
        this.setPosition(f4, f5 += this.getHorizontalOffset(this.variant.height), f6);
        float f8 = -0.00625f;
        this.boundingBox.set(f4 - f - f8, f5 - f2 - f8, f6 - f3 - f8, f4 + f + f8, f5 + f2 + f8, f6 + f3 + f8);
    }

    private float getHorizontalOffset(int width) {
        if (width == 32) {
            return 0.5f;
        }
        if (width == 64) {
            return 0.5f;
        }
        return 0.0f;
    }

    public void tick() {
        if (this.obstructionCheckCounter++ == 100 && !this.world.isRemote) {
            this.obstructionCheckCounter = 0;
            if (!this.canStayAttached()) {
                this.markDead();
                this.world.spawnEntity(new ItemEntity(this.world, this.x, this.y, this.z, new ItemStack(Item.PAINTING)));
            }
        }
    }

    public boolean canStayAttached() {
        int n;
        if (this.world.getEntityCollisions(this, this.boundingBox).size() > 0) {
            return false;
        }
        int n2 = this.variant.width / 16;
        int n3 = this.variant.height / 16;
        int n4 = this.attachmentX;
        int n5 = this.attachmentY;
        int n6 = this.attachmentZ;
        if (this.facing == 0) {
            n4 = MathHelper.floor(this.x - (double)((float)this.variant.width / 32.0f));
        }
        if (this.facing == 1) {
            n6 = MathHelper.floor(this.z - (double)((float)this.variant.width / 32.0f));
        }
        if (this.facing == 2) {
            n4 = MathHelper.floor(this.x - (double)((float)this.variant.width / 32.0f));
        }
        if (this.facing == 3) {
            n6 = MathHelper.floor(this.z - (double)((float)this.variant.width / 32.0f));
        }
        n5 = MathHelper.floor(this.y - (double)((float)this.variant.height / 32.0f));
        for (int i = 0; i < n2; ++i) {
            for (n = 0; n < n3; ++n) {
                Material material = this.facing == 0 || this.facing == 2 ? this.world.getMaterial(n4 + i, n5 + n, this.attachmentZ) : this.world.getMaterial(this.attachmentX, n5 + n, n6 + i);
                if (material.isSolid()) continue;
                return false;
            }
        }
        List list = this.world.getEntities(this, this.boundingBox);
        for (n = 0; n < list.size(); ++n) {
            if (!(list.get(n) instanceof PaintingEntity)) continue;
            return false;
        }
        return true;
    }

    public boolean isCollidable() {
        return true;
    }

    public boolean damage(Entity damageSource, int amount) {
        if (!this.dead && !this.world.isRemote) {
            this.markDead();
            this.scheduleVelocityUpdate();
            this.world.spawnEntity(new ItemEntity(this.world, this.x, this.y, this.z, new ItemStack(Item.PAINTING)));
        }
        return true;
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putByte("Dir", (byte)this.facing);
        nbt.putString("Motive", this.variant.id);
        nbt.putInt("TileX", this.attachmentX);
        nbt.putInt("TileY", this.attachmentY);
        nbt.putInt("TileZ", this.attachmentZ);
    }

    public void readNbt(NbtCompound nbt) {
        this.facing = nbt.getByte("Dir");
        this.attachmentX = nbt.getInt("TileX");
        this.attachmentY = nbt.getInt("TileY");
        this.attachmentZ = nbt.getInt("TileZ");
        String string = nbt.getString("Motive");
        for (PaintingVariants paintingVariants : PaintingVariants.values()) {
            if (!paintingVariants.id.equals(string)) continue;
            this.variant = paintingVariants;
        }
        if (this.variant == null) {
            this.variant = PaintingVariants.KEBAB;
        }
        this.setFacing(this.facing);
    }

    public void move(double dx, double dy, double dz) {
        if (!this.world.isRemote && dx * dx + dy * dy + dz * dz > 0.0) {
            this.markDead();
            this.world.spawnEntity(new ItemEntity(this.world, this.x, this.y, this.z, new ItemStack(Item.PAINTING)));
        }
    }

    public void addVelocity(double x, double y, double z) {
        if (!this.world.isRemote && x * x + y * y + z * z > 0.0) {
            this.markDead();
            this.world.spawnEntity(new ItemEntity(this.world, this.x, this.y, this.z, new ItemStack(Item.PAINTING)));
        }
    }
}

