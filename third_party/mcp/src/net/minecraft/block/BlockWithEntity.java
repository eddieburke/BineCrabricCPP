/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.world.World;

public abstract class BlockWithEntity
extends Block {
    protected BlockWithEntity(int i, Material arg) {
        super(i, arg);
        BlockWithEntity.BLOCKS_WITH_ENTITY[i] = true;
    }

    protected BlockWithEntity(int i, int j, Material arg) {
        super(i, j, arg);
        BlockWithEntity.BLOCKS_WITH_ENTITY[i] = true;
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
        world.setBlockEntity(x, y, z, this.createBlockEntity());
    }

    public void onBreak(World world, int x, int y, int z) {
        super.onBreak(world, x, y, z);
        world.removeBlockEntity(x, y, z);
    }

    protected abstract BlockEntity createBlockEntity();
}

