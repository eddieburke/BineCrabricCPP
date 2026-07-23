/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

public class ShearsItem
extends Item {
    public ShearsItem(int i) {
        super(i);
        this.setMaxCount(1);
        this.setMaxDamage(238);
    }

    public boolean postMine(ItemStack stack, int blockId, int x, int y, int z, LivingEntity miner) {
        if (blockId == Block.LEAVES.id || blockId == Block.COBWEB.id) {
            stack.damage(1, miner);
        }
        return super.postMine(stack, blockId, x, y, z, miner);
    }

    public boolean isSuitableFor(Block block) {
        return block.id == Block.COBWEB.id;
    }

    public float getMiningSpeedMultiplier(ItemStack stack, Block block) {
        if (block.id == Block.COBWEB.id || block.id == Block.LEAVES.id) {
            return 15.0f;
        }
        if (block.id == Block.WOOL.id) {
            return 5.0f;
        }
        return super.getMiningSpeedMultiplier(stack, block);
    }
}

