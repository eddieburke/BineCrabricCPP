/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.passive.PigEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

public class SaddleItem
extends Item {
    public SaddleItem(int i) {
        super(i);
        this.maxCount = 1;
    }

    public void useOnEntity(ItemStack stack, LivingEntity entity) {
        PigEntity pigEntity;
        if (entity instanceof PigEntity && !(pigEntity = (PigEntity)entity).isSaddled()) {
            pigEntity.setSaddled(true);
            --stack.count;
        }
    }

    public boolean postHit(ItemStack stack, LivingEntity target, LivingEntity attacker) {
        this.useOnEntity(stack, target);
        return true;
    }
}

