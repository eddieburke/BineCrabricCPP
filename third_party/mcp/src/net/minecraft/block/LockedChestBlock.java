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

public class LockedChestBlock
extends Block {
    protected LockedChestBlock(int id) {
        super(id, Material.WOOD);
        this.textureId = 26;
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(BlockView blockView, int x, int y, int z, int side) {
        if (side == 1) {
            return this.textureId - 1;
        }
        if (side == 0) {
            return this.textureId - 1;
        }
        int n = blockView.getBlockId(x, y, z - 1);
        int n2 = blockView.getBlockId(x, y, z + 1);
        int n3 = blockView.getBlockId(x - 1, y, z);
        int n4 = blockView.getBlockId(x + 1, y, z);
        int n5 = 3;
        if (Block.BLOCKS_OPAQUE[n] && !Block.BLOCKS_OPAQUE[n2]) {
            n5 = 3;
        }
        if (Block.BLOCKS_OPAQUE[n2] && !Block.BLOCKS_OPAQUE[n]) {
            n5 = 2;
        }
        if (Block.BLOCKS_OPAQUE[n3] && !Block.BLOCKS_OPAQUE[n4]) {
            n5 = 5;
        }
        if (Block.BLOCKS_OPAQUE[n4] && !Block.BLOCKS_OPAQUE[n3]) {
            n5 = 4;
        }
        return side == n5 ? this.textureId + 1 : this.textureId;
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId - 1;
        }
        if (side == 0) {
            return this.textureId - 1;
        }
        if (side == 3) {
            return this.textureId + 1;
        }
        return this.textureId;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return true;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        world.setBlock(x, y, z, 0);
    }
}

