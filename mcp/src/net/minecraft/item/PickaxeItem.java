/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.item.ToolItem;
import net.minecraft.item.ToolMaterial;

public class PickaxeItem
extends ToolItem {
    private static Block[] pickaxeEffectiveBlocks = new Block[]{Block.COBBLESTONE, Block.DOUBLE_SLAB, Block.SLAB, Block.STONE, Block.SANDSTONE, Block.MOSSY_COBBLESTONE, Block.IRON_ORE, Block.IRON_BLOCK, Block.COAL_ORE, Block.GOLD_BLOCK, Block.GOLD_ORE, Block.DIAMOND_ORE, Block.DIAMOND_BLOCK, Block.ICE, Block.NETHERRACK, Block.LAPIS_ORE, Block.LAPIS_BLOCK};

    protected PickaxeItem(int id, ToolMaterial toolMaterial) {
        super(id, 2, toolMaterial, pickaxeEffectiveBlocks);
    }

    public boolean isSuitableFor(Block block) {
        if (block == Block.OBSIDIAN) {
            return this.toolMaterial.getMiningLevel() == 3;
        }
        if (block == Block.DIAMOND_BLOCK || block == Block.DIAMOND_ORE) {
            return this.toolMaterial.getMiningLevel() >= 2;
        }
        if (block == Block.GOLD_BLOCK || block == Block.GOLD_ORE) {
            return this.toolMaterial.getMiningLevel() >= 2;
        }
        if (block == Block.IRON_BLOCK || block == Block.IRON_ORE) {
            return this.toolMaterial.getMiningLevel() >= 1;
        }
        if (block == Block.LAPIS_BLOCK || block == Block.LAPIS_ORE) {
            return this.toolMaterial.getMiningLevel() >= 1;
        }
        if (block == Block.REDSTONE_ORE || block == Block.LIT_REDSTONE_ORE) {
            return this.toolMaterial.getMiningLevel() >= 2;
        }
        if (block.material == Material.STONE) {
            return true;
        }
        return block.material == Material.METAL;
    }
}

