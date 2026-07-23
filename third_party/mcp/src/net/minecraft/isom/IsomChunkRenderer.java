/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.isom;

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.util.Arrays;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.isom.IsomRenderChunk;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

@Environment(value=EnvType.CLIENT)
public class IsomChunkRenderer {
    private float[] colors = new float[768];
    private int[] pixels = new int[5120];
    private int[] height = new int[5120];
    private int[] waterHeight = new int[5120];
    private int[] waterBrightness = new int[5120];
    private int[] depth = new int[34];
    private int[] sprites = new int[768];

    public IsomChunkRenderer() {
        try {
            BufferedImage bufferedImage = ImageIO.read(IsomChunkRenderer.class.getResource("/terrain.png"));
            int[] nArray = new int[65536];
            bufferedImage.getRGB(0, 0, 256, 256, nArray, 0, 256);
            for (int i = 0; i < 256; ++i) {
                int n = 0;
                int n2 = 0;
                int n3 = 0;
                int n4 = i % 16 * 16;
                int n5 = i / 16 * 16;
                int n6 = 0;
                for (int j = 0; j < 16; ++j) {
                    for (int k = 0; k < 16; ++k) {
                        int n7 = nArray[k + n4 + (j + n5) * 256];
                        int n8 = n7 >> 24 & 0xFF;
                        if (n8 <= 128) continue;
                        n += n7 >> 16 & 0xFF;
                        n2 += n7 >> 8 & 0xFF;
                        n3 += n7 & 0xFF;
                        ++n6;
                    }
                    if (n6 == 0) {
                        ++n6;
                    }
                    this.colors[i * 3 + 0] = n / n6;
                    this.colors[i * 3 + 1] = n2 / n6;
                    this.colors[i * 3 + 2] = n3 / n6;
                }
            }
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
        for (int i = 0; i < 256; ++i) {
            if (Block.BLOCKS[i] == null) continue;
            this.sprites[i * 3 + 0] = Block.BLOCKS[i].getTexture(1);
            this.sprites[i * 3 + 1] = Block.BLOCKS[i].getTexture(2);
            this.sprites[i * 3 + 2] = Block.BLOCKS[i].getTexture(3);
        }
    }

    public void render(IsomRenderChunk chunk) {
        World world = chunk.world;
        if (world == null) {
            chunk.empty = true;
            chunk.rendered = true;
            return;
        }
        int n = chunk.chunkX * 16;
        int n2 = chunk.chunkZ * 16;
        int n3 = n + 16;
        int n4 = n2 + 16;
        Chunk chunk2 = world.getChunk(chunk.chunkX, chunk.chunkZ);
        if (chunk2.isEmpty()) {
            chunk.empty = true;
            chunk.rendered = true;
            return;
        }
        chunk.empty = false;
        Arrays.fill(this.height, 0);
        Arrays.fill(this.waterHeight, 0);
        Arrays.fill(this.depth, 160);
        for (int i = n4 - 1; i >= n2; --i) {
            for (int j = n3 - 1; j >= n; --j) {
                int n5 = j - n;
                int n6 = i - n2;
                int n7 = n5 + n6;
                boolean bl = true;
                for (int k = 0; k < 128; ++k) {
                    float f;
                    float f2;
                    int n8;
                    int n9 = n6 - n5 - k + 160 - 16;
                    if (n9 >= this.depth[n7] && n9 >= this.depth[n7 + 1]) continue;
                    Block block = Block.BLOCKS[world.getBlockId(j, k, i)];
                    if (block == null) {
                        bl = false;
                        continue;
                    }
                    if (block.material == Material.WATER) {
                        int n10 = world.getBlockId(j, k + 1, i);
                        if (n10 != 0 && Block.BLOCKS[n10].material == Material.WATER) continue;
                        float f3 = (float)k / 127.0f * 0.6f + 0.4f;
                        float f4 = world.getLightBrightness(j, k + 1, i) * f3;
                        if (n9 < 0 || n9 >= 160) continue;
                        int n11 = n7 + n9 * 32;
                        if (n7 >= 0 && n7 <= 32 && this.waterHeight[n11] <= k) {
                            this.waterHeight[n11] = k;
                            this.waterBrightness[n11] = (int)(f4 * 127.0f);
                        }
                        if (n7 >= -1 && n7 <= 31 && this.waterHeight[n11 + 1] <= k) {
                            this.waterHeight[n11 + 1] = k;
                            this.waterBrightness[n11 + 1] = (int)(f4 * 127.0f);
                        }
                        bl = false;
                        continue;
                    }
                    if (bl) {
                        if (n9 < this.depth[n7]) {
                            this.depth[n7] = n9;
                        }
                        if (n9 < this.depth[n7 + 1]) {
                            this.depth[n7 + 1] = n9;
                        }
                    }
                    float f5 = (float)k / 127.0f * 0.6f + 0.4f;
                    if (n9 >= 0 && n9 < 160) {
                        int n12 = n7 + n9 * 32;
                        int n13 = this.sprites[block.id * 3 + 0];
                        float f6 = (world.getLightBrightness(j, k + 1, i) * 0.8f + 0.2f) * f5;
                        n8 = n13;
                        if (n7 >= 0) {
                            f2 = f6;
                            if (this.height[n12] <= k) {
                                this.height[n12] = k;
                                this.pixels[n12] = 0xFF000000 | (int)(this.colors[n8 * 3 + 0] * f2) << 16 | (int)(this.colors[n8 * 3 + 1] * f2) << 8 | (int)(this.colors[n8 * 3 + 2] * f2);
                            }
                        }
                        if (n7 < 31) {
                            f2 = f6 * 0.9f;
                            if (this.height[n12 + 1] <= k) {
                                this.height[n12 + 1] = k;
                                this.pixels[n12 + 1] = 0xFF000000 | (int)(this.colors[n8 * 3 + 0] * f2) << 16 | (int)(this.colors[n8 * 3 + 1] * f2) << 8 | (int)(this.colors[n8 * 3 + 2] * f2);
                            }
                        }
                    }
                    if (n9 < -1 || n9 >= 159) continue;
                    int n14 = n7 + (n9 + 1) * 32;
                    int n15 = this.sprites[block.id * 3 + 1];
                    float f7 = world.getLightBrightness(j - 1, k, i) * 0.8f + 0.2f;
                    n8 = this.sprites[block.id * 3 + 2];
                    f2 = world.getLightBrightness(j, k, i + 1) * 0.8f + 0.2f;
                    if (n7 >= 0) {
                        f = f7 * f5 * 0.6f;
                        if (this.height[n14] <= k - 1) {
                            this.height[n14] = k - 1;
                            this.pixels[n14] = 0xFF000000 | (int)(this.colors[n15 * 3 + 0] * f) << 16 | (int)(this.colors[n15 * 3 + 1] * f) << 8 | (int)(this.colors[n15 * 3 + 2] * f);
                        }
                    }
                    if (n7 >= 31) continue;
                    f = f2 * 0.9f * f5 * 0.4f;
                    if (this.height[n14 + 1] > k - 1) continue;
                    this.height[n14 + 1] = k - 1;
                    this.pixels[n14 + 1] = 0xFF000000 | (int)(this.colors[n8 * 3 + 0] * f) << 16 | (int)(this.colors[n8 * 3 + 1] * f) << 8 | (int)(this.colors[n8 * 3 + 2] * f);
                }
            }
        }
        this.postProcess();
        if (chunk.image == null) {
            chunk.image = new BufferedImage(32, 160, 2);
        }
        chunk.image.setRGB(0, 0, 32, 160, this.pixels, 0, 32);
        chunk.rendered = true;
    }

    private void postProcess() {
        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j < 160; ++j) {
                int n = i + j * 32;
                if (this.height[n] == 0) {
                    this.pixels[n] = 0;
                }
                if (this.waterHeight[n] <= this.height[n]) continue;
                int n2 = this.pixels[n] >> 24 & 0xFF;
                this.pixels[n] = ((this.pixels[n] & 0xFEFEFE) >> 1) + this.waterBrightness[n];
                if (n2 < 128) {
                    this.pixels[n] = Integer.MIN_VALUE + this.waterBrightness[n] * 2;
                    continue;
                }
                int n3 = n;
                this.pixels[n3] = this.pixels[n3] | 0xFF000000;
            }
        }
    }
}

