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
import net.minecraft.block.material.Material;
import net.minecraft.client.color.world.GrassColors;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class GrassBlock
extends Block {
    protected GrassBlock(int id) {
        super(id, Material.SOLID_ORGANIC);
        this.textureId = 3;
        this.setTickRandomly(true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(BlockView blockView, int x, int y, int z, int side) {
        if (side == 1) {
            return 0;
        }
        if (side == 0) {
            return 2;
        }
        Material material = blockView.getMaterial(x, y + 1, z);
        if (material == Material.SNOW_LAYER || material == Material.SNOW_BLOCK) {
            return 68;
        }
        return 3;
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        blockView.getBiomeSource().getBiomesInArea(x, z, 1, 1);
        double d = blockView.getBiomeSource().temperatureMap[0];
        double d2 = blockView.getBiomeSource().downfallMap[0];
        return GrassColors.getColor(d, d2);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        if (world.getLightLevel(x, y + 1, z) < 4 && Block.BLOCKS_LIGHT_OPACITY[world.getBlockId(x, y + 1, z)] > 2) {
            if (random.nextInt(4) != 0) {
                return;
            }
            world.setBlock(x, y, z, Block.DIRT.id);
        } else if (world.getLightLevel(x, y + 1, z) >= 9) {
            int n = x + random.nextInt(3) - 1;
            int n2 = y + random.nextInt(5) - 3;
            int n3 = z + random.nextInt(3) - 1;
            int n4 = world.getBlockId(n, n2 + 1, n3);
            if (world.getBlockId(n, n2, n3) == Block.DIRT.id && world.getLightLevel(n, n2 + 1, n3) >= 4 && Block.BLOCKS_LIGHT_OPACITY[n4] <= 2) {
                world.setBlock(n, n2, n3, Block.GRASS_BLOCK.id);
            }
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.DIRT.getDroppedItemId(0, random);
    }
}

