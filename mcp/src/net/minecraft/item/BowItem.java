/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class BowItem
extends Item {
    public BowItem(int i) {
        super(i);
        this.maxCount = 1;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        if (user.inventory.remove(Item.ARROW.id)) {
            world.playSound(user, "random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 0.8f));
            if (!world.isRemote) {
                world.spawnEntity(new ArrowEntity(world, user));
            }
        }
        return stack;
    }
}

