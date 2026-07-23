/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.material;

import net.minecraft.block.MapColor;
import net.minecraft.block.material.Material;

public class FluidMaterial
extends Material {
    public FluidMaterial(MapColor arg) {
        super(arg);
        this.setReplaceable();
        this.setDestroyPistonBehavior();
    }

    public boolean isFluid() {
        return true;
    }

    public boolean blocksMovement() {
        return false;
    }

    public boolean isSolid() {
        return false;
    }
}

