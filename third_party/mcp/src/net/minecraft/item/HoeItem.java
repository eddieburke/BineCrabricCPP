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
import net.minecraft.item.ToolMaterial;
import net.minecraft.world.World;

public class HoeItem
extends Item {
    public HoeItem(int id, ToolMaterial toolMaterial) {
        super(id);
        this.maxCount = 1;
        this.setMaxDamage(toolMaterial.getDurability());
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        int n = world.getBlockId(x, y, z);
        int n2 = world.getBlockId(x, y + 1, z);
        if (side != 0 && n2 == 0 && n == Block.GRASS_BLOCK.id || n == Block.DIRT.id) {
            Block block = Block.FARMLAND;
            world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, block.soundGroup.getSound(), (block.soundGroup.getVolume() + 1.0f) / 2.0f, block.soundGroup.getPitch() * 0.8f);
            if (world.isRemote) {
                return true;
            }
            world.setBlock(x, y, z, block.id);
            stack.damage(1, user);
            return true;
        }
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheld() {
        return true;
    }
}

