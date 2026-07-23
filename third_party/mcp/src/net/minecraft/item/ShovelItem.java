/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.item.ToolItem;
import net.minecraft.item.ToolMaterial;

public class ShovelItem
extends ToolItem {
    private static Block[] shovelEffectiveBlocks = new Block[]{Block.GRASS_BLOCK, Block.DIRT, Block.SAND, Block.GRAVEL, Block.SNOW, Block.SNOW_BLOCK, Block.CLAY, Block.FARMLAND};

    public ShovelItem(int id, ToolMaterial toolMaterial) {
        super(id, 1, toolMaterial, shovelEffectiveBlocks);
    }

    public boolean isSuitableFor(Block block) {
        if (block == Block.SNOW) {
            return true;
        }
        return block == Block.SNOW_BLOCK;
    }
}

