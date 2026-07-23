/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.material;

import net.minecraft.block.MapColor;
import net.minecraft.block.material.Material;

public class AirMaterial
extends Material {
    public AirMaterial(MapColor arg) {
        super(arg);
        this.setReplaceable();
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

