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
import net.minecraft.item.Item;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class SugarCaneBlock
extends Block {
    protected SugarCaneBlock(int id, int textureId) {
        super(id, Material.PLANT);
        this.textureId = textureId;
        float f = 0.375f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
        this.setTickRandomly(true);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isAir(x, y + 1, z)) {
            int n = 1;
            while (world.getBlockId(x, y - n, z) == this.id) {
                ++n;
            }
            if (n < 3) {
                int n2 = world.getBlockMeta(x, y, z);
                if (n2 == 15) {
                    world.setBlock(x, y + 1, z, this.id);
                    world.setBlockMeta(x, y, z, 0);
                } else {
                    world.setBlockMeta(x, y, z, n2 + 1);
                }
            }
        }
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y - 1, z);
        if (n == this.id) {
            return true;
        }
        if (n != Block.GRASS_BLOCK.id && n != Block.DIRT.id) {
            return false;
        }
        if (world.getMaterial(x - 1, y - 1, z) == Material.WATER) {
            return true;
        }
        if (world.getMaterial(x + 1, y - 1, z) == Material.WATER) {
            return true;
        }
        if (world.getMaterial(x, y - 1, z - 1) == Material.WATER) {
            return true;
        }
        return world.getMaterial(x, y - 1, z + 1) == Material.WATER;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        this.breakIfCannotGrow(world, x, y, z);
    }

    protected final void breakIfCannotGrow(World world, int x, int y, int z) {
        if (!this.canGrow(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }

    public boolean canGrow(World world, int x, int y, int z) {
        return this.canPlaceAt(world, x, y, z);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.SUGAR_CANE.id;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 1;
    }
}

