/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.biome;

import java.util.Random;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.EntitySpawnGroup;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.PineTreeFeature;
import net.minecraft.world.gen.feature.SpruceTreeFeature;

public class TaigaBiome
extends Biome {
    public TaigaBiome() {
        this.spawnablePassive.add(new EntitySpawnGroup(WolfEntity.class, 2));
    }

    public Feature getRandomTreeFeature(Random random) {
        if (random.nextInt(3) == 0) {
            return new PineTreeFeature();
        }
        return new SpruceTreeFeature();
    }
}

