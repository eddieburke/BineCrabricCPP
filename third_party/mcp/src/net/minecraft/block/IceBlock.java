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
import net.minecraft.block.TranslucentBlock;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.world.BlockView;
import net.minecraft.world.LightType;
import net.minecraft.world.World;

public class IceBlock
extends TranslucentBlock {
    public IceBlock(int id, int textureId) {
        super(id, textureId, Material.ICE, false);
        this.slipperiness = 0.98f;
        this.setTickRandomly(true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return 1;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        return super.isSideVisible(blockView, x, y, z, 1 - side);
    }

    public void afterBreak(World world, PlayerEntity playerEntity, int x, int y, int z, int meta) {
        super.afterBreak(world, playerEntity, x, y, z, meta);
        Material material = world.getMaterial(x, y - 1, z);
        if (material.blocksMovement() || material.isFluid()) {
            world.setBlock(x, y, z, Block.FLOWING_WATER.id);
        }
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.getBrightness(LightType.BLOCK, x, y, z) > 11 - Block.BLOCKS_LIGHT_OPACITY[this.id]) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, Block.WATER.id);
        }
    }

    public int getPistonBehavior() {
        return 0;
    }
}

