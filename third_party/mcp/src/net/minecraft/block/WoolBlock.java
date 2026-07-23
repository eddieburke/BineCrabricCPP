/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;

public class WoolBlock
extends Block {
    public WoolBlock() {
        super(35, 64, Material.WOOL);
    }

    public int getTexture(int side, int meta) {
        if (meta == 0) {
            return this.textureId;
        }
        meta = ~(meta & 0xF);
        return 113 + ((meta & 8) >> 3) + (meta & 7) * 16;
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return blockMeta;
    }

    public static int getBlockMeta(int itemMeta) {
        return ~itemMeta & 0xF;
    }

    public static int getItemMeta(int blockMeta) {
        return ~blockMeta & 0xF;
    }
}

