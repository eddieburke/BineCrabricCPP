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
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.ToolMaterial;

public class SwordItem
extends Item {
    private int damage;

    public SwordItem(int id, ToolMaterial toolMaterial) {
        super(id);
        this.maxCount = 1;
        this.setMaxDamage(toolMaterial.getDurability());
        this.damage = 4 + toolMaterial.getAttackDamage() * 2;
    }

    public float getMiningSpeedMultiplier(ItemStack stack, Block block) {
        if (block.id == Block.COBWEB.id) {
            return 15.0f;
        }
        return 1.5f;
    }

    public boolean postHit(ItemStack stack, LivingEntity target, LivingEntity attacker) {
        stack.damage(1, attacker);
        return true;
    }

    public boolean postMine(ItemStack stack, int blockId, int x, int y, int z, LivingEntity miner) {
        stack.damage(2, miner);
        return true;
    }

    public int getAttackDamage(Entity attackedEntity) {
        return this.damage;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isHandheld() {
        return true;
    }

    public boolean isSuitableFor(Block block) {
        return block.id == Block.COBWEB.id;
    }
}

