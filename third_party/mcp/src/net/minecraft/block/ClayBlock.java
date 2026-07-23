/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.item.Item;

public class ClayBlock
extends Block {
    public ClayBlock(int id, int textureId) {
        super(id, textureId, Material.CLAY);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.CLAY.id;
    }

    public int getDroppedItemCount(Random random) {
        return 4;
    }
}

