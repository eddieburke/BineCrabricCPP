/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class DoorItem
extends Item {
    private Material material;

    public DoorItem(int id, Material material) {
        super(id);
        this.material = material;
        this.maxCount = 1;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (side != 1) {
            return false;
        }
        Block block = this.material == Material.WOOD ? Block.DOOR : Block.IRON_DOOR;
        if (!block.canPlaceAt(world, x, ++y, z)) {
            return false;
        }
        int n = MathHelper.floor((double)((user.yaw + 180.0f) * 4.0f / 360.0f) - 0.5) & 3;
        int n2 = 0;
        int n3 = 0;
        if (n == 0) {
            n3 = 1;
        }
        if (n == 1) {
            n2 = -1;
        }
        if (n == 2) {
            n3 = -1;
        }
        if (n == 3) {
            n2 = 1;
        }
        int n4 = (world.shouldSuffocate(x - n2, y, z - n3) ? 1 : 0) + (world.shouldSuffocate(x - n2, y + 1, z - n3) ? 1 : 0);
        int n5 = (world.shouldSuffocate(x + n2, y, z + n3) ? 1 : 0) + (world.shouldSuffocate(x + n2, y + 1, z + n3) ? 1 : 0);
        boolean bl = world.getBlockId(x - n2, y, z - n3) == block.id || world.getBlockId(x - n2, y + 1, z - n3) == block.id;
        boolean bl2 = world.getBlockId(x + n2, y, z + n3) == block.id || world.getBlockId(x + n2, y + 1, z + n3) == block.id;
        boolean bl3 = false;
        if (bl && !bl2) {
            bl3 = true;
        } else if (n5 > n4) {
            bl3 = true;
        }
        if (bl3) {
            n = n - 1 & 3;
            n += 4;
        }
        world.pauseTicking = true;
        world.setBlock(x, y, z, block.id, n);
        world.setBlock(x, y + 1, z, block.id, n + 8);
        world.pauseTicking = false;
        world.notifyNeighbors(x, y, z, block.id);
        world.notifyNeighbors(x, y + 1, z, block.id);
        --stack.count;
        return true;
    }
}

