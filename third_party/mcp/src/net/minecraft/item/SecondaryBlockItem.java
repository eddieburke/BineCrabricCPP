/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class SecondaryBlockItem
extends Item {
    private int blockId;

    public SecondaryBlockItem(int id, Block block) {
        super(id);
        this.blockId = block.id;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (world.getBlockId(x, y, z) == Block.SNOW.id) {
            side = 0;
        } else {
            if (side == 0) {
                --y;
            }
            if (side == 1) {
                ++y;
            }
            if (side == 2) {
                --z;
            }
            if (side == 3) {
                ++z;
            }
            if (side == 4) {
                --x;
            }
            if (side == 5) {
                ++x;
            }
        }
        if (stack.count == 0) {
            return false;
        }
        if (world.canPlace(this.blockId, x, y, z, false, side)) {
            Block block = Block.BLOCKS[this.blockId];
            if (world.setBlock(x, y, z, this.blockId)) {
                Block.BLOCKS[this.blockId].onPlaced(world, x, y, z, side);
                Block.BLOCKS[this.blockId].onPlaced(world, x, y, z, user);
                world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, block.soundGroup.getSound(), (block.soundGroup.getVolume() + 1.0f) / 2.0f, block.soundGroup.getPitch() * 0.8f);
                --stack.count;
            }
        }
        return true;
    }
}

