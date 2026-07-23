/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.world.World;

public class WorkbenchBlock
extends Block {
    protected WorkbenchBlock(int id) {
        super(id, Material.WOOD);
        this.textureId = 59;
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId - 16;
        }
        if (side == 0) {
            return Block.PLANKS.getTexture(0);
        }
        if (side == 2 || side == 4) {
            return this.textureId + 1;
        }
        return this.textureId;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return true;
        }
        player.openCraftingScreen(x, y, z);
        return true;
    }
}

