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
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class FenceBlock
extends Block {
    public FenceBlock(int id, int textureId) {
        super(id, textureId, Material.WOOD);
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (world.getBlockId(x, y - 1, z) == this.id) {
            return true;
        }
        if (!world.getMaterial(x, y - 1, z).isSolid()) {
            return false;
        }
        return super.canPlaceAt(world, x, y, z);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return Box.createCached(x, y, z, x + 1, (float)y + 1.5f, z + 1);
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 11;
    }
}

