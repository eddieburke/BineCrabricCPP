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
import net.minecraft.block.PlantBlock;
import net.minecraft.client.color.world.GrassColors;
import net.minecraft.item.Item;
import net.minecraft.world.BlockView;

public class TallPlantBlock
extends PlantBlock {
    protected TallPlantBlock(int i, int j) {
        super(i, j);
        float f = 0.4f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }

    public int getTexture(int side, int meta) {
        if (meta == 1) {
            return this.textureId;
        }
        if (meta == 2) {
            return this.textureId + 16 + 1;
        }
        if (meta == 0) {
            return this.textureId + 16;
        }
        return this.textureId;
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        if (n == 0) {
            return 0xFFFFFF;
        }
        long l = x * 3129871 + z * 6129781 + y;
        l = l * l * 42317861L + l * 11L;
        x = (int)((long)x + (l >> 14 & 0x1FL));
        y = (int)((long)y + (l >> 19 & 0x1FL));
        z = (int)((long)z + (l >> 24 & 0x1FL));
        blockView.getBiomeSource().getBiomesInArea(x, z, 1, 1);
        double d = blockView.getBiomeSource().temperatureMap[0];
        double d2 = blockView.getBiomeSource().downfallMap[0];
        return GrassColors.getColor(d, d2);
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        if (random.nextInt(8) == 0) {
            return Item.SEEDS.id;
        }
        return -1;
    }
}

