/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.font;

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.IntBuffer;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.util.CharacterUtils;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class TextRenderer {
    private int[] characterWidths = new int[256];
    public int boundTexture = 0;
    private int boundPage;
    private IntBuffer pageBuffer = GlAllocationUtils.allocateIntBuffer(1024);

    public TextRenderer(GameOptions options, String fontPath, TextureManager textureManager) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        BufferedImage bufferedImage;
        try {
            bufferedImage = ImageIO.read(TextureManager.class.getResourceAsStream(fontPath));
        }
        catch (IOException iOException) {
            throw new RuntimeException(iOException);
        }
        int n6 = bufferedImage.getWidth();
        int n7 = bufferedImage.getHeight();
        int[] nArray = new int[n6 * n7];
        bufferedImage.getRGB(0, 0, n6, n7, nArray, 0, n6);
        for (int i = 0; i < 256; ++i) {
            n5 = i % 16;
            n4 = i / 16;
            for (n3 = 7; n3 >= 0; --n3) {
                int n8 = n5 * 8 + n3;
                boolean bl = true;
                for (int j = 0; j < 8 && bl; ++j) {
                    n2 = (n4 * 8 + j) * n6;
                    n = nArray[n8 + n2] & 0xFF;
                    if (n <= 0) continue;
                    bl = false;
                }
                if (!bl) break;
            }
            if (i == 32) {
                n3 = 2;
            }
            this.characterWidths[i] = n3 + 2;
        }
        this.boundTexture = textureManager.load(bufferedImage);
        this.boundPage = GlAllocationUtils.generateDisplayLists(288);
        for (n5 = 0; n5 < 256; ++n5) {
            GL11.glNewList(this.boundPage + n5, 4864);
            n4 = n5 % 16 * 8;
            n3 = n5 / 16 * 8;
            float f = 7.99f;
            GL11.glBegin(GL11.GL_QUADS);
            GL11.glTexCoord2f((float)n4 / 128.0f,           ((float)n3 + f) / 128.0f);
            GL11.glVertex3f(0.0f,      f,   0.0f);
            GL11.glTexCoord2f(((float)n4 + f) / 128.0f,     ((float)n3 + f) / 128.0f);
            GL11.glVertex3f(f,         f,   0.0f);
            GL11.glTexCoord2f(((float)n4 + f) / 128.0f,     (float)n3 / 128.0f);
            GL11.glVertex3f(f,         0.0f, 0.0f);
            GL11.glTexCoord2f((float)n4 / 128.0f,           (float)n3 / 128.0f);
            GL11.glVertex3f(0.0f,      0.0f, 0.0f);
            GL11.glEnd();
            GL11.glTranslatef(this.characterWidths[n5], 0.0f, 0.0f);
            GL11.glEndList();
        }
        for (n5 = 0; n5 < 32; ++n5) {
            boolean bl;
            n4 = (n5 >> 3 & 1) * 85;
            n3 = (n5 >> 2 & 1) * 170 + n4;
            int n9 = (n5 >> 1 & 1) * 170 + n4;
            int n10 = (n5 >> 0 & 1) * 170 + n4;
            if (n5 == 6) {
                n3 += 85;
            }
            boolean bl2 = bl = n5 >= 16;
            if (options.anaglyph3d) {
                n2 = (n3 * 30 + n9 * 59 + n10 * 11) / 100;
                n = (n3 * 30 + n9 * 70) / 100;
                int n11 = (n3 * 30 + n10 * 70) / 100;
                n3 = n2;
                n9 = n;
                n10 = n11;
            }
            if (bl) {
                n3 /= 4;
                n9 /= 4;
                n10 /= 4;
            }
            GL11.glNewList(this.boundPage + 256 + n5, 4864);
            GL11.glColor3f((float)n3 / 255.0f, (float)n9 / 255.0f, (float)n10 / 255.0f);
            GL11.glEndList();
        }
    }

    public void drawWithShadow(String text, int x, int y, int color) {
        this.draw(text, x + 1, y + 1, color, true);
        this.draw(text, x, y, color);
    }

    public void draw(String text, int x, int y, int color) {
        this.draw(text, x, y, color, false);
    }

    public void draw(String text, int x, int y, int color, boolean shadow) {
        if (text == null) {
            return;
        }
        if (shadow) {
            int n = color & 0xFF000000;
            color = (color & 0xFCFCFC) >> 2;
            color += n;
        }
        GL11.glBindTexture(3553, this.boundTexture);
        float f = (float)(color >> 16 & 0xFF) / 255.0f;
        float f2 = (float)(color >> 8 & 0xFF) / 255.0f;
        float f3 = (float)(color & 0xFF) / 255.0f;
        float f4 = (float)(color >> 24 & 0xFF) / 255.0f;
        if (f4 == 0.0f) {
            f4 = 1.0f;
        }
        GL11.glColor4f(f, f2, f3, f4);
        this.pageBuffer.clear();
        GL11.glPushMatrix();
        GL11.glTranslatef(x, y, 0.0f);
        for (int i = 0; i < text.length(); ++i) {
            int n;
            while (text.length() > i + 1 && text.charAt(i) == '\u00a7') {
                int n2 = "0123456789abcdef".indexOf(text.toLowerCase().charAt(i + 1));
                if (n2 < 0 || n2 > 15) {
                    n2 = 15;
                }
                this.pageBuffer.put(this.boundPage + 256 + n2 + (shadow ? 16 : 0));
                if (this.pageBuffer.remaining() == 0) {
                    this.pageBuffer.flip();
                    GL11.glCallLists(this.pageBuffer);
                    this.pageBuffer.clear();
                }
                i += 2;
            }
            if (i < text.length() && (n = CharacterUtils.VALID_CHARACTERS.indexOf(text.charAt(i))) >= 0) {
                this.pageBuffer.put(this.boundPage + n + 32);
            }
            if (this.pageBuffer.remaining() != 0) continue;
            this.pageBuffer.flip();
            GL11.glCallLists(this.pageBuffer);
            this.pageBuffer.clear();
        }
        this.pageBuffer.flip();
        GL11.glCallLists(this.pageBuffer);
        GL11.glPopMatrix();
    }

    public int getWidth(String text) {
        if (text == null) {
            return 0;
        }
        int n = 0;
        for (int i = 0; i < text.length(); ++i) {
            if (text.charAt(i) == '\u00a7') {
                ++i;
                continue;
            }
            int n2 = CharacterUtils.VALID_CHARACTERS.indexOf(text.charAt(i));
            if (n2 < 0) continue;
            n += this.characterWidths[n2 + 32];
        }
        return n;
    }

    public void drawSplit(String text, int x, int y, int width, int color) {
        String[] stringArray = text.split("\n");
        if (stringArray.length > 1) {
            for (int i = 0; i < stringArray.length; ++i) {
                this.drawSplit(stringArray[i], x, y, width, color);
                y += this.splitAndGetHeight(stringArray[i], width);
            }
            return;
        }
        String[] stringArray2 = text.split(" ");
        int n = 0;
        while (n < stringArray2.length) {
            String string = stringArray2[n++] + " ";
            while (n < stringArray2.length && this.getWidth(string + stringArray2[n]) < width) {
                string = string + stringArray2[n++] + " ";
            }
            while (this.getWidth(string) > width) {
                int n2 = 0;
                while (this.getWidth(string.substring(0, n2 + 1)) <= width) {
                    ++n2;
                }
                if (string.substring(0, n2).trim().length() > 0) {
                    this.draw(string.substring(0, n2), x, y, color);
                    y += 8;
                }
                string = string.substring(n2);
            }
            if (string.trim().length() <= 0) continue;
            this.draw(string, x, y, color);
            y += 8;
        }
    }

    public int splitAndGetHeight(String text, int width) {
        String[] stringArray = text.split("\n");
        if (stringArray.length > 1) {
            int n = 0;
            for (int i = 0; i < stringArray.length; ++i) {
                n += this.splitAndGetHeight(stringArray[i], width);
            }
            return n;
        }
        String[] stringArray2 = text.split(" ");
        int n = 0;
        int n2 = 0;
        while (n < stringArray2.length) {
            String string = stringArray2[n++] + " ";
            while (n < stringArray2.length && this.getWidth(string + stringArray2[n]) < width) {
                string = string + stringArray2[n++] + " ";
            }
            while (this.getWidth(string) > width) {
                int n3 = 0;
                while (this.getWidth(string.substring(0, n3 + 1)) <= width) {
                    ++n3;
                }
                if (string.substring(0, n3).trim().length() > 0) {
                    n2 += 8;
                }
                string = string.substring(n3);
            }
            if (string.trim().length() <= 0) continue;
            n2 += 8;
        }
        if (n2 < 8) {
            n2 += 8;
        }
        return n2;
    }
}

