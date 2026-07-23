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
import net.minecraft.block.SlabBlock;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemStack;

public class SlabBlockItem
extends BlockItem {
    public SlabBlockItem(int i) {
        super(i);
        this.setMaxDamage(0);
        this.setHasSubtypes(true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(int damage) {
        return Block.SLAB.getTexture(2, damage);
    }

    public int getPlacementMetadata(int meta) {
        return meta;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey(ItemStack stack) {
        return super.getTranslationKey() + "." + SlabBlock.names[stack.getDamage()];
    }
}

