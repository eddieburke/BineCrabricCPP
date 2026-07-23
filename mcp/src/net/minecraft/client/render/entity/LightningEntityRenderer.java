/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.entity.LightningEntity;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class LightningEntityRenderer
extends EntityRenderer {
    public void render(LightningEntity arg, double d, double e, double f, float g, float h) {
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glDisable(3553);
        GL11.glDisable(2896);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 1);
        double[] dArray = new double[8];
        double[] dArray2 = new double[8];
        double d2 = 0.0;
        double d3 = 0.0;
        Random random = new Random(arg.seed);
        for (int i = 7; i >= 0; --i) {
            dArray[i] = d2;
            dArray2[i] = d3;
            d2 += (double)(random.nextInt(11) - 5);
            d3 += (double)(random.nextInt(11) - 5);
        }
        for (int i = 0; i < 4; ++i) {
            Random random2 = new Random(arg.seed);
            for (int j = 0; j < 3; ++j) {
                int n = 7;
                int n2 = 0;
                if (j > 0) {
                    n = 7 - j;
                }
                if (j > 0) {
                    n2 = n - 2;
                }
                double d4 = dArray[n] - d2;
                double d5 = dArray2[n] - d3;
                for (int k = n; k >= n2; --k) {
                    double d6 = d4;
                    double d7 = d5;
                    if (j == 0) {
                        d4 += (double)(random2.nextInt(11) - 5);
                        d5 += (double)(random2.nextInt(11) - 5);
                    } else {
                        d4 += (double)(random2.nextInt(31) - 15);
                        d5 += (double)(random2.nextInt(31) - 15);
                    }
                    tessellator.start(GL11.GL_TRIANGLE_STRIP);
                    float f2 = 0.5f;
                    tessellator.color(0.9f * f2, 0.9f * f2, 1.0f * f2, 0.3f);
                    double d8 = 0.1 + (double)i * 0.2;
                    if (j == 0) {
                        d8 *= (double)k * 0.1 + 1.0;
                    }
                    double d9 = 0.1 + (double)i * 0.2;
                    if (j == 0) {
                        d9 *= (double)(k - 1) * 0.1 + 1.0;
                    }
                    for (int i2 = 0; i2 < 5; ++i2) {
                        double d10 = d + 0.5 - d8;
                        double d11 = f + 0.5 - d8;
                        if (i2 == 1 || i2 == 2) {
                            d10 += d8 * 2.0;
                        }
                        if (i2 == 2 || i2 == 3) {
                            d11 += d8 * 2.0;
                        }
                        double d12 = d + 0.5 - d9;
                        double d13 = f + 0.5 - d9;
                        if (i2 == 1 || i2 == 2) {
                            d12 += d9 * 2.0;
                        }
                        if (i2 == 2 || i2 == 3) {
                            d13 += d9 * 2.0;
                        }
                        tessellator.vertex(d12 + d4, e + (double)(k * 16), d13 + d5);
                        tessellator.vertex(d10 + d6, e + (double)((k + 1) * 16), d11 + d7);
                    }
                    tessellator.draw();
                }
            }
        }
        GL11.glDisable(3042);
        GL11.glEnable(2896);
        GL11.glEnable(3553);
    }
}

