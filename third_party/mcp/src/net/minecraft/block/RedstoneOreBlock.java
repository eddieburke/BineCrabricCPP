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
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.world.World;

public class RedstoneOreBlock
extends Block {
    private boolean lit;

    public RedstoneOreBlock(int id, int textureId, boolean lit) {
        super(id, textureId, Material.STONE);
        if (lit) {
            this.setTickRandomly(true);
        }
        this.lit = lit;
    }

    public int getTickRate() {
        return 30;
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.light(world, x, y, z);
        super.onBlockBreakStart(world, x, y, z, player);
    }

    public void onSteppedOn(World world, int x, int y, int z, Entity entity) {
        this.light(world, x, y, z);
        super.onSteppedOn(world, x, y, z, entity);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        this.light(world, x, y, z);
        return super.onUse(world, x, y, z, player);
    }

    private void light(World world, int x, int y, int z) {
        this.spawnParticles(world, x, y, z);
        if (this.id == Block.REDSTONE_ORE.id) {
            world.setBlock(x, y, z, Block.LIT_REDSTONE_ORE.id);
        }
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (this.id == Block.LIT_REDSTONE_ORE.id) {
            world.setBlock(x, y, z, Block.REDSTONE_ORE.id);
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.REDSTONE.id;
    }

    public int getDroppedItemCount(Random random) {
        return 4 + random.nextInt(2);
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        if (this.lit) {
            this.spawnParticles(world, x, y, z);
        }
    }

    private void spawnParticles(World world, int x, int y, int z) {
        Random random = world.random;
        double d = 0.0625;
        for (int i = 0; i < 6; ++i) {
            double d2 = (float)x + random.nextFloat();
            double d3 = (float)y + random.nextFloat();
            double d4 = (float)z + random.nextFloat();
            if (i == 0 && !world.isBlockOpaqueCube(x, y + 1, z)) {
                d3 = (double)(y + 1) + d;
            }
            if (i == 1 && !world.isBlockOpaqueCube(x, y - 1, z)) {
                d3 = (double)(y + 0) - d;
            }
            if (i == 2 && !world.isBlockOpaqueCube(x, y, z + 1)) {
                d4 = (double)(z + 1) + d;
            }
            if (i == 3 && !world.isBlockOpaqueCube(x, y, z - 1)) {
                d4 = (double)(z + 0) - d;
            }
            if (i == 4 && !world.isBlockOpaqueCube(x + 1, y, z)) {
                d2 = (double)(x + 1) + d;
            }
            if (i == 5 && !world.isBlockOpaqueCube(x - 1, y, z)) {
                d2 = (double)(x + 0) - d;
            }
            if (!(d2 < (double)x || d2 > (double)(x + 1) || d3 < 0.0 || d3 > (double)(y + 1) || d4 < (double)z) && !(d4 > (double)(z + 1))) continue;
            world.addParticle("reddust", d2, d3, d4, 0.0, 0.0, 0.0);
        }
    }
}

