/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import java.awt.image.BufferedImage;
import java.awt.image.RenderedImage;
import java.io.File;
import java.nio.ByteBuffer;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import org.lwjgl.BufferUtils;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class Screenshot {
    private static DateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd_HH.mm.ss");
    private static ByteBuffer pixels;
    private static byte[] colorBuffer;
    private static int[] pixelBuffer;

    public static String take(File gameDir, int width, int height) {
        try {
            File file;
            File file2 = new File(gameDir, "screenshots");
            file2.mkdir();
            if (pixels == null || pixels.capacity() < width * height) {
                pixels = BufferUtils.createByteBuffer(width * height * 3);
            }
            if (pixelBuffer == null || pixelBuffer.length < width * height * 3) {
                colorBuffer = new byte[width * height * 3];
                pixelBuffer = new int[width * height];
            }
            GL11.glPixelStorei(3333, 1);
            GL11.glPixelStorei(3317, 1);
            pixels.clear();
            GL11.glReadPixels(0, 0, width, height, 6407, 5121, pixels);
            pixels.clear();
            String string = "" + DATE_FORMAT.format(new Date());
            int n = 1;
            while ((file = new File(file2, string + (n == 1 ? "" : "_" + n) + ".png")).exists()) {
                ++n;
            }
            pixels.get(colorBuffer);
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    int n2;
                    int n3 = i + (height - j - 1) * width;
                    int n4 = colorBuffer[n3 * 3 + 0] & 0xFF;
                    int n5 = colorBuffer[n3 * 3 + 1] & 0xFF;
                    int n6 = colorBuffer[n3 * 3 + 2] & 0xFF;
                    Screenshot.pixelBuffer[i + j * width] = n2 = 0xFF000000 | n4 << 16 | n5 << 8 | n6;
                }
            }
            BufferedImage bufferedImage = new BufferedImage(width, height, 1);
            bufferedImage.setRGB(0, 0, width, height, pixelBuffer, 0, width);
            ImageIO.write((RenderedImage)bufferedImage, "png", file);
            return "Saved screenshot as " + file.getName();
        }
        catch (Exception exception) {
            exception.printStackTrace();
            return "Failed to save: " + exception;
        }
    }
}

