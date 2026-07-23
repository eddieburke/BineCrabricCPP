/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.PlantBlock;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.BirchTreeFeature;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.LargeOakTreeFeature;
import net.minecraft.world.gen.feature.OakTreeFeature;
import net.minecraft.world.gen.feature.SpruceTreeFeature;

public class SaplingBlock
extends PlantBlock {
    protected SaplingBlock(int i, int j) {
        super(i, j);
        float f = 0.4f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        super.onTick(world, x, y, z, random);
        if (world.getLightLevel(x, y + 1, z) >= 9 && random.nextInt(30) == 0) {
            int n = world.getBlockMeta(x, y, z);
            if ((n & 8) == 0) {
                world.setBlockMeta(x, y, z, n | 8);
            } else {
                this.generate(world, x, y, z, random);
            }
        }
    }

    public int getTexture(int side, int meta) {
        if ((meta &= 3) == 1) {
            return 63;
        }
        if (meta == 2) {
            return 79;
        }
        return super.getTexture(side, meta);
    }

    public void generate(World world, int x, int y, int z, Random random) {
        int n = world.getBlockMeta(x, y, z) & 3;
        world.setBlockWithoutNotifyingNeighbors(x, y, z, 0);
        Feature feature = null;
        if (n == 1) {
            feature = new SpruceTreeFeature();
        } else if (n == 2) {
            feature = new BirchTreeFeature();
        } else {
            feature = new OakTreeFeature();
            if (random.nextInt(10) == 0) {
                feature = new LargeOakTreeFeature();
            }
        }
        if (!((Feature)feature).generate(world, random, x, y, z)) {
            world.setBlockWithoutNotifyingNeighbors(x, y, z, this.id, n);
        }
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return blockMeta & 3;
    }
}

