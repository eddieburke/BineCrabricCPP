/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.texture;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.render.texture.DynamicTexture;

@Environment(value=EnvType.CLIENT)
public class FireSprite
extends DynamicTexture {
    protected float[] current = new float[320];
    protected float[] next = new float[320];

    public FireSprite(int i) {
        super(Block.FIRE.textureId + i * 16);
    }

    public void tick() {
        int n;
        int n2;
        int n3;
        int n4;
        float f;
        int n5;
        for (int i = 0; i < 16; ++i) {
            for (n5 = 0; n5 < 20; ++n5) {
                int n6 = 18;
                f = this.current[i + (n5 + 1) % 20 * 16] * (float)n6;
                for (n4 = i - 1; n4 <= i + 1; ++n4) {
                    for (n3 = n5; n3 <= n5 + 1; ++n3) {
                        n2 = n4;
                        n = n3;
                        if (n2 >= 0 && n >= 0 && n2 < 16 && n < 20) {
                            f += this.current[n2 + n * 16];
                        }
                        ++n6;
                    }
                }
                this.next[i + n5 * 16] = f / ((float)n6 * 1.06f);
                if (n5 < 19) continue;
                this.next[i + n5 * 16] = (float)(Math.random() * Math.random() * Math.random() * 4.0 + Math.random() * (double)0.1f + (double)0.2f);
            }
        }
        float[] fArray = this.next;
        this.next = this.current;
        this.current = fArray;
        for (n5 = 0; n5 < 256; ++n5) {
            float f2 = this.current[n5] * 1.8f;
            if (f2 > 1.0f) {
                f2 = 1.0f;
            }
            if (f2 < 0.0f) {
                f2 = 0.0f;
            }
            f = f2;
            n4 = (int)(f * 155.0f + 100.0f);
            n3 = (int)(f * f * 255.0f);
            n2 = (int)(f * f * f * f * f * f * f * f * f * f * 255.0f);
            n = 255;
            if (f < 0.5f) {
                n = 0;
            }
            f = (f - 0.5f) * 2.0f;
            if (this.anaglyph) {
                int n7 = (n4 * 30 + n3 * 59 + n2 * 11) / 100;
                int n8 = (n4 * 30 + n3 * 70) / 100;
                int n9 = (n4 * 30 + n2 * 70) / 100;
                n4 = n7;
                n3 = n8;
                n2 = n9;
            }
            this.pixels[n5 * 4 + 0] = (byte)n4;
            this.pixels[n5 * 4 + 1] = (byte)n3;
            this.pixels[n5 * 4 + 2] = (byte)n2;
            this.pixels[n5 * 4 + 3] = (byte)n;
        }
    }
}

