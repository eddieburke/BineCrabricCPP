/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.biome;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.passive.ChickenEntity;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.EntitySpawnGroup;

public class SkyBiome
extends Biome {
    public SkyBiome() {
        this.spawnableMonsters.clear();
        this.spawnablePassive.clear();
        this.spawnableWaterCreatures.clear();
        this.spawnablePassive.add(new EntitySpawnGroup(ChickenEntity.class, 10));
    }

    @Environment(value=EnvType.CLIENT)
    public int getSkyColor(float f) {
        return 0xC0C0FF;
    }
}

