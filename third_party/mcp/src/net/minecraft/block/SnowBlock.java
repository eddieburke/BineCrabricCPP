/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.item.Item;
import net.minecraft.world.LightType;
import net.minecraft.world.World;

public class SnowBlock
extends Block {
    protected SnowBlock(int id, int textureId) {
        super(id, textureId, Material.SNOW_BLOCK);
        this.setTickRandomly(true);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.SNOWBALL.id;
    }

    public int getDroppedItemCount(Random random) {
        return 4;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.getBrightness(LightType.BLOCK, x, y, z) > 11) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }
}

