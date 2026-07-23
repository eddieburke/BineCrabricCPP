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
import net.minecraft.client.color.world.FoliageColors;
import net.minecraft.item.BlockItem;

public class LeavesBlockItem
extends BlockItem {
    public LeavesBlockItem(int i) {
        super(i);
        this.setMaxDamage(0);
        this.setHasSubtypes(true);
    }

    public int getPlacementMetadata(int meta) {
        return meta | 8;
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(int damage) {
        return Block.LEAVES.getTexture(0, damage);
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(int color) {
        if ((color & 1) == 1) {
            return FoliageColors.getSpruceColor();
        }
        if ((color & 2) == 2) {
            return FoliageColors.getBirchColor();
        }
        return FoliageColors.getDefaultColor();
    }
}

