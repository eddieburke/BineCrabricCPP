/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.PlantBlock;

public class DeadBushBlock
extends PlantBlock {
    protected DeadBushBlock(int i, int j) {
        super(i, j);
        float f = 0.4f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }

    protected boolean canPlantOnTop(int id) {
        return id == Block.SAND.id;
    }

    public int getTexture(int side, int meta) {
        return this.textureId;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return -1;
    }
}

