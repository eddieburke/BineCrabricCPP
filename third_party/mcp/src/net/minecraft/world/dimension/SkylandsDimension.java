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
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.source.FixedBiomeSource;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.gen.chunk.SkyChunkGenerator;

public class SkylandsDimension
extends Dimension {
    public void initBiomeSource() {
        this.biomeSource = new FixedBiomeSource(Biome.SKY, 0.5, 0.0);
        this.id = 1;
    }

    public ChunkSource createChunkGenerator() {
        return new SkyChunkGenerator(this.world, this.world.getSeed());
    }

    public float getTimeOfDay(long time, float tickDelta) {
        return 0.0f;
    }

    @Environment(value=EnvType.CLIENT)
    public float[] getBackgroundColor(float timeOfDay, float tickDelta) {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d getFogColor(float timeOfDay, float tickDelta) {
        int n = 0x8080A0;
        float f = MathHelper.cos(timeOfDay * (float)Math.PI * 2.0f) * 2.0f + 0.5f;
        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        float f2 = (float)(n >> 16 & 0xFF) / 255.0f;
        float f3 = (float)(n >> 8 & 0xFF) / 255.0f;
        float f4 = (float)(n & 0xFF) / 255.0f;
        return Vec3d.createCached(f2 *= f * 0.94f + 0.06f, f3 *= f * 0.94f + 0.06f, f4 *= f * 0.91f + 0.09f);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean hasGround() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public float getCloudHeight() {
        return 8.0f;
    }

    public boolean isValidSpawnPoint(int x, int z) {
        int n = this.world.getSpawnBlockId(x, z);
        if (n == 0) {
            return false;
        }
        return Block.BLOCKS[n].material.blocksMovement();
    }
}

