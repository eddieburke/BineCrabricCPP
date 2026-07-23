/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.material;

import net.minecraft.block.MapColor;
import net.minecraft.block.material.Material;

public class ReplaceableMaterial
extends Material {
    public ReplaceableMaterial(MapColor arg) {
        super(arg);
    }

    public boolean isSolid() {
        return false;
    }

    public boolean blocksVision() {
        return false;
    }

    public boolean blocksMovement() {
        return false;
    }
}

