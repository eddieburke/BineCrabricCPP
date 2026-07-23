/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.PlantBlock;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class DeadBushPatchFeature
extends Feature {
    private int deadBushBlockId;

    public DeadBushPatchFeature(int deadBushBlockId) {
        this.deadBushBlockId = deadBushBlockId;
    }

    public boolean generate(World world, Random random, int x, int y, int z) {
        int n = 0;
        while (((n = world.getBlockId(x, y, z)) == 0 || n == Block.LEAVES.id) && y > 0) {
            --y;
        }
        for (int i = 0; i < 4; ++i) {
            int n2;
            int n3;
            int n4 = x + random.nextInt(8) - random.nextInt(8);
            if (!world.isAir(n4, n3 = y + random.nextInt(4) - random.nextInt(4), n2 = z + random.nextInt(8) - random.nextInt(8)) || !((PlantBlock)Block.BLOCKS[this.deadBushBlockId]).canGrow(world, n4, n3, n2)) continue;
            world.setBlockWithoutNotifyingNeighbors(n4, n3, n2, this.deadBushBlockId);
        }
        return true;
    }
}

