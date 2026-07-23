/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.SandBlock;
import net.minecraft.item.Item;

public class GravelBlock
extends SandBlock {
    public GravelBlock(int i, int j) {
        super(i, j);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        if (random.nextInt(10) == 0) {
            return Item.FLINT.id;
        }
        return this.id;
    }
}

