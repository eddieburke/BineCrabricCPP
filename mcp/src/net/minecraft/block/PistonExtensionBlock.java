/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.PistonConstants;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.PistonBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class PistonExtensionBlock
extends BlockWithEntity {
    public PistonExtensionBlock(int id) {
        super(id, Material.PISTON);
        this.setHardness(-1.0f);
    }

    protected BlockEntity createBlockEntity() {
        return null;
    }

    public void onPlaced(World world, int x, int y, int z) {
    }

    public void onBreak(World world, int x, int y, int z) {
        BlockEntity blockEntity = world.getBlockEntity(x, y, z);
        if (blockEntity != null && blockEntity instanceof PistonBlockEntity) {
            ((PistonBlockEntity)blockEntity).finish();
        } else {
            super.onBreak(world, x, y, z);
        }
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return -1;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (!world.isRemote && world.getBlockEntity(x, y, z) == null) {
            world.setBlock(x, y, z, 0);
            return true;
        }
        return false;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return 0;
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        if (world.isRemote) {
            return;
        }
        PistonBlockEntity pistonBlockEntity = this.getPistonBlockEntity(world, x, y, z);
        if (pistonBlockEntity == null) {
            return;
        }
        Block.BLOCKS[pistonBlockEntity.getPushedBlockId()].dropStacks(world, x, y, z, pistonBlockEntity.getPushedBlockData());
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (world.isRemote || world.getBlockEntity(x, y, z) == null) {
            // empty if block
        }
    }

    public static BlockEntity createPistonBlockEntity(int blockId, int blockMeta, int facing, boolean extending, boolean source) {
        return new PistonBlockEntity(blockId, blockMeta, facing, extending, source);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        PistonBlockEntity pistonBlockEntity = this.getPistonBlockEntity(world, x, y, z);
        if (pistonBlockEntity == null) {
            return null;
        }
        float f = pistonBlockEntity.getProgress(0.0f);
        if (pistonBlockEntity.isExtending()) {
            f = 1.0f - f;
        }
        return this.getPushedBlockCollisionShape(world, x, y, z, pistonBlockEntity.getPushedBlockId(), f, pistonBlockEntity.getFacing());
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        PistonBlockEntity pistonBlockEntity = this.getPistonBlockEntity(blockView, x, y, z);
        if (pistonBlockEntity != null) {
            Block block = Block.BLOCKS[pistonBlockEntity.getPushedBlockId()];
            if (block == null || block == this) {
                return;
            }
            block.updateBoundingBox(blockView, x, y, z);
            float f = pistonBlockEntity.getProgress(0.0f);
            if (pistonBlockEntity.isExtending()) {
                f = 1.0f - f;
            }
            int n = pistonBlockEntity.getFacing();
            this.minX = block.minX - (double)((float)PistonConstants.HEAD_OFFSET_X[n] * f);
            this.minY = block.minY - (double)((float)PistonConstants.HEAD_OFFSET_Y[n] * f);
            this.minZ = block.minZ - (double)((float)PistonConstants.HEAD_OFFSET_Z[n] * f);
            this.maxX = block.maxX - (double)((float)PistonConstants.HEAD_OFFSET_X[n] * f);
            this.maxY = block.maxY - (double)((float)PistonConstants.HEAD_OFFSET_Y[n] * f);
            this.maxZ = block.maxZ - (double)((float)PistonConstants.HEAD_OFFSET_Z[n] * f);
        }
    }

    public Box getPushedBlockCollisionShape(World world, int x, int y, int z, int blockId, float sizeMultiplier, int facing) {
        if (blockId == 0 || blockId == this.id) {
            return null;
        }
        Box box = Block.BLOCKS[blockId].getCollisionShape(world, x, y, z);
        if (box == null) {
            return null;
        }
        box.minX -= (double)((float)PistonConstants.HEAD_OFFSET_X[facing] * sizeMultiplier);
        box.maxX -= (double)((float)PistonConstants.HEAD_OFFSET_X[facing] * sizeMultiplier);
        box.minY -= (double)((float)PistonConstants.HEAD_OFFSET_Y[facing] * sizeMultiplier);
        box.maxY -= (double)((float)PistonConstants.HEAD_OFFSET_Y[facing] * sizeMultiplier);
        box.minZ -= (double)((float)PistonConstants.HEAD_OFFSET_Z[facing] * sizeMultiplier);
        box.maxZ -= (double)((float)PistonConstants.HEAD_OFFSET_Z[facing] * sizeMultiplier);
        return box;
    }

    private PistonBlockEntity getPistonBlockEntity(BlockView blockView, int x, int y, int z) {
        BlockEntity blockEntity = blockView.getBlockEntity(x, y, z);
        if (blockEntity != null && blockEntity instanceof PistonBlockEntity) {
            return (PistonBlockEntity)blockEntity;
        }
        return null;
    }
}

