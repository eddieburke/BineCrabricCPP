/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;

public class OreStorageBlock
extends Block {
    public OreStorageBlock(int id, int textureId) {
        super(id, Material.METAL);
        this.textureId = textureId;
    }

    public int getTexture(int side) {
        return this.textureId;
    }
}

