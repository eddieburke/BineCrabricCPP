/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class NetherLavaSpringFeature
extends Feature {
    private int lavaBlockId;

    public NetherLavaSpringFeature(int i) {
        this.lavaBlockId = i;
    }

    public boolean generate(World world, Random random, int x, int y, int z) {
        if (world.getBlockId(x, y + 1, z) != Block.NETHERRACK.id) {
            return false;
        }
        if (world.getBlockId(x, y, z) != 0 && world.getBlockId(x, y, z) != Block.NETHERRACK.id) {
            return false;
        }
        int n = 0;
        if (world.getBlockId(x - 1, y, z) == Block.NETHERRACK.id) {
            ++n;
        }
        if (world.getBlockId(x + 1, y, z) == Block.NETHERRACK.id) {
            ++n;
        }
        if (world.getBlockId(x, y, z - 1) == Block.NETHERRACK.id) {
            ++n;
        }
        if (world.getBlockId(x, y, z + 1) == Block.NETHERRACK.id) {
            ++n;
        }
        if (world.getBlockId(x, y - 1, z) == Block.NETHERRACK.id) {
            ++n;
        }
        int n2 = 0;
        if (world.isAir(x - 1, y, z)) {
            ++n2;
        }
        if (world.isAir(x + 1, y, z)) {
            ++n2;
        }
        if (world.isAir(x, y, z - 1)) {
            ++n2;
        }
        if (world.isAir(x, y, z + 1)) {
            ++n2;
        }
        if (world.isAir(x, y - 1, z)) {
            ++n2;
        }
        if (n == 4 && n2 == 1) {
            world.setBlock(x, y, z, this.lavaBlockId);
            world.instantBlockUpdateEnabled = true;
            Block.BLOCKS[this.lavaBlockId].onTick(world, x, y, z, random);
            world.instantBlockUpdateEnabled = false;
        }
        return true;
    }
}

