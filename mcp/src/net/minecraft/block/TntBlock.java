/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class TntBlock
extends Block {
    public TntBlock(int id, int textureId) {
        super(id, textureId, Material.TNT);
    }

    public int getTexture(int side) {
        if (side == 0) {
            return this.textureId + 2;
        }
        if (side == 1) {
            return this.textureId + 1;
        }
        return this.textureId;
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
        if (world.isEmittingRedstonePower(x, y, z)) {
            this.onMetadataChange(world, x, y, z, 1);
            world.setBlock(x, y, z, 0);
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower() && world.isEmittingRedstonePower(x, y, z)) {
            this.onMetadataChange(world, x, y, z, 1);
            world.setBlock(x, y, z, 0);
        }
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public void onDestroyedByExplosion(World world, int x, int y, int z) {
        TntEntity tntEntity = new TntEntity(world, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
        tntEntity.fuse = world.random.nextInt(tntEntity.fuse / 4) + tntEntity.fuse / 8;
        world.spawnEntity(tntEntity);
    }

    public void onMetadataChange(World world, int x, int y, int z, int meta) {
        if (world.isRemote) {
            return;
        }
        if ((meta & 1) == 0) {
            this.dropStack(world, x, y, z, new ItemStack(Block.TNT.id, 1, 0));
        } else {
            TntEntity tntEntity = new TntEntity(world, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
            world.spawnEntity(tntEntity);
            world.playSound(tntEntity, "random.fuse", 1.0f, 1.0f);
        }
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        if (player.getHand() != null && player.getHand().itemId == Item.FLINT_AND_STEEL.id) {
            world.setBlockMetaWithoutNotifyingNeighbors(x, y, z, 1);
        }
        super.onBlockBreakStart(world, x, y, z, player);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        return super.onUse(world, x, y, z, player);
    }
}

