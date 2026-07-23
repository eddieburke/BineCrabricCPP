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
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.stat.Stats;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.LightType;
import net.minecraft.world.World;

public class SnowyBlock
extends Block {
    protected SnowyBlock(int id, int textureId) {
        super(id, textureId, Material.SNOW_LAYER);
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
        this.setTickRandomly(true);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z) & 7;
        if (n >= 3) {
            return Box.createCached((double)x + this.minX, (double)y + this.minY, (double)z + this.minZ, (double)x + this.maxX, (float)y + 0.5f, (double)z + this.maxZ);
        }
        return null;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z) & 7;
        float f = (float)(2 * (1 + n)) / 16.0f;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, f, 1.0f);
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y - 1, z);
        if (n == 0 || !Block.BLOCKS[n].isOpaque()) {
            return false;
        }
        return world.getMaterial(x, y - 1, z).blocksMovement();
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        this.breakIfCannotPlaceAt(world, x, y, z);
    }

    private boolean breakIfCannotPlaceAt(World world, int x, int y, int z) {
        if (!this.canPlaceAt(world, x, y, z)) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
            return false;
        }
        return true;
    }

    public void afterBreak(World world, PlayerEntity playerEntity, int x, int y, int z, int meta) {
        int n = Item.SNOWBALL.id;
        float f = 0.7f;
        double d = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        double d2 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        double d3 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        ItemEntity itemEntity = new ItemEntity(world, (double)x + d, (double)y + d2, (double)z + d3, new ItemStack(n, 1, 0));
        itemEntity.pickupDelay = 10;
        world.spawnEntity(itemEntity);
        world.setBlock(x, y, z, 0);
        playerEntity.increaseStat(Stats.MINE_BLOCK[this.id], 1);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.SNOWBALL.id;
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.getBrightness(LightType.BLOCK, x, y, z) > 11) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        if (side == 1) {
            return true;
        }
        return super.isSideVisible(blockView, x, y, z, side);
    }
}

