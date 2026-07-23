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
import net.minecraft.util.math.MathHelper;

@Environment(value=EnvType.CLIENT)
public class LavaSideSprite
extends DynamicTexture {
    protected float[] current = new float[256];
    protected float[] next = new float[256];
    protected float[] heat = new float[256];
    protected float[] heatDelta = new float[256];
    int ticks = 0;

    public LavaSideSprite() {
        super(Block.FLOWING_LAVA.textureId + 1);
        this.replicate = 2;
    }

    public void tick() {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        float f;
        int n6;
        ++this.ticks;
        for (int i = 0; i < 16; ++i) {
            for (n6 = 0; n6 < 16; ++n6) {
                f = 0.0f;
                int n7 = (int)(MathHelper.sin((float)n6 * (float)Math.PI * 2.0f / 16.0f) * 1.2f);
                n5 = (int)(MathHelper.sin((float)i * (float)Math.PI * 2.0f / 16.0f) * 1.2f);
                for (n4 = i - 1; n4 <= i + 1; ++n4) {
                    for (n3 = n6 - 1; n3 <= n6 + 1; ++n3) {
                        n2 = n4 + n7 & 0xF;
                        n = n3 + n5 & 0xF;
                        f += this.current[n2 + n * 16];
                    }
                }
                this.next[i + n6 * 16] = f / 10.0f + (this.heat[(i + 0 & 0xF) + (n6 + 0 & 0xF) * 16] + this.heat[(i + 1 & 0xF) + (n6 + 0 & 0xF) * 16] + this.heat[(i + 1 & 0xF) + (n6 + 1 & 0xF) * 16] + this.heat[(i + 0 & 0xF) + (n6 + 1 & 0xF) * 16]) / 4.0f * 0.8f;
                int n8 = i + n6 * 16;
                this.heat[n8] = this.heat[n8] + this.heatDelta[i + n6 * 16] * 0.01f;
                if (this.heat[i + n6 * 16] < 0.0f) {
                    this.heat[i + n6 * 16] = 0.0f;
                }
                int n9 = i + n6 * 16;
                this.heatDelta[n9] = this.heatDelta[n9] - 0.06f;
                if (!(Math.random() < 0.005)) continue;
                this.heatDelta[i + n6 * 16] = 1.5f;
            }
        }
        float[] fArray = this.next;
        this.next = this.current;
        this.current = fArray;
        for (n6 = 0; n6 < 256; ++n6) {
            f = this.current[n6 - this.ticks / 3 * 16 & 0xFF] * 2.0f;
            if (f > 1.0f) {
                f = 1.0f;
            }
            if (f < 0.0f) {
                f = 0.0f;
            }
            float f2 = f;
            n5 = (int)(f2 * 100.0f + 155.0f);
            n4 = (int)(f2 * f2 * 255.0f);
            n3 = (int)(f2 * f2 * f2 * f2 * 128.0f);
            if (this.anaglyph) {
                n2 = (n5 * 30 + n4 * 59 + n3 * 11) / 100;
                n = (n5 * 30 + n4 * 70) / 100;
                int n10 = (n5 * 30 + n3 * 70) / 100;
                n5 = n2;
                n4 = n;
                n3 = n10;
            }
            this.pixels[n6 * 4 + 0] = (byte)n5;
            this.pixels[n6 * 4 + 1] = (byte)n4;
            this.pixels[n6 * 4 + 2] = (byte)n3;
            this.pixels[n6 * 4 + 3] = -1;
        }
    }
}

