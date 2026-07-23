/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.TranslucentBlock;
import net.minecraft.block.material.Material;

public class GlassBlock
extends TranslucentBlock {
    public GlassBlock(int i, int j, Material arg, boolean bl) {
        super(i, j, arg, bl);
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return 0;
    }
}

