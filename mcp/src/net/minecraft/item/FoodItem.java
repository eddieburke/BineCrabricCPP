/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class FoodItem
extends Item {
    private int healthRestored;
    private boolean meat;

    public FoodItem(int id, int healthRestored, boolean meat) {
        super(id);
        this.healthRestored = healthRestored;
        this.meat = meat;
        this.maxCount = 1;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        --stack.count;
        user.heal(this.healthRestored);
        return stack;
    }

    public int getHealthRestored() {
        return this.healthRestored;
    }

    public boolean isMeat() {
        return this.meat;
    }
}

