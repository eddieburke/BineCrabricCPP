/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class EggItem
extends Item {
    public EggItem(int i) {
        super(i);
        this.maxCount = 16;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        --stack.count;
        world.playSound(user, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
        if (!world.isRemote) {
            world.spawnEntity(new EggEntity(world, user));
        }
        return stack;
    }
}

