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
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class FishingRodItem
extends Item {
    public FishingRodItem(int i) {
        super(i);
        this.setMaxDamage(64);
        this.setMaxCount(1);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheld() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheldRod() {
        return true;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        if (user.fishHook != null) {
            int n = user.fishHook.use();
            stack.damage(n, user);
            user.swingHand();
        } else {
            world.playSound(user, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
            if (!world.isRemote) {
                world.spawnEntity(new FishingBobberEntity(world, user));
            }
            user.swingHand();
        }
        return stack;
    }
}

