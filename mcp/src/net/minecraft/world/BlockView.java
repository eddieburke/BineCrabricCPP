/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.world.biome.source.BiomeSource;

public interface BlockView {
    public int getBlockId(int var1, int var2, int var3);

    public BlockEntity getBlockEntity(int var1, int var2, int var3);

    @Environment(value=EnvType.CLIENT)
    public float getNaturalBrightness(int var1, int var2, int var3, int var4);

    @Environment(value=EnvType.CLIENT)
    public float getLightBrightness(int var1, int var2, int var3);

    public int getBlockMeta(int var1, int var2, int var3);

    public Material getMaterial(int var1, int var2, int var3);

    @Environment(value=EnvType.CLIENT)
    public boolean isBlockOpaqueCube(int var1, int var2, int var3);

    public boolean shouldSuffocate(int var1, int var2, int var3);

    @Environment(value=EnvType.CLIENT)
    public BiomeSource getBiomeSource();
}

