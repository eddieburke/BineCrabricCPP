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
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.item.Item;
import net.minecraft.util.math.Box;
import net.minecraft.world.World;

public class CobwebBlock
extends Block {
    public CobwebBlock(int id, int texturePosition) {
        super(id, texturePosition, Material.COBWEB);
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        entity.slowed = true;
    }

    public boolean isOpaque() {
        return false;
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 1;
    }

    public boolean isFullCube() {
        return false;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.STRING.id;
    }
}

