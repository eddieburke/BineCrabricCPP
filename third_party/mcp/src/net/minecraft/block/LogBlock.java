/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.world.World;

public class LogBlock
extends Block {
    protected LogBlock(int id) {
        super(id, Material.WOOD);
        this.textureId = 20;
    }

    public int getDroppedItemCount(Random random) {
        return 1;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.LOG.id;
    }

    public void afterBreak(World world, PlayerEntity playerEntity, int x, int y, int z, int meta) {
        super.afterBreak(world, playerEntity, x, y, z, meta);
    }

    public void onBreak(World world, int x, int y, int z) {
        int n = 4;
        int n2 = n + 1;
        if (world.isRegionLoaded(x - n2, y - n2, z - n2, x + n2, y + n2, z + n2)) {
            for (int i = -n; i <= n; ++i) {
                for (int j = -n; j <= n; ++j) {
                    for (int k = -n; k <= n; ++k) {
                        int n3;
                        int n4 = world.getBlockId(x + i, y + j, z + k);
                        if (n4 != Block.LEAVES.id || ((n3 = world.getBlockMeta(x + i, y + j, z + k)) & 8) != 0) continue;
                        world.setBlockMetaWithoutNotifyingNeighbors(x + i, y + j, z + k, n3 | 8);
                    }
                }
            }
        }
    }

    public int getTexture(int side, int meta) {
        if (side == 1) {
            return 21;
        }
        if (side == 0) {
            return 21;
        }
        if (meta == 1) {
            return 116;
        }
        if (meta == 2) {
            return 117;
        }
        return 20;
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return blockMeta;
    }
}

