/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk.light;

import net.minecraft.block.Block;
import net.minecraft.world.LightType;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public class LightUpdate {
    public final LightType lightType;
    public int minX;
    public int minY;
    public int minZ;
    public int maxX;
    public int maxY;
    public int maxZ;

    public LightUpdate(LightType lightType, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        this.lightType = lightType;
        this.minX = minX;
        this.minY = minY;
        this.minZ = minZ;
        this.maxX = maxX;
        this.maxY = maxY;
        this.maxZ = maxZ;
    }

    public void updateLight(World world) {
        int n = this.maxX - this.minX + 1;
        int n2 = this.maxY - this.minY + 1;
        int n3 = this.maxZ - this.minZ + 1;
        int n4 = n * n2 * n3;
        if (n4 > 32768) {
            System.out.println("Light too large, skipping!");
            return;
        }
        int n5 = 0;
        int n6 = 0;
        boolean bl = false;
        boolean bl2 = false;
        for (int i = this.minX; i <= this.maxX; ++i) {
            for (int j = this.minZ; j <= this.maxZ; ++j) {
                int n7 = i >> 4;
                int n8 = j >> 4;
                boolean bl3 = false;
                if (bl && n7 == n5 && n8 == n6) {
                    bl3 = bl2;
                } else {
                    Chunk chunk;
                    bl3 = world.isRegionLoaded(i, 0, j, 1);
                    if (bl3 && (chunk = world.getChunk(i >> 4, j >> 4)).isEmpty()) {
                        bl3 = false;
                    }
                    bl2 = bl3;
                    n5 = n7;
                    n6 = n8;
                }
                if (!bl3) continue;
                if (this.minY < 0) {
                    this.minY = 0;
                }
                if (this.maxY >= 128) {
                    this.maxY = 127;
                }
                for (int k = this.minY; k <= this.maxY; ++k) {
                    int n9;
                    int n10 = world.getBrightness(this.lightType, i, k, j);
                    int n11 = 0;
                    int n12 = world.getBlockId(i, k, j);
                    int n13 = Block.BLOCKS_LIGHT_OPACITY[n12];
                    if (n13 == 0) {
                        n13 = 1;
                    }
                    int n14 = 0;
                    if (this.lightType == LightType.SKY) {
                        if (world.isTopY(i, k, j)) {
                            n14 = 15;
                        }
                    } else if (this.lightType == LightType.BLOCK) {
                        n14 = Block.BLOCKS_LIGHT_LUMINANCE[n12];
                    }
                    if (n13 >= 15 && n14 == 0) {
                        n11 = 0;
                    } else {
                        n9 = world.getBrightness(this.lightType, i - 1, k, j);
                        int n15 = world.getBrightness(this.lightType, i + 1, k, j);
                        int n16 = world.getBrightness(this.lightType, i, k - 1, j);
                        int n17 = world.getBrightness(this.lightType, i, k + 1, j);
                        int n18 = world.getBrightness(this.lightType, i, k, j - 1);
                        int n19 = world.getBrightness(this.lightType, i, k, j + 1);
                        n11 = n9;
                        if (n15 > n11) {
                            n11 = n15;
                        }
                        if (n16 > n11) {
                            n11 = n16;
                        }
                        if (n17 > n11) {
                            n11 = n17;
                        }
                        if (n18 > n11) {
                            n11 = n18;
                        }
                        if (n19 > n11) {
                            n11 = n19;
                        }
                        if ((n11 -= n13) < 0) {
                            n11 = 0;
                        }
                        if (n14 > n11) {
                            n11 = n14;
                        }
                    }
                    if (n10 == n11) continue;
                    world.setLight(this.lightType, i, k, j, n11);
                    n9 = n11 - 1;
                    if (n9 < 0) {
                        n9 = 0;
                    }
                    world.updateLight(this.lightType, i - 1, k, j, n9);
                    world.updateLight(this.lightType, i, k - 1, j, n9);
                    world.updateLight(this.lightType, i, k, j - 1, n9);
                    if (i + 1 >= this.maxX) {
                        world.updateLight(this.lightType, i + 1, k, j, n9);
                    }
                    if (k + 1 >= this.maxY) {
                        world.updateLight(this.lightType, i, k + 1, j, n9);
                    }
                    if (j + 1 < this.maxZ) continue;
                    world.updateLight(this.lightType, i, k, j + 1, n9);
                }
            }
        }
    }

    public boolean expand(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        if (minX >= this.minX && minY >= this.minY && minZ >= this.minZ && maxX <= this.maxX && maxY <= this.maxY && maxZ <= this.maxZ) {
            return true;
        }
        int n = 1;
        if (minX >= this.minX - n && minY >= this.minY - n && minZ >= this.minZ - n && maxX <= this.maxX + n && maxY <= this.maxY + n && maxZ <= this.maxZ + n) {
            int n2;
            int n3;
            int n4;
            int n5;
            int n6;
            int n7 = this.maxX - this.minX;
            int n8 = this.maxY - this.minY;
            int n9 = this.maxZ - this.minZ;
            if (minX > this.minX) {
                minX = this.minX;
            }
            if (minY > this.minY) {
                minY = this.minY;
            }
            if (minZ > this.minZ) {
                minZ = this.minZ;
            }
            if (maxX < this.maxX) {
                maxX = this.maxX;
            }
            if (maxY < this.maxY) {
                maxY = this.maxY;
            }
            if (maxZ < this.maxZ) {
                maxZ = this.maxZ;
            }
            if ((n6 = (n5 = maxX - minX) * (n4 = maxY - minY) * (n3 = maxZ - minZ)) - (n2 = n7 * n8 * n9) <= 2) {
                this.minX = minX;
                this.minY = minY;
                this.minZ = minZ;
                this.maxX = maxX;
                this.maxY = maxY;
                this.maxZ = maxZ;
                return true;
            }
        }
        return false;
    }
}

