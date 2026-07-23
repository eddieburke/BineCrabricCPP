/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.FoodItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class MushroomStewItem
extends FoodItem {
    public MushroomStewItem(int id, int healthRestored) {
        super(id, healthRestored, false);
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        super.use(stack, world, user);
        return new ItemStack(Item.BOWL);
    }
}

