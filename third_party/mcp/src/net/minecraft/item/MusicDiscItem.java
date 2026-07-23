/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.block.JukeboxBlock;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class MusicDiscItem
extends Item {
    public final String sound;

    protected MusicDiscItem(int id, String sound) {
        super(id);
        this.sound = sound;
        this.maxCount = 1;
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (world.getBlockId(x, y, z) == Block.JUKEBOX.id && world.getBlockMeta(x, y, z) == 0) {
            if (world.isRemote) {
                return true;
            }
            ((JukeboxBlock)Block.JUKEBOX).insertRecord(world, x, y, z, this.id);
            world.worldEvent(null, 1005, x, y, z, this.id);
            --stack.count;
            return true;
        }
        return false;
    }
}

