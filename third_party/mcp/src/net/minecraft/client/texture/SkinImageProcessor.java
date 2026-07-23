/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.texture;

import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.texture.ImageProcessor;

@Environment(value=EnvType.CLIENT)
public class SkinImageProcessor
implements ImageProcessor {
    private int[] data;
    private int width;
    private int height;

    public BufferedImage process(BufferedImage image) {
        int n;
        int n2;
        int n3;
        if (image == null) {
            return null;
        }
        this.width = 64;
        this.height = 32;
        BufferedImage bufferedImage = new BufferedImage(this.width, this.height, 2);
        Graphics graphics = bufferedImage.getGraphics();
        graphics.drawImage(image, 0, 0, null);
        graphics.dispose();
        this.data = ((DataBufferInt)bufferedImage.getRaster().getDataBuffer()).getData();
        this.setOpaque(0, 0, 32, 16);
        this.setTransparent(32, 0, 64, 32);
        this.setOpaque(0, 16, 64, 32);
        boolean bl = false;
        for (n3 = 32; n3 < 64; ++n3) {
            for (n2 = 0; n2 < 16; ++n2) {
                n = this.data[n3 + n2 * 64];
                if ((n >> 24 & 0xFF) >= 128) continue;
                bl = true;
            }
        }
        if (!bl) {
            for (n3 = 32; n3 < 64; ++n3) {
                for (n2 = 0; n2 < 16; ++n2) {
                    n = this.data[n3 + n2 * 64];
                    if ((n >> 24 & 0xFF) >= 128) continue;
                    bl = true;
                }
            }
        }
        return bufferedImage;
    }

    private void setTransparent(int x1, int y1, int x2, int y2) {
        if (this.hasTransparency(x1, y1, x2, y2)) {
            return;
        }
        for (int i = x1; i < x2; ++i) {
            for (int j = y1; j < y2; ++j) {
                int n = i + j * this.width;
                this.data[n] = this.data[n] & 0xFFFFFF;
            }
        }
    }

    private void setOpaque(int x1, int y1, int x2, int y2) {
        for (int i = x1; i < x2; ++i) {
            for (int j = y1; j < y2; ++j) {
                int n = i + j * this.width;
                this.data[n] = this.data[n] | 0xFF000000;
            }
        }
    }

    private boolean hasTransparency(int x1, int y1, int x2, int y2) {
        for (int i = x1; i < x2; ++i) {
            for (int j = y1; j < y2; ++j) {
                int n = this.data[i + j * this.width];
                if ((n >> 24 & 0xFF) >= 128) continue;
                return true;
            }
        }
        return false;
    }
}

