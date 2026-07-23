/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.texture;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.render.texture.DynamicTexture;
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class NetherPortalSprite
extends DynamicTexture {
    private int ticks = 0;
    private byte[][] frames = new byte[32][1024];

    public NetherPortalSprite() {
        super(Block.NETHER_PORTAL.textureId);
        Random random = new Random(100L);
        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j < 16; ++j) {
                for (int k = 0; k < 16; ++k) {
                    int n;
                    float f = 0.0f;
                    for (n = 0; n < 2; ++n) {
                        float f2 = n * 8;
                        float f3 = n * 8;
                        float f4 = ((float)j - f2) / 16.0f * 2.0f;
                        float f5 = ((float)k - f3) / 16.0f * 2.0f;
                        if (f4 < -1.0f) {
                            f4 += 2.0f;
                        }
                        if (f4 >= 1.0f) {
                            f4 -= 2.0f;
                        }
                        if (f5 < -1.0f) {
                            f5 += 2.0f;
                        }
                        if (f5 >= 1.0f) {
                            f5 -= 2.0f;
                        }
                        float f6 = f4 * f4 + f5 * f5;
                        float f7 = (float)Math.atan2(f5, f4) + ((float)i / 32.0f * (float)Math.PI * 2.0f - f6 * 10.0f + (float)(n * 2)) * (float)(n * 2 - 1);
                        f7 = (MathHelper.sin(f7) + 1.0f) / 2.0f;
                        f += (f7 /= f6 + 1.0f) * 0.5f;
                    }
                    n = (int)((f += random.nextFloat() * 0.1f) * 100.0f + 155.0f);
                    int n2 = (int)(f * f * 200.0f + 55.0f);
                    int n3 = (int)(f * f * f * f * 255.0f);
                    int n4 = (int)(f * 100.0f + 155.0f);
                    int n5 = k * 16 + j;
                    this.frames[i][n5 * 4 + 0] = (byte)n2;
                    this.frames[i][n5 * 4 + 1] = (byte)n3;
                    this.frames[i][n5 * 4 + 2] = (byte)n;
                    this.frames[i][n5 * 4 + 3] = (byte)n4;
                }
            }
        }
    }

    public void tick() {
        ++this.ticks;
        byte[] byArray = this.frames[this.ticks & 0x1F];
        for (int i = 0; i < 256; ++i) {
            int n = byArray[i * 4 + 0] & 0xFF;
            int n2 = byArray[i * 4 + 1] & 0xFF;
            int n3 = byArray[i * 4 + 2] & 0xFF;
            int n4 = byArray[i * 4 + 3] & 0xFF;
            if (this.anaglyph) {
                int n5 = (n * 30 + n2 * 59 + n3 * 11) / 100;
                int n6 = (n * 30 + n2 * 70) / 100;
                int n7 = (n * 30 + n3 * 70) / 100;
                n = n5;
                n2 = n6;
                n3 = n7;
            }
            this.pixels[i * 4 + 0] = (byte)n;
            this.pixels[i * 4 + 1] = (byte)n2;
            this.pixels[i * 4 + 2] = (byte)n3;
            this.pixels[i * 4 + 3] = (byte)n4;
        }
    }
}

