/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.world.World;

public class SpongeBlock
extends Block {
    protected SpongeBlock(int id) {
        super(id, Material.SPONGE);
        this.textureId = 48;
    }

    public void onPlaced(World world, int x, int y, int z) {
        int n = 2;
        for (int i = x - n; i <= x + n; ++i) {
            for (int j = y - n; j <= y + n; ++j) {
                for (int k = z - n; k <= z + n; ++k) {
                    if (world.getMaterial(i, j, k) != Material.WATER) continue;
                }
            }
        }
    }

    public void onBreak(World world, int x, int y, int z) {
        int n = 2;
        for (int i = x - n; i <= x + n; ++i) {
            for (int j = y - n; j <= y + n; ++j) {
                for (int k = z - n; k <= z + n; ++k) {
                    world.notifyNeighbors(i, j, k, world.getBlockId(i, j, k));
                }
            }
        }
    }
}

