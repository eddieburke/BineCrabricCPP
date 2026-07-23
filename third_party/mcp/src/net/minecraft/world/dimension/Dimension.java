/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.dimension;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;
import net.minecraft.world.biome.source.BiomeSource;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.dimension.NetherDimension;
import net.minecraft.world.dimension.OverworldDimension;
import net.minecraft.world.dimension.SkylandsDimension;
import net.minecraft.world.gen.chunk.OverworldChunkGenerator;

public abstract class Dimension {
    public World world;
    public BiomeSource biomeSource;
    public boolean isNether = false;
    public boolean evaporatesWater = false;
    public boolean hasCeiling = false;
    public float[] lightLevelToLuminance = new float[16];
    public int id = 0;
    private float[] backgroundColor = new float[4];

    public final void setWorld(World world) {
        this.world = world;
        this.initBiomeSource();
        this.initBrightnessTable();
    }

    protected void initBrightnessTable() {
        float f = 0.05f;
        for (int i = 0; i <= 15; ++i) {
            float f2 = 1.0f - (float)i / 15.0f;
            this.lightLevelToLuminance[i] = (1.0f - f2) / (f2 * 3.0f + 1.0f) * (1.0f - f) + f;
        }
    }

    protected void initBiomeSource() {
        this.biomeSource = new BiomeSource(this.world);
    }

    public ChunkSource createChunkGenerator() {
        return new OverworldChunkGenerator(this.world, this.world.getSeed());
    }

    public boolean isValidSpawnPoint(int x, int z) {
        int n = this.world.getSpawnBlockId(x, z);
        return n == Block.SAND.id;
    }

    public float getTimeOfDay(long time, float tickDelta) {
        int n = (int)(time % 24000L);
        float f = ((float)n + tickDelta) / 24000.0f - 0.25f;
        if (f < 0.0f) {
            f += 1.0f;
        }
        if (f > 1.0f) {
            f -= 1.0f;
        }
        float f2 = f;
        f = 1.0f - (float)((Math.cos((double)f * Math.PI) + 1.0) / 2.0);
        f = f2 + (f - f2) / 3.0f;
        return f;
    }

    @Environment(value=EnvType.CLIENT)
    public float[] getBackgroundColor(float timeOfDay, float tickDelta) {
        float f;
        float f2 = 0.4f;
        float f3 = MathHelper.cos(timeOfDay * (float)Math.PI * 2.0f) - 0.0f;
        if (f3 >= (f = -0.0f) - f2 && f3 <= f + f2) {
            float f4 = (f3 - f) / f2 * 0.5f + 0.5f;
            float f5 = 1.0f - (1.0f - MathHelper.sin(f4 * (float)Math.PI)) * 0.99f;
            f5 *= f5;
            this.backgroundColor[0] = f4 * 0.3f + 0.7f;
            this.backgroundColor[1] = f4 * f4 * 0.7f + 0.2f;
            this.backgroundColor[2] = f4 * f4 * 0.0f + 0.2f;
            this.backgroundColor[3] = f5;
            return this.backgroundColor;
        }
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getFogColor(float timeOfDay, float tickDelta) {
        float f = MathHelper.cos(timeOfDay * (float)Math.PI * 2.0f) * 2.0f + 0.5f;
        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        float f2 = 0.7529412f;
        float f3 = 0.84705883f;
        float f4 = 1.0f;
        return Vec3d.createCached(f2 *= f * 0.94f + 0.06f, f3 *= f * 0.94f + 0.06f, f4 *= f * 0.91f + 0.09f);
    }

    public boolean hasWorldSpawn() {
        return true;
    }

    public static Dimension fromId(int id) {
        if (id == -1) {
            return new NetherDimension();
        }
        if (id == 0) {
            return new OverworldDimension();
        }
        if (id == 1) {
            return new SkylandsDimension();
        }
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public float getCloudHeight() {
        return 108.0f;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean hasGround() {
        return true;
    }
}

