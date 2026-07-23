/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.JukeboxBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class JukeboxBlock
extends BlockWithEntity {
    protected JukeboxBlock(int id, int textureId) {
        super(id, textureId, Material.WOOD);
    }

    public int getTexture(int side) {
        return this.textureId + (side == 1 ? 1 : 0);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.getBlockMeta(x, y, z) == 0) {
            return false;
        }
        this.tryEjectRecord(world, x, y, z);
        return true;
    }

    public void insertRecord(World world, int x, int y, int z, int id) {
        if (world.isRemote) {
            return;
        }
        JukeboxBlockEntity jukeboxBlockEntity = (JukeboxBlockEntity)world.getBlockEntity(x, y, z);
        jukeboxBlockEntity.recordId = id;
        jukeboxBlockEntity.markDirty();
        world.setBlockMeta(x, y, z, 1);
    }

    public void tryEjectRecord(World world, int x, int y, int z) {
        if (world.isRemote) {
            return;
        }
        JukeboxBlockEntity jukeboxBlockEntity = (JukeboxBlockEntity)world.getBlockEntity(x, y, z);
        int n = jukeboxBlockEntity.recordId;
        if (n == 0) {
            return;
        }
        world.worldEvent(1005, x, y, z, 0);
        world.playStreaming(null, x, y, z);
        jukeboxBlockEntity.recordId = 0;
        jukeboxBlockEntity.markDirty();
        world.setBlockMeta(x, y, z, 0);
        int n2 = n;
        float f = 0.7f;
        double d = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        double d2 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.2 + 0.6;
        double d3 = (double)(world.random.nextFloat() * f) + (double)(1.0f - f) * 0.5;
        ItemEntity itemEntity = new ItemEntity(world, (double)x + d, (double)y + d2, (double)z + d3, new ItemStack(n2, 1, 0));
        itemEntity.pickupDelay = 10;
        world.spawnEntity(itemEntity);
    }

    public void onBreak(World world, int x, int y, int z) {
        this.tryEjectRecord(world, x, y, z);
        super.onBreak(world, x, y, z);
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        if (world.isRemote) {
            return;
        }
        super.dropStacks(world, x, y, z, meta, luck);
    }

    protected BlockEntity createBlockEntity() {
        return new JukeboxBlockEntity();
    }
}

