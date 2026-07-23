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
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class SlabBlock
extends Block {
    public static final String[] names = new String[]{"stone", "sand", "wood", "cobble"};
    private boolean doubleSlab;

    public SlabBlock(int id, boolean doubleSlab) {
        super(id, 6, Material.STONE);
        this.doubleSlab = doubleSlab;
        if (!doubleSlab) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        }
        this.setOpacity(255);
    }

    public int getTexture(int side, int meta) {
        if (meta == 0) {
            if (side <= 1) {
                return 6;
            }
            return 5;
        }
        if (meta == 1) {
            if (side == 0) {
                return 208;
            }
            if (side == 1) {
                return 176;
            }
            return 192;
        }
        if (meta == 2) {
            return 4;
        }
        if (meta == 3) {
            return 16;
        }
        return 6;
    }

    public int getTexture(int side) {
        return this.getTexture(side, 0);
    }

    public boolean isOpaque() {
        return this.doubleSlab;
    }

    public void onPlaced(World world, int x, int y, int z) {
        int n;
        if (this != Block.SLAB) {
            super.onPlaced(world, x, y, z);
        }
        int n2 = world.getBlockId(x, y - 1, z);
        int n3 = world.getBlockMeta(x, y, z);
        if (n3 != (n = world.getBlockMeta(x, y - 1, z))) {
            return;
        }
        if (n2 == SlabBlock.SLAB.id) {
            world.setBlock(x, y, z, 0);
            world.setBlock(x, y - 1, z, Block.DOUBLE_SLAB.id, n3);
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.SLAB.id;
    }

    public int getDroppedItemCount(Random random) {
        if (this.doubleSlab) {
            return 2;
        }
        return 1;
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return blockMeta;
    }

    public boolean isFullCube() {
        return this.doubleSlab;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        if (this != Block.SLAB) {
            super.isSideVisible(blockView, x, y, z, side);
        }
        if (side == 1) {
            return true;
        }
        if (!super.isSideVisible(blockView, x, y, z, side)) {
            return false;
        }
        if (side == 0) {
            return true;
        }
        return blockView.getBlockId(x, y, z) != this.id;
    }
}

