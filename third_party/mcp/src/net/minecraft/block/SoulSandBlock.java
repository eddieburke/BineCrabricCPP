/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class SoulSandBlock
extends Block {
    public SoulSandBlock(int id, int textureId) {
        super(id, textureId, Material.SAND);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        float f = 0.125f;
        return Box.createCached(x, y, z, x + 1, (float)(y + 1) - f, z + 1);
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        entity.velocityX *= 0.4;
        entity.velocityZ *= 0.4;
    }
}

