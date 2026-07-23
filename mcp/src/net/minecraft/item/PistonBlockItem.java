/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.item.BlockItem;

public class PistonBlockItem
extends BlockItem {
    public PistonBlockItem(int i) {
        super(i);
    }

    public int getPlacementMetadata(int meta) {
        return 7;
    }
}

