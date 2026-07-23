/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.biome;

import java.util.Random;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.LargeOakTreeFeature;
import net.minecraft.world.gen.feature.OakTreeFeature;

public class RainforestBiome
extends Biome {
    public Feature getRandomTreeFeature(Random random) {
        if (random.nextInt(3) == 0) {
            return new LargeOakTreeFeature();
        }
        return new OakTreeFeature();
    }
}

