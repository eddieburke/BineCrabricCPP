/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.item.FoodItem;

public class StackableFoodItem
extends FoodItem {
    public StackableFoodItem(int id, int healthRestored, boolean meat, int maxCount) {
        super(id, healthRestored, meat);
        this.maxCount = maxCount;
    }
}

