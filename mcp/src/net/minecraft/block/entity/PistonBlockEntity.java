/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block.entity;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.PistonConstants;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.Box;

public class PistonBlockEntity
extends BlockEntity {
    private int pushedBlockId;
    private int pushedBlockData;
    private int facing;
    private boolean extending;
    private boolean source;
    private float lastProgress;
    private float progress;
    private static List pushedEntities = new ArrayList();

    public PistonBlockEntity() {
    }

    public PistonBlockEntity(int pushedBlockId, int pushedBlockData, int facing, boolean extending, boolean source) {
        this.pushedBlockId = pushedBlockId;
        this.pushedBlockData = pushedBlockData;
        this.facing = facing;
        this.extending = extending;
        this.source = source;
    }

    public int getPushedBlockId() {
        return this.pushedBlockId;
    }

    public int getPushedBlockData() {
        return this.pushedBlockData;
    }

    public boolean isExtending() {
        return this.extending;
    }

    public int getFacing() {
        return this.facing;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSource() {
        return this.source;
    }

    public float getProgress(float tickDelta) {
        if (tickDelta > 1.0f) {
            tickDelta = 1.0f;
        }
        return this.progress + (this.lastProgress - this.progress) * tickDelta;
    }

    @Environment(value=EnvType.CLIENT)
    public float getRenderOffsetX(float tickDelta) {
        if (this.extending) {
            return (this.getProgress(tickDelta) - 1.0f) * (float)PistonConstants.HEAD_OFFSET_X[this.facing];
        }
        return (1.0f - this.getProgress(tickDelta)) * (float)PistonConstants.HEAD_OFFSET_X[this.facing];
    }

    @Environment(value=EnvType.CLIENT)
    public float getRenderOffsetY(float tickDelta) {
        if (this.extending) {
            return (this.getProgress(tickDelta) - 1.0f) * (float)PistonConstants.HEAD_OFFSET_Y[this.facing];
        }
        return (1.0f - this.getProgress(tickDelta)) * (float)PistonConstants.HEAD_OFFSET_Y[this.facing];
    }

    @Environment(value=EnvType.CLIENT)
    public float getRenderOffsetZ(float tickDelta) {
        if (this.extending) {
            return (this.getProgress(tickDelta) - 1.0f) * (float)PistonConstants.HEAD_OFFSET_Z[this.facing];
        }
        return (1.0f - this.getProgress(tickDelta)) * (float)PistonConstants.HEAD_OFFSET_Z[this.facing];
    }

    private void pushEntities(float collisionShapeSizeMultiplier, float entityMoveMultiplier) {
        List list;
        collisionShapeSizeMultiplier = !this.extending ? (collisionShapeSizeMultiplier -= 1.0f) : 1.0f - collisionShapeSizeMultiplier;
        Box box = Block.MOVING_PISTON.getPushedBlockCollisionShape(this.world, this.x, this.y, this.z, this.pushedBlockId, collisionShapeSizeMultiplier, this.facing);
        if (box != null && !(list = this.world.getEntities(null, box)).isEmpty()) {
            pushedEntities.addAll(list);
            for (Entity entity : pushedEntities) {
                entity.move(entityMoveMultiplier * (float)PistonConstants.HEAD_OFFSET_X[this.facing], entityMoveMultiplier * (float)PistonConstants.HEAD_OFFSET_Y[this.facing], entityMoveMultiplier * (float)PistonConstants.HEAD_OFFSET_Z[this.facing]);
            }
            pushedEntities.clear();
        }
    }

    public void finish() {
        if (this.progress < 1.0f) {
            this.lastProgress = 1.0f;
            this.progress = 1.0f;
            this.world.removeBlockEntity(this.x, this.y, this.z);
            this.markRemoved();
            if (this.world.getBlockId(this.x, this.y, this.z) == Block.MOVING_PISTON.id) {
                this.world.setBlock(this.x, this.y, this.z, this.pushedBlockId, this.pushedBlockData);
            }
        }
    }

    public void tick() {
        this.progress = this.lastProgress;
        if (this.progress >= 1.0f) {
            this.pushEntities(1.0f, 0.25f);
            this.world.removeBlockEntity(this.x, this.y, this.z);
            this.markRemoved();
            if (this.world.getBlockId(this.x, this.y, this.z) == Block.MOVING_PISTON.id) {
                this.world.setBlock(this.x, this.y, this.z, this.pushedBlockId, this.pushedBlockData);
            }
            return;
        }
        this.lastProgress += 0.5f;
        if (this.lastProgress >= 1.0f) {
            this.lastProgress = 1.0f;
        }
        if (this.extending) {
            this.pushEntities(this.lastProgress, this.lastProgress - this.progress + 0.0625f);
        }
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.pushedBlockId = nbt.getInt("blockId");
        this.pushedBlockData = nbt.getInt("blockData");
        this.facing = nbt.getInt("facing");
        this.progress = this.lastProgress = nbt.getFloat("progress");
        this.extending = nbt.getBoolean("extending");
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putInt("blockId", this.pushedBlockId);
        nbt.putInt("blockData", this.pushedBlockData);
        nbt.putInt("facing", this.facing);
        nbt.putFloat("progress", this.progress);
        nbt.putBoolean("extending", this.extending);
    }
}

