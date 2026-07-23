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
import net.minecraft.block.PlantBlock;
import net.minecraft.entity.ItemEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class CropBlock
extends PlantBlock {
    protected CropBlock(int i, int j) {
        super(i, j);
        this.textureId = j;
        this.setTickRandomly(true);
        float f = 0.5f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
    }

    protected boolean canPlantOnTop(int id) {
        return id == Block.FARMLAND.id;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        float f;
        int n;
        super.onTick(world, x, y, z, random);
        if (world.getLightLevel(x, y + 1, z) >= 9 && (n = world.getBlockMeta(x, y, z)) < 7 && random.nextInt((int)(100.0f / (f = this.getAvailableMoisture(world, x, y, z)))) == 0) {
            world.setBlockMeta(x, y, z, ++n);
        }
    }

    public void applyFullGrowth(World world, int x, int y, int z) {
        world.setBlockMeta(x, y, z, 7);
    }

    private float getAvailableMoisture(World world, int x, int y, int z) {
        float f = 1.0f;
        int n = world.getBlockId(x, y, z - 1);
        int n2 = world.getBlockId(x, y, z + 1);
        int n3 = world.getBlockId(x - 1, y, z);
        int n4 = world.getBlockId(x + 1, y, z);
        int n5 = world.getBlockId(x - 1, y, z - 1);
        int n6 = world.getBlockId(x + 1, y, z - 1);
        int n7 = world.getBlockId(x + 1, y, z + 1);
        int n8 = world.getBlockId(x - 1, y, z + 1);
        boolean bl = n3 == this.id || n4 == this.id;
        boolean bl2 = n == this.id || n2 == this.id;
        boolean bl3 = n5 == this.id || n6 == this.id || n7 == this.id || n8 == this.id;
        for (int i = x - 1; i <= x + 1; ++i) {
            for (int j = z - 1; j <= z + 1; ++j) {
                int n9 = world.getBlockId(i, y - 1, j);
                float f2 = 0.0f;
                if (n9 == Block.FARMLAND.id) {
                    f2 = 1.0f;
                    if (world.getBlockMeta(i, y - 1, j) > 0) {
                        f2 = 3.0f;
                    }
                }
                if (i != x || j != z) {
                    f2 /= 4.0f;
                }
                f += f2;
            }
        }
        if (bl3 || bl && bl2) {
            f /= 2.0f;
        }
        return f;
    }

    public int getTexture(int side, int meta) {
        if (meta < 0) {
            meta = 7;
        }
        return this.textureId + meta;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 6;
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        super.dropStacks(world, x, y, z, meta, luck);
        if (world.isRemote) {
            return;
        }
        for (int i = 0; i < 3; ++i) {
            if (world.random.nextInt(15) > meta) continue;
            float f = 0.7f;
            float f2 = world.random.nextFloat() * f + (1.0f - f) * 0.5f;
            float f3 = world.random.nextFloat() * f + (1.0f - f) * 0.5f;
            float f4 = world.random.nextFloat() * f + (1.0f - f) * 0.5f;
            ItemEntity itemEntity = new ItemEntity(world, (float)x + f2, (float)y + f3, (float)z + f4, new ItemStack(Item.SEEDS));
            itemEntity.pickupDelay = 10;
            world.spawnEntity(itemEntity);
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        if (blockMeta == 7) {
            return Item.WHEAT.id;
        }
        return -1;
    }

    public int getDroppedItemCount(Random random) {
        return 1;
    }
}

