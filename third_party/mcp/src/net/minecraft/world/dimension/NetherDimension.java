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
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.source.FixedBiomeSource;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.gen.chunk.NetherChunkGenerator;

public class NetherDimension
extends Dimension {
    public void initBiomeSource() {
        this.biomeSource = new FixedBiomeSource(Biome.HELL, 1.0, 0.0);
        this.isNether = true;
        this.evaporatesWater = true;
        this.hasCeiling = true;
        this.id = -1;
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getFogColor(float timeOfDay, float tickDelta) {
        return Vec3d.createCached(0.2f, 0.03f, 0.03f);
    }

    protected void initBrightnessTable() {
        float f = 0.1f;
        for (int i = 0; i <= 15; ++i) {
            float f2 = 1.0f - (float)i / 15.0f;
            this.lightLevelToLuminance[i] = (1.0f - f2) / (f2 * 3.0f + 1.0f) * (1.0f - f) + f;
        }
    }

    public ChunkSource createChunkGenerator() {
        return new NetherChunkGenerator(this.world, this.world.getSeed());
    }

    public boolean isValidSpawnPoint(int x, int z) {
        int n = this.world.getSpawnBlockId(x, z);
        if (n == Block.BEDROCK.id) {
            return false;
        }
        if (n == 0) {
            return false;
        }
        return Block.BLOCKS_OPAQUE[n];
    }

    public float getTimeOfDay(long time, float tickDelta) {
        return 0.5f;
    }

    public boolean hasWorldSpawn() {
        return false;
    }
}

