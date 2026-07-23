/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.biome;

import net.minecraft.entity.mob.GhastEntity;
import net.minecraft.entity.mob.PigZombieEntity;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.EntitySpawnGroup;

public class HellBiome
extends Biome {
    public HellBiome() {
        this.spawnableMonsters.clear();
        this.spawnablePassive.clear();
        this.spawnableWaterCreatures.clear();
        this.spawnableMonsters.add(new EntitySpawnGroup(GhastEntity.class, 10));
        this.spawnableMonsters.add(new EntitySpawnGroup(PigZombieEntity.class, 10));
    }
}

