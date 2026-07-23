/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class BlockItem
extends Item {
    private int itemId;

    public BlockItem(int i) {
        super(i);
        this.itemId = i + 256;
        this.setTextureId(Block.BLOCKS[i + 256].getTexture(2));
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
        if (y == 127 && Block.BLOCKS[this.itemId].material.isSolid()) {
            return false;
        }
        if (world.canPlace(this.itemId, x, y, z, false, side)) {
            Block block = Block.BLOCKS[this.itemId];
            if (world.setBlock(x, y, z, this.itemId, this.getPlacementMetadata(stack.getDamage()))) {
                Block.BLOCKS[this.itemId].onPlaced(world, x, y, z, side);
                Block.BLOCKS[this.itemId].onPlaced(world, x, y, z, user);
                world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, block.soundGroup.getSound(), (block.soundGroup.getVolume() + 1.0f) / 2.0f, block.soundGroup.getPitch() * 0.8f);
                --stack.count;
            }
            return true;
        }
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey(ItemStack stack) {
        return Block.BLOCKS[this.itemId].getTranslationKey();
    }

    public String getTranslationKey() {
        return Block.BLOCKS[this.itemId].getTranslationKey();
    }
}

