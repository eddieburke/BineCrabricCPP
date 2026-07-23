/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.biome;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.SpawnGroup;
import net.minecraft.entity.mob.CreeperEntity;
import net.minecraft.entity.mob.SkeletonEntity;
import net.minecraft.entity.mob.SlimeEntity;
import net.minecraft.entity.mob.SpiderEntity;
import net.minecraft.entity.mob.ZombieEntity;
import net.minecraft.entity.passive.ChickenEntity;
import net.minecraft.entity.passive.CowEntity;
import net.minecraft.entity.passive.PigEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.passive.SquidEntity;
import net.minecraft.world.biome.BasicBiome;
import net.minecraft.world.biome.EntitySpawnGroup;
import net.minecraft.world.biome.ForestBiome;
import net.minecraft.world.biome.HellBiome;
import net.minecraft.world.biome.RainforestBiome;
import net.minecraft.world.biome.SkyBiome;
import net.minecraft.world.biome.SwamplandBiome;
import net.minecraft.world.biome.TaigaBiome;
import net.minecraft.world.gen.feature.Feature;
import net.minecraft.world.gen.feature.LargeOakTreeFeature;
import net.minecraft.world.gen.feature.OakTreeFeature;

public class Biome {
    public static final Biome RAINFOREST = new RainforestBiome().setGrassColor(588342).setName("Rainforest").setFoliageColor(2094168);
    public static final Biome SWAMPLAND = new SwamplandBiome().setGrassColor(522674).setName("Swampland").setFoliageColor(9154376);
    public static final Biome SEASONAL_FOREST = new Biome().setGrassColor(10215459).setName("Seasonal Forest");
    public static final Biome FOREST = new ForestBiome().setGrassColor(353825).setName("Forest").setFoliageColor(5159473);
    public static final Biome SAVANNA = new BasicBiome().setGrassColor(14278691).setName("Savanna");
    public static final Biome SHRUBLAND = new Biome().setGrassColor(10595616).setName("Shrubland");
    public static final Biome TAIGA = new TaigaBiome().setGrassColor(3060051).setName("Taiga").enableSnow().setFoliageColor(8107825);
    public static final Biome DESERT = new BasicBiome().setGrassColor(16421912).setName("Desert").disableRain();
    public static final Biome PLAINS = new BasicBiome().setGrassColor(16767248).setName("Plains");
    public static final Biome ICE_DESERT = new BasicBiome().setGrassColor(16772499).setName("Ice Desert").enableSnow().disableRain().setFoliageColor(12899129);
    public static final Biome TUNDRA = new Biome().setGrassColor(5762041).setName("Tundra").enableSnow().setFoliageColor(12899129);
    public static final Biome HELL = new HellBiome().setGrassColor(0xFF0000).setName("Hell").disableRain();
    public static final Biome SKY = new SkyBiome().setGrassColor(0x8080FF).setName("Sky").disableRain();
    public String name;
    public int grassColor;
    public byte topBlockId;
    public byte soilBlockId;
    public int foliageColor;
    protected List spawnableMonsters;
    protected List spawnablePassive;
    protected List spawnableWaterCreatures;
    private boolean hasSnow;
    private boolean hasRain;
    private static Biome[] BIOMES = new Biome[4096];

    protected Biome() {
        this.topBlockId = (byte)Block.GRASS_BLOCK.id;
        this.soilBlockId = (byte)Block.DIRT.id;
        this.foliageColor = 5169201;
        this.spawnableMonsters = new ArrayList();
        this.spawnablePassive = new ArrayList();
        this.spawnableWaterCreatures = new ArrayList();
        this.hasRain = true;
        this.spawnableMonsters.add(new EntitySpawnGroup(SpiderEntity.class, 10));
        this.spawnableMonsters.add(new EntitySpawnGroup(ZombieEntity.class, 10));
        this.spawnableMonsters.add(new EntitySpawnGroup(SkeletonEntity.class, 10));
        this.spawnableMonsters.add(new EntitySpawnGroup(CreeperEntity.class, 10));
        this.spawnableMonsters.add(new EntitySpawnGroup(SlimeEntity.class, 10));
        this.spawnablePassive.add(new EntitySpawnGroup(SheepEntity.class, 12));
        this.spawnablePassive.add(new EntitySpawnGroup(PigEntity.class, 10));
        this.spawnablePassive.add(new EntitySpawnGroup(ChickenEntity.class, 10));
        this.spawnablePassive.add(new EntitySpawnGroup(CowEntity.class, 8));
        this.spawnableWaterCreatures.add(new EntitySpawnGroup(SquidEntity.class, 10));
    }

    private Biome disableRain() {
        this.hasRain = false;
        return this;
    }

    public static void init() {
        for (int i = 0; i < 64; ++i) {
            for (int j = 0; j < 64; ++j) {
                Biome.BIOMES[i + j * 64] = Biome.locateBiome((float)i / 63.0f, (float)j / 63.0f);
            }
        }
        Biome.DESERT.topBlockId = Biome.DESERT.soilBlockId = (byte)Block.SAND.id;
        Biome.ICE_DESERT.topBlockId = Biome.ICE_DESERT.soilBlockId = (byte)Block.SAND.id;
    }

    public Feature getRandomTreeFeature(Random random) {
        if (random.nextInt(10) == 0) {
            return new LargeOakTreeFeature();
        }
        return new OakTreeFeature();
    }

    protected Biome enableSnow() {
        this.hasSnow = true;
        return this;
    }

    protected Biome setName(String name) {
        this.name = name;
        return this;
    }

    protected Biome setFoliageColor(int foliageColor) {
        this.foliageColor = foliageColor;
        return this;
    }

    protected Biome setGrassColor(int grassColor) {
        this.grassColor = grassColor;
        return this;
    }

    public static Biome getBiome(double temperature, double downfall) {
        int n = (int)(temperature * 63.0);
        int n2 = (int)(downfall * 63.0);
        return BIOMES[n + n2 * 64];
    }

    public static Biome locateBiome(float temperature, float downfall) {
        downfall *= temperature;
        if (temperature < 0.1f) {
            return TUNDRA;
        }
        if (downfall < 0.2f) {
            if (temperature < 0.5f) {
                return TUNDRA;
            }
            if (temperature < 0.95f) {
                return SAVANNA;
            }
            return DESERT;
        }
        if (downfall > 0.5f && temperature < 0.7f) {
            return SWAMPLAND;
        }
        if (temperature < 0.5f) {
            return TAIGA;
        }
        if (temperature < 0.97f) {
            if (downfall < 0.35f) {
                return SHRUBLAND;
            }
            return FOREST;
        }
        if (downfall < 0.45f) {
            return PLAINS;
        }
        if (downfall < 0.9f) {
            return SEASONAL_FOREST;
        }
        return RAINFOREST;
    }

    @Environment(value=EnvType.CLIENT)
    public int getSkyColor(float f) {
        if ((f /= 3.0f) < -1.0f) {
            f = -1.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        return Color.getHSBColor(0.62222224f - f * 0.05f, 0.5f + f * 0.1f, 1.0f).getRGB();
    }

    public List getSpawnableEntities(SpawnGroup group) {
        if (group == SpawnGroup.MONSTER) {
            return this.spawnableMonsters;
        }
        if (group == SpawnGroup.CREATURE) {
            return this.spawnablePassive;
        }
        if (group == SpawnGroup.WATER_CREATURE) {
            return this.spawnableWaterCreatures;
        }
        return null;
    }

    public boolean canSnow() {
        return this.hasSnow;
    }

    public boolean canRain() {
        if (this.hasSnow) {
            return false;
        }
        return this.hasRain;
    }

    static {
        Biome.init();
    }
}

