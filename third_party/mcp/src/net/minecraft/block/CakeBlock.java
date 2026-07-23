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
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class CakeBlock
extends Block {
    protected CakeBlock(int id, int textureId) {
        super(id, textureId, Material.CAKE);
        this.setTickRandomly(true);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        float f = 0.0625f;
        float f2 = (float)(1 + n * 2) / 16.0f;
        float f3 = 0.5f;
        this.setBoundingBox(f2, 0.0f, f, 1.0f - f, f3, 1.0f - f);
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
        float f = 0.0625f;
        float f2 = 0.5f;
        this.setBoundingBox(f, 0.0f, f, 1.0f - f, f2, 1.0f - f);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        float f = 0.0625f;
        float f2 = (float)(1 + n * 2) / 16.0f;
        float f3 = 0.5f;
        return Box.createCached((float)x + f2, y, (float)z + f, (float)(x + 1) - f, (float)y + f3 - f, (float)(z + 1) - f);
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        float f = 0.0625f;
        float f2 = (float)(1 + n * 2) / 16.0f;
        float f3 = 0.5f;
        return Box.createCached((float)x + f2, y, (float)z + f, (float)(x + 1) - f, (float)y + f3, (float)(z + 1) - f);
    }

    public int getTexture(int side, int meta) {
        if (side == 1) {
            return this.textureId;
        }
        if (side == 0) {
            return this.textureId + 3;
        }
        if (meta > 0 && side == 4) {
            return this.textureId + 2;
        }
        return this.textureId + 1;
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId;
        }
        if (side == 0) {
            return this.textureId + 3;
        }
        return this.textureId + 1;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        this.tryEat(world, x, y, z, player);
        return true;
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.tryEat(world, x, y, z, player);
    }

    private void tryEat(World world, int x, int y, int z, PlayerEntity player) {
        if (player.health < 20) {
            player.heal(3);
            int n = world.getBlockMeta(x, y, z) + 1;
            if (n >= 6) {
                world.setBlock(x, y, z, 0);
            } else {
                world.setBlockMeta(x, y, z, n);
                world.setBlockDirty(x, y, z);
            }
        }
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (!super.canPlaceAt(world, x, y, z)) {
            return false;
        }
        return this.canGrow(world, x, y, z);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (!this.canGrow(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }

    public boolean canGrow(World world, int x, int y, int z) {
        return world.getMaterial(x, y - 1, z).isSolid();
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return 0;
    }
}

