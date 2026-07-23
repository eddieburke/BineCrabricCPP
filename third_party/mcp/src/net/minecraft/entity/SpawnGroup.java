/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity;

import net.minecraft.block.material.Material;
import net.minecraft.entity.Monster;
import net.minecraft.entity.WaterCreatureEntity;
import net.minecraft.entity.passive.AnimalEntity;

public enum SpawnGroup {
    MONSTER(Monster.class, 70, Material.AIR, false),
    CREATURE(AnimalEntity.class, 15, Material.AIR, true),
    WATER_CREATURE(WaterCreatureEntity.class, 5, Material.WATER, true);

    private final Class creatureClass;
    private final int capacity;
    private final Material spawnMaterial;
    private final boolean peaceful;

    /*
     * WARNING - Possible parameter corruption
     * WARNING - void declaration
     */
    private SpawnGroup(Material creatureClass, boolean spawnCap) {
        void peaceful;
        void spawnMaterial;
        this.creatureClass = creatureClass;
        this.capacity = spawnCap ? 1 : 0;
        this.spawnMaterial = spawnMaterial;
        this.peaceful = peaceful;
    }

    public Class getCreatureClass() {
        return this.creatureClass;
    }

    public int getCapacity() {
        return this.capacity;
    }

    public Material getSpawnMaterial() {
        return this.spawnMaterial;
    }

    public boolean isPeaceful() {
        return this.peaceful;
    }
}

