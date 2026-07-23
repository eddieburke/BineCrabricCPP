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
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

public class CoalItem
extends Item {
    public CoalItem(int i) {
        super(i);
        this.setHasSubtypes(true);
        this.setMaxDamage(0);
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey(ItemStack stack) {
        if (stack.getDamage() == 1) {
            return "item.charcoal";
        }
        return "item.coal";
    }
}

