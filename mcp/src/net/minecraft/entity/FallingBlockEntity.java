/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.SandBlock;
import net.minecraft.entity.Entity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class FallingBlockEntity
extends Entity {
    public int blockId;
    public int timeFalling = 0;

    public FallingBlockEntity(World arg) {
        super(arg);
    }

    public FallingBlockEntity(World world, double x, double y, double z, int blockId) {
        super(world);
        this.blockId = blockId;
        this.blocksSameBlockSpawning = true;
        this.setBoundingBoxSpacing(0.98f, 0.98f);
        this.standingEyeHeight = this.height / 2.0f;
        this.setPosition(x, y, z);
        this.velocityX = 0.0;
        this.velocityY = 0.0;
        this.velocityZ = 0.0;
        this.prevX = x;
        this.prevY = y;
        this.prevZ = z;
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    protected void initDataTracker() {
    }

    public boolean isCollidable() {
        return !this.dead;
    }

    public void tick() {
        if (this.blockId == 0) {
            this.markDead();
            return;
        }
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        ++this.timeFalling;
        this.velocityY -= (double)0.04f;
        this.move(this.velocityX, this.velocityY, this.velocityZ);
        this.velocityX *= (double)0.98f;
        this.velocityY *= (double)0.98f;
        this.velocityZ *= (double)0.98f;
        int n = MathHelper.floor(this.x);
        int n2 = MathHelper.floor(this.y);
        int n3 = MathHelper.floor(this.z);
        if (this.world.getBlockId(n, n2, n3) == this.blockId) {
            this.world.setBlock(n, n2, n3, 0);
        }
        if (this.onGround) {
            this.velocityX *= (double)0.7f;
            this.velocityZ *= (double)0.7f;
            this.velocityY *= -0.5;
            this.markDead();
            if (!(this.world.canPlace(this.blockId, n, n2, n3, true, 1) && !SandBlock.canFallThrough(this.world, n, n2 - 1, n3) && this.world.setBlock(n, n2, n3, this.blockId) || this.world.isRemote)) {
                this.dropItem(this.blockId, 1);
            }
        } else if (this.timeFalling > 100 && !this.world.isRemote) {
            this.dropItem(this.blockId, 1);
            this.markDead();
        }
    }

    protected void writeNbt(NbtCompound nbt) {
        nbt.putByte("Tile", (byte)this.blockId);
    }

    protected void readNbt(NbtCompound nbt) {
        this.blockId = nbt.getByte("Tile") & 0xFF;
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return 0.0f;
    }

    @Environment(value=EnvType.CLIENT)
    public World getWorld() {
        return this.world;
    }
}

