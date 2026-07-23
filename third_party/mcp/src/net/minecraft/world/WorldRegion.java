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
import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;
import net.minecraft.world.biome.source.BiomeSource;
import net.minecraft.world.chunk.Chunk;

public class WorldRegion
implements BlockView {
    private int chunkX;
    private int chunkZ;
    private Chunk[][] chunks;
    private World world;

    public WorldRegion(World world, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        this.world = world;
        this.chunkX = minX >> 4;
        this.chunkZ = minZ >> 4;
        int n = maxX >> 4;
        int n2 = maxZ >> 4;
        this.chunks = new Chunk[n - this.chunkX + 1][n2 - this.chunkZ + 1];
        for (int i = this.chunkX; i <= n; ++i) {
            for (int j = this.chunkZ; j <= n2; ++j) {
                this.chunks[i - this.chunkX][j - this.chunkZ] = world.getChunk(i, j);
            }
        }
    }

    public int getBlockId(int x, int y, int z) {
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            return 0;
        }
        int n = (x >> 4) - this.chunkX;
        int n2 = (z >> 4) - this.chunkZ;
        if (n < 0 || n >= this.chunks.length || n2 < 0 || n2 >= this.chunks[n].length) {
            return 0;
        }
        Chunk chunk = this.chunks[n][n2];
        if (chunk == null) {
            return 0;
        }
        return chunk.getBlockId(x & 0xF, y, z & 0xF);
    }

    public BlockEntity getBlockEntity(int x, int y, int z) {
        int n = (x >> 4) - this.chunkX;
        int n2 = (z >> 4) - this.chunkZ;
        return this.chunks[n][n2].getBlockEntity(x & 0xF, y, z & 0xF);
    }

    @Environment(value=EnvType.CLIENT)
    public float getNaturalBrightness(int x, int y, int z, int blockLight) {
        int n = this.getRawBrightness(x, y, z);
        if (n < blockLight) {
            n = blockLight;
        }
        return this.world.dimension.lightLevelToLuminance[n];
    }

    @Environment(value=EnvType.CLIENT)
    public float getLightBrightness(int i, int j, int k) {
        return this.world.dimension.lightLevelToLuminance[this.getRawBrightness(i, j, k)];
    }

    @Environment(value=EnvType.CLIENT)
    public int getRawBrightness(int x, int y, int z) {
        return this.getRawBrightness(x, y, z, true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getRawBrightness(int x, int y, int z, boolean useNeighborLight) {
        int n;
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 15;
        }
        if (useNeighborLight && ((n = this.getBlockId(x, y, z)) == Block.SLAB.id || n == Block.FARMLAND.id || n == Block.WOODEN_STAIRS.id || n == Block.COBBLESTONE_STAIRS.id)) {
            int n2 = this.getRawBrightness(x, y + 1, z, false);
            int n3 = this.getRawBrightness(x + 1, y, z, false);
            int n4 = this.getRawBrightness(x - 1, y, z, false);
            int n5 = this.getRawBrightness(x, y, z + 1, false);
            int n6 = this.getRawBrightness(x, y, z - 1, false);
            if (n3 > n2) {
                n2 = n3;
            }
            if (n4 > n2) {
                n2 = n4;
            }
            if (n5 > n2) {
                n2 = n5;
            }
            if (n6 > n2) {
                n2 = n6;
            }
            return n2;
        }
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            n = 15 - this.world.ambientDarkness;
            if (n < 0) {
                n = 0;
            }
            return n;
        }
        n = (x >> 4) - this.chunkX;
        int n7 = (z >> 4) - this.chunkZ;
        return this.chunks[n][n7].getLight(x & 0xF, y, z & 0xF, this.world.ambientDarkness);
    }

    public int getBlockMeta(int x, int y, int z) {
        if (y < 0) {
            return 0;
        }
        if (y >= 128) {
            return 0;
        }
        int n = (x >> 4) - this.chunkX;
        int n2 = (z >> 4) - this.chunkZ;
        return this.chunks[n][n2].getBlockMeta(x & 0xF, y, z & 0xF);
    }

    public Material getMaterial(int x, int y, int z) {
        int n = this.getBlockId(x, y, z);
        if (n == 0) {
            return Material.AIR;
        }
        return Block.BLOCKS[n].material;
    }

    @Environment(value=EnvType.CLIENT)
    public BiomeSource getBiomeSource() {
        return this.world.getBiomeSource();
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isBlockOpaqueCube(int i, int j, int k) {
        Block block = Block.BLOCKS[this.getBlockId(i, j, k)];
        if (block == null) {
            return false;
        }
        return block.isOpaque();
    }

    public boolean shouldSuffocate(int x, int y, int z) {
        Block block = Block.BLOCKS[this.getBlockId(x, y, z)];
        if (block == null) {
            return false;
        }
        return block.material.blocksMovement() && block.isFullCube();
    }
}

