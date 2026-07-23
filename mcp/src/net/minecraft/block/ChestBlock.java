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
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.ChestBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.DoubleInventory;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class ChestBlock
extends BlockWithEntity {
    private Random random = new Random();

    protected ChestBlock(int id) {
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
        if (n == this.id || n2 == this.id) {
            if (side == 2 || side == 3) {
                return this.textureId;
            }
            int n5 = 0;
            if (n == this.id) {
                n5 = -1;
            }
            int n6 = blockView.getBlockId(x - 1, y, n == this.id ? z - 1 : z + 1);
            int n7 = blockView.getBlockId(x + 1, y, n == this.id ? z - 1 : z + 1);
            if (side == 4) {
                n5 = -1 - n5;
            }
            int n8 = 5;
            if ((Block.BLOCKS_OPAQUE[n3] || Block.BLOCKS_OPAQUE[n6]) && !Block.BLOCKS_OPAQUE[n4] && !Block.BLOCKS_OPAQUE[n7]) {
                n8 = 5;
            }
            if ((Block.BLOCKS_OPAQUE[n4] || Block.BLOCKS_OPAQUE[n7]) && !Block.BLOCKS_OPAQUE[n3] && !Block.BLOCKS_OPAQUE[n6]) {
                n8 = 4;
            }
            return (side == n8 ? this.textureId + 16 : this.textureId + 32) + n5;
        }
        if (n3 == this.id || n4 == this.id) {
            if (side == 4 || side == 5) {
                return this.textureId;
            }
            int n9 = 0;
            if (n3 == this.id) {
                n9 = -1;
            }
            int n10 = blockView.getBlockId(n3 == this.id ? x - 1 : x + 1, y, z - 1);
            int n11 = blockView.getBlockId(n3 == this.id ? x - 1 : x + 1, y, z + 1);
            if (side == 3) {
                n9 = -1 - n9;
            }
            int n12 = 3;
            if ((Block.BLOCKS_OPAQUE[n] || Block.BLOCKS_OPAQUE[n10]) && !Block.BLOCKS_OPAQUE[n2] && !Block.BLOCKS_OPAQUE[n11]) {
                n12 = 3;
            }
            if ((Block.BLOCKS_OPAQUE[n2] || Block.BLOCKS_OPAQUE[n11]) && !Block.BLOCKS_OPAQUE[n] && !Block.BLOCKS_OPAQUE[n10]) {
                n12 = 2;
            }
            return (side == n12 ? this.textureId + 16 : this.textureId + 32) + n9;
        }
        int n13 = 3;
        if (Block.BLOCKS_OPAQUE[n] && !Block.BLOCKS_OPAQUE[n2]) {
            n13 = 3;
        }
        if (Block.BLOCKS_OPAQUE[n2] && !Block.BLOCKS_OPAQUE[n]) {
            n13 = 2;
        }
        if (Block.BLOCKS_OPAQUE[n3] && !Block.BLOCKS_OPAQUE[n4]) {
            n13 = 5;
        }
        if (Block.BLOCKS_OPAQUE[n4] && !Block.BLOCKS_OPAQUE[n3]) {
            n13 = 4;
        }
        return side == n13 ? this.textureId + 1 : this.textureId;
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
        int n = 0;
        if (world.getBlockId(x - 1, y, z) == this.id) {
            ++n;
        }
        if (world.getBlockId(x + 1, y, z) == this.id) {
            ++n;
        }
        if (world.getBlockId(x, y, z - 1) == this.id) {
            ++n;
        }
        if (world.getBlockId(x, y, z + 1) == this.id) {
            ++n;
        }
        if (n > 1) {
            return false;
        }
        if (this.hasNeighbor(world, x - 1, y, z)) {
            return false;
        }
        if (this.hasNeighbor(world, x + 1, y, z)) {
            return false;
        }
        if (this.hasNeighbor(world, x, y, z - 1)) {
            return false;
        }
        return !this.hasNeighbor(world, x, y, z + 1);
    }

    private boolean hasNeighbor(World world, int x, int y, int z) {
        if (world.getBlockId(x, y, z) != this.id) {
            return false;
        }
        if (world.getBlockId(x - 1, y, z) == this.id) {
            return true;
        }
        if (world.getBlockId(x + 1, y, z) == this.id) {
            return true;
        }
        if (world.getBlockId(x, y, z - 1) == this.id) {
            return true;
        }
        return world.getBlockId(x, y, z + 1) == this.id;
    }

    public void onBreak(World world, int x, int y, int z) {
        ChestBlockEntity chestBlockEntity = (ChestBlockEntity)world.getBlockEntity(x, y, z);
        for (int i = 0; i < chestBlockEntity.size(); ++i) {
            ItemStack itemStack = chestBlockEntity.getStack(i);
            if (itemStack == null) continue;
            float f = this.random.nextFloat() * 0.8f + 0.1f;
            float f2 = this.random.nextFloat() * 0.8f + 0.1f;
            float f3 = this.random.nextFloat() * 0.8f + 0.1f;
            while (itemStack.count > 0) {
                int n = this.random.nextInt(21) + 10;
                if (n > itemStack.count) {
                    n = itemStack.count;
                }
                itemStack.count -= n;
                ItemEntity itemEntity = new ItemEntity(world, (float)x + f, (float)y + f2, (float)z + f3, new ItemStack(itemStack.itemId, n, itemStack.getDamage()));
                float f4 = 0.05f;
                itemEntity.velocityX = (float)this.random.nextGaussian() * f4;
                itemEntity.velocityY = (float)this.random.nextGaussian() * f4 + 0.2f;
                itemEntity.velocityZ = (float)this.random.nextGaussian() * f4;
                world.spawnEntity(itemEntity);
            }
        }
        super.onBreak(world, x, y, z);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        Inventory inventory = (ChestBlockEntity)world.getBlockEntity(x, y, z);
        if (world.shouldSuffocate(x, y + 1, z)) {
            return true;
        }
        if (world.getBlockId(x - 1, y, z) == this.id && world.shouldSuffocate(x - 1, y + 1, z)) {
            return true;
        }
        if (world.getBlockId(x + 1, y, z) == this.id && world.shouldSuffocate(x + 1, y + 1, z)) {
            return true;
        }
        if (world.getBlockId(x, y, z - 1) == this.id && world.shouldSuffocate(x, y + 1, z - 1)) {
            return true;
        }
        if (world.getBlockId(x, y, z + 1) == this.id && world.shouldSuffocate(x, y + 1, z + 1)) {
            return true;
        }
        if (world.getBlockId(x - 1, y, z) == this.id) {
            inventory = new DoubleInventory("Large chest", (ChestBlockEntity)world.getBlockEntity(x - 1, y, z), inventory);
        }
        if (world.getBlockId(x + 1, y, z) == this.id) {
            inventory = new DoubleInventory("Large chest", inventory, (ChestBlockEntity)world.getBlockEntity(x + 1, y, z));
        }
        if (world.getBlockId(x, y, z - 1) == this.id) {
            inventory = new DoubleInventory("Large chest", (ChestBlockEntity)world.getBlockEntity(x, y, z - 1), inventory);
        }
        if (world.getBlockId(x, y, z + 1) == this.id) {
            inventory = new DoubleInventory("Large chest", inventory, (ChestBlockEntity)world.getBlockEntity(x, y, z + 1));
        }
        if (world.isRemote) {
            return true;
        }
        player.openChestScreen(inventory);
        return true;
    }

    protected BlockEntity createBlockEntity() {
        return new ChestBlockEntity();
    }
}

