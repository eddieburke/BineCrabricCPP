/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.world.BlockView;

public class TransparentBlock
extends Block {
    protected boolean renderSides;

    protected TransparentBlock(int id, int textureId, Material material, boolean renderSides) {
        super(id, textureId, material);
        this.renderSides = renderSides;
    }

    public boolean isOpaque() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        int n = blockView.getBlockId(x, y, z);
        if (!this.renderSides && n == this.id) {
            return false;
        }
        return super.isSideVisible(blockView, x, y, z, side);
    }
}

