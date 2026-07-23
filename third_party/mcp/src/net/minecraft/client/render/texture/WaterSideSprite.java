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
public class WaterSideSprite
extends DynamicTexture {
    protected float[] current = new float[256];
    protected float[] next = new float[256];
    protected float[] heat = new float[256];
    protected float[] heatDelta = new float[256];
    private int ticks = 0;

    public WaterSideSprite() {
        super(Block.FLOWING_WATER.textureId + 1);
        this.replicate = 2;
    }

    public void tick() {
        int n;
        int n2;
        float f;
        int n3;
        int n4;
        ++this.ticks;
        for (n4 = 0; n4 < 16; ++n4) {
            for (n3 = 0; n3 < 16; ++n3) {
                f = 0.0f;
                for (int i = n3 - 2; i <= n3; ++i) {
                    n2 = n4 & 0xF;
                    n = i & 0xF;
                    f += this.current[n2 + n * 16];
                }
                this.next[n4 + n3 * 16] = f / 3.2f + this.heat[n4 + n3 * 16] * 0.8f;
            }
        }
        for (n4 = 0; n4 < 16; ++n4) {
            for (n3 = 0; n3 < 16; ++n3) {
                int n5 = n4 + n3 * 16;
                this.heat[n5] = this.heat[n5] + this.heatDelta[n4 + n3 * 16] * 0.05f;
                if (this.heat[n4 + n3 * 16] < 0.0f) {
                    this.heat[n4 + n3 * 16] = 0.0f;
                }
                int n6 = n4 + n3 * 16;
                this.heatDelta[n6] = this.heatDelta[n6] - 0.3f;
                if (!(Math.random() < 0.2)) continue;
                this.heatDelta[n4 + n3 * 16] = 0.5f;
            }
        }
        float[] fArray = this.next;
        this.next = this.current;
        this.current = fArray;
        for (n3 = 0; n3 < 256; ++n3) {
            f = this.current[n3 - this.ticks * 16 & 0xFF];
            if (f > 1.0f) {
                f = 1.0f;
            }
            if (f < 0.0f) {
                f = 0.0f;
            }
            float f2 = f * f;
            n2 = (int)(32.0f + f2 * 32.0f);
            n = (int)(50.0f + f2 * 64.0f);
            int n7 = 255;
            int n8 = (int)(146.0f + f2 * 50.0f);
            if (this.anaglyph) {
                int n9 = (n2 * 30 + n * 59 + n7 * 11) / 100;
                int n10 = (n2 * 30 + n * 70) / 100;
                int n11 = (n2 * 30 + n7 * 70) / 100;
                n2 = n9;
                n = n10;
                n7 = n11;
            }
            this.pixels[n3 * 4 + 0] = (byte)n2;
            this.pixels[n3 * 4 + 1] = (byte)n;
            this.pixels[n3 * 4 + 2] = (byte)n7;
            this.pixels[n3 * 4 + 3] = (byte)n8;
        }
    }
}

