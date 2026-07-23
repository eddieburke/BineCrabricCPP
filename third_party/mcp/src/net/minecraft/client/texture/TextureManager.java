/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.texture;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.render.texture.DynamicTexture;
import net.minecraft.client.resource.pack.TexturePack;
import net.minecraft.client.resource.pack.TexturePacks;
import net.minecraft.client.texture.ImageDownload;
import net.minecraft.client.texture.ImageProcessor;
import net.minecraft.client.util.GlAllocationUtils;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class TextureManager {
    public static boolean MIPMAP = false;
    private HashMap textures = new HashMap();
    private HashMap colors = new HashMap();
    private HashMap images = new HashMap();
    private IntBuffer idBuffer = GlAllocationUtils.allocateIntBuffer(1);
    private ByteBuffer imageBuffer = GlAllocationUtils.allocateByteBuffer(0x100000);
    private List dynamicTextures = new ArrayList();
    private Map downloadedImages = new HashMap();
    private GameOptions gameOptions;
    private boolean clamp = false;
    private boolean blur = false;
    private TexturePacks texturePacks;
    private BufferedImage image = new BufferedImage(64, 64, 2);

    public TextureManager(TexturePacks texturePacks, GameOptions options) {
        this.texturePacks = texturePacks;
        this.gameOptions = options;
        Graphics graphics = this.image.getGraphics();
        graphics.setColor(Color.WHITE);
        graphics.fillRect(0, 0, 64, 64);
        graphics.setColor(Color.BLACK);
        graphics.drawString("missingtex", 1, 10);
        graphics.dispose();
    }

    public int[] getColors(String path) {
        TexturePack texturePack = this.texturePacks.selected;
        int[] nArray = (int[])this.colors.get(path);
        if (nArray != null) {
            return nArray;
        }
        try {
            nArray = null;
            if (path.startsWith("##")) {
                nArray = this.readColors(this.rescale(this.readImage(texturePack.getResource(path.substring(2)))));
            } else if (path.startsWith("%clamp%")) {
                this.clamp = true;
                nArray = this.readColors(this.readImage(texturePack.getResource(path.substring(7))));
                this.clamp = false;
            } else if (path.startsWith("%blur%")) {
                this.blur = true;
                nArray = this.readColors(this.readImage(texturePack.getResource(path.substring(6))));
                this.blur = false;
            } else {
                InputStream inputStream = texturePack.getResource(path);
                nArray = inputStream == null ? this.readColors(this.image) : this.readColors(this.readImage(inputStream));
            }
            this.colors.put(path, nArray);
            return nArray;
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
            int[] nArray2 = this.readColors(this.image);
            this.colors.put(path, nArray2);
            return nArray2;
        }
    }

    private int[] readColors(BufferedImage image) {
        int n = image.getWidth();
        int n2 = image.getHeight();
        int[] nArray = new int[n * n2];
        image.getRGB(0, 0, n, n2, nArray, 0, n);
        return nArray;
    }

    private int[] readColors(BufferedImage image, int[] colors) {
        int n = image.getWidth();
        int n2 = image.getHeight();
        image.getRGB(0, 0, n, n2, colors, 0, n);
        return colors;
    }

    public int getTextureId(String path) {
        TexturePack texturePack = this.texturePacks.selected;
        Integer n = (Integer)this.textures.get(path);
        if (n != null) {
            return n;
        }
        try {
            this.idBuffer.clear();
            GlAllocationUtils.generateTextureNames(this.idBuffer);
            int n2 = this.idBuffer.get(0);
            if (path.startsWith("##")) {
                this.load(this.rescale(this.readImage(texturePack.getResource(path.substring(2)))), n2);
            } else if (path.startsWith("%clamp%")) {
                this.clamp = true;
                this.load(this.readImage(texturePack.getResource(path.substring(7))), n2);
                this.clamp = false;
            } else if (path.startsWith("%blur%")) {
                this.blur = true;
                this.load(this.readImage(texturePack.getResource(path.substring(6))), n2);
                this.blur = false;
            } else {
                InputStream inputStream = texturePack.getResource(path);
                if (inputStream == null) {
                    this.load(this.image, n2);
                } else {
                    this.load(this.readImage(inputStream), n2);
                }
            }
            this.textures.put(path, n2);
            return n2;
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
            GlAllocationUtils.generateTextureNames(this.idBuffer);
            int n3 = this.idBuffer.get(0);
            this.load(this.image, n3);
            this.textures.put(path, n3);
            return n3;
        }
    }

    private BufferedImage rescale(BufferedImage image) {
        int n = image.getWidth() / 16;
        BufferedImage bufferedImage = new BufferedImage(16, image.getHeight() * n, 2);
        Graphics graphics = bufferedImage.getGraphics();
        for (int i = 0; i < n; ++i) {
            graphics.drawImage(image, -i * 16, i * image.getHeight(), null);
        }
        graphics.dispose();
        return bufferedImage;
    }

    public int load(BufferedImage image) {
        this.idBuffer.clear();
        GlAllocationUtils.generateTextureNames(this.idBuffer);
        int n = this.idBuffer.get(0);
        this.load(image, n);
        this.images.put(n, image);
        return n;
    }

    public void load(BufferedImage image, int id) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        int n7;
        int n8;
        GL11.glBindTexture(3553, id);
        if (MIPMAP) {
            GL11.glTexParameteri(3553, 10241, 9986);
            GL11.glTexParameteri(3553, 10240, 9728);
        } else {
            GL11.glTexParameteri(3553, 10241, 9728);
            GL11.glTexParameteri(3553, 10240, 9728);
        }
        if (this.blur) {
            GL11.glTexParameteri(3553, 10241, 9729);
            GL11.glTexParameteri(3553, 10240, 9729);
        }
        if (this.clamp) {
            GL11.glTexParameteri(3553, 10242, 10496);
            GL11.glTexParameteri(3553, 10243, 10496);
        } else {
            GL11.glTexParameteri(3553, 10242, 10497);
            GL11.glTexParameteri(3553, 10243, 10497);
        }
        int n9 = image.getWidth();
        int n10 = image.getHeight();
        int[] nArray = new int[n9 * n10];
        byte[] byArray = new byte[n9 * n10 * 4];
        image.getRGB(0, 0, n9, n10, nArray, 0, n9);
        for (n8 = 0; n8 < nArray.length; ++n8) {
            n7 = nArray[n8] >> 24 & 0xFF;
            n6 = nArray[n8] >> 16 & 0xFF;
            n5 = nArray[n8] >> 8 & 0xFF;
            n4 = nArray[n8] & 0xFF;
            if (this.gameOptions != null && this.gameOptions.anaglyph3d) {
                n3 = (n6 * 30 + n5 * 59 + n4 * 11) / 100;
                n2 = (n6 * 30 + n5 * 70) / 100;
                n = (n6 * 30 + n4 * 70) / 100;
                n6 = n3;
                n5 = n2;
                n4 = n;
            }
            byArray[n8 * 4 + 0] = (byte)n6;
            byArray[n8 * 4 + 1] = (byte)n5;
            byArray[n8 * 4 + 2] = (byte)n4;
            byArray[n8 * 4 + 3] = (byte)n7;
        }
        this.imageBuffer.clear();
        this.imageBuffer.put(byArray);
        this.imageBuffer.position(0).limit(byArray.length);
        GL11.glTexImage2D(3553, 0, 6408, n9, n10, 0, 6408, 5121, this.imageBuffer);
        if (MIPMAP) {
            for (n8 = 1; n8 <= 4; ++n8) {
                n7 = n9 >> n8 - 1;
                n6 = n9 >> n8;
                n5 = n10 >> n8;
                for (n4 = 0; n4 < n6; ++n4) {
                    for (n3 = 0; n3 < n5; ++n3) {
                        n2 = this.imageBuffer.getInt((n4 * 2 + 0 + (n3 * 2 + 0) * n7) * 4);
                        n = this.imageBuffer.getInt((n4 * 2 + 1 + (n3 * 2 + 0) * n7) * 4);
                        int n11 = this.imageBuffer.getInt((n4 * 2 + 1 + (n3 * 2 + 1) * n7) * 4);
                        int n12 = this.imageBuffer.getInt((n4 * 2 + 0 + (n3 * 2 + 1) * n7) * 4);
                        int n13 = this.crispBlend(this.crispBlend(n2, n), this.crispBlend(n11, n12));
                        this.imageBuffer.putInt((n4 + n3 * n6) * 4, n13);
                    }
                }
                GL11.glTexImage2D(3553, n8, 6408, n6, n5, 0, 6408, 5121, this.imageBuffer);
            }
        }
    }

    public void bind(int[] colors, int width, int height, int id) {
        GL11.glBindTexture(3553, id);
        if (MIPMAP) {
            GL11.glTexParameteri(3553, 10241, 9986);
            GL11.glTexParameteri(3553, 10240, 9728);
        } else {
            GL11.glTexParameteri(3553, 10241, 9728);
            GL11.glTexParameteri(3553, 10240, 9728);
        }
        if (this.blur) {
            GL11.glTexParameteri(3553, 10241, 9729);
            GL11.glTexParameteri(3553, 10240, 9729);
        }
        if (this.clamp) {
            GL11.glTexParameteri(3553, 10242, 10496);
            GL11.glTexParameteri(3553, 10243, 10496);
        } else {
            GL11.glTexParameteri(3553, 10242, 10497);
            GL11.glTexParameteri(3553, 10243, 10497);
        }
        byte[] byArray = new byte[width * height * 4];
        for (int i = 0; i < colors.length; ++i) {
            int n = colors[i] >> 24 & 0xFF;
            int n2 = colors[i] >> 16 & 0xFF;
            int n3 = colors[i] >> 8 & 0xFF;
            int n4 = colors[i] & 0xFF;
            if (this.gameOptions != null && this.gameOptions.anaglyph3d) {
                int n5 = (n2 * 30 + n3 * 59 + n4 * 11) / 100;
                int n6 = (n2 * 30 + n3 * 70) / 100;
                int n7 = (n2 * 30 + n4 * 70) / 100;
                n2 = n5;
                n3 = n6;
                n4 = n7;
            }
            byArray[i * 4 + 0] = (byte)n2;
            byArray[i * 4 + 1] = (byte)n3;
            byArray[i * 4 + 2] = (byte)n4;
            byArray[i * 4 + 3] = (byte)n;
        }
        this.imageBuffer.clear();
        this.imageBuffer.put(byArray);
        this.imageBuffer.position(0).limit(byArray.length);
        GL11.glTexSubImage2D(3553, 0, 0, 0, width, height, 6408, 5121, this.imageBuffer);
    }

    public void delete(int textureId) {
        this.images.remove(textureId);
        this.idBuffer.clear();
        this.idBuffer.put(textureId);
        this.idBuffer.flip();
        GL11.glDeleteTextures(this.idBuffer);
    }

    public int downloadTexture(String url, String backup) {
        ImageDownload imageDownload = (ImageDownload)this.downloadedImages.get(url);
        if (imageDownload != null && imageDownload.image != null && !imageDownload.uploaded) {
            if (imageDownload.textureId < 0) {
                imageDownload.textureId = this.load(imageDownload.image);
            } else {
                this.load(imageDownload.image, imageDownload.textureId);
            }
            imageDownload.uploaded = true;
        }
        if (imageDownload == null || imageDownload.textureId < 0) {
            if (backup == null) {
                return -1;
            }
            return this.getTextureId(backup);
        }
        return imageDownload.textureId;
    }

    public ImageDownload downloadImage(String url, ImageProcessor processor) {
        ImageDownload imageDownload = (ImageDownload)this.downloadedImages.get(url);
        if (imageDownload == null) {
            this.downloadedImages.put(url, new ImageDownload(url, processor));
        } else {
            ++imageDownload.requestCount;
        }
        return imageDownload;
    }

    public void releaseImage(String url) {
        ImageDownload imageDownload = (ImageDownload)this.downloadedImages.get(url);
        if (imageDownload != null) {
            --imageDownload.requestCount;
            if (imageDownload.requestCount == 0) {
                if (imageDownload.textureId >= 0) {
                    this.delete(imageDownload.textureId);
                }
                this.downloadedImages.remove(url);
            }
        }
    }

    public void addDynamicTexture(DynamicTexture texture) {
        this.dynamicTextures.add(texture);
        texture.tick();
    }

    public void tick() {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        int n7;
        int n8;
        int n9;
        int n10;
        DynamicTexture dynamicTexture;
        int n11;
        for (n11 = 0; n11 < this.dynamicTextures.size(); ++n11) {
            dynamicTexture = (DynamicTexture)this.dynamicTextures.get(n11);
            dynamicTexture.anaglyph = this.gameOptions.anaglyph3d;
            dynamicTexture.tick();
            this.imageBuffer.clear();
            this.imageBuffer.put(dynamicTexture.pixels);
            this.imageBuffer.position(0).limit(dynamicTexture.pixels.length);
            dynamicTexture.bind(this);
            for (n10 = 0; n10 < dynamicTexture.replicate; ++n10) {
                for (n9 = 0; n9 < dynamicTexture.replicate; ++n9) {
                    GL11.glTexSubImage2D(3553, 0, dynamicTexture.sprite % 16 * 16 + n10 * 16, dynamicTexture.sprite / 16 * 16 + n9 * 16, 16, 16, 6408, 5121, this.imageBuffer);
                    if (!MIPMAP) continue;
                    for (n8 = 1; n8 <= 4; ++n8) {
                        n7 = 16 >> n8 - 1;
                        n6 = 16 >> n8;
                        for (n5 = 0; n5 < n6; ++n5) {
                            for (n4 = 0; n4 < n6; ++n4) {
                                n3 = this.imageBuffer.getInt((n5 * 2 + 0 + (n4 * 2 + 0) * n7) * 4);
                                n2 = this.imageBuffer.getInt((n5 * 2 + 1 + (n4 * 2 + 0) * n7) * 4);
                                n = this.imageBuffer.getInt((n5 * 2 + 1 + (n4 * 2 + 1) * n7) * 4);
                                int n12 = this.imageBuffer.getInt((n5 * 2 + 0 + (n4 * 2 + 1) * n7) * 4);
                                int n13 = this.smoothBlend(this.smoothBlend(n3, n2), this.smoothBlend(n, n12));
                                this.imageBuffer.putInt((n5 + n4 * n6) * 4, n13);
                            }
                        }
                        GL11.glTexSubImage2D(3553, n8, dynamicTexture.sprite % 16 * n6, dynamicTexture.sprite / 16 * n6, n6, n6, 6408, 5121, this.imageBuffer);
                    }
                }
            }
        }
        for (n11 = 0; n11 < this.dynamicTextures.size(); ++n11) {
            dynamicTexture = (DynamicTexture)this.dynamicTextures.get(n11);
            if (dynamicTexture.copyTo <= 0) continue;
            this.imageBuffer.clear();
            this.imageBuffer.put(dynamicTexture.pixels);
            this.imageBuffer.position(0).limit(dynamicTexture.pixels.length);
            GL11.glBindTexture(3553, dynamicTexture.copyTo);
            GL11.glTexSubImage2D(3553, 0, 0, 0, 16, 16, 6408, 5121, this.imageBuffer);
            if (!MIPMAP) continue;
            for (n10 = 1; n10 <= 4; ++n10) {
                n9 = 16 >> n10 - 1;
                n8 = 16 >> n10;
                for (n7 = 0; n7 < n8; ++n7) {
                    for (n6 = 0; n6 < n8; ++n6) {
                        n5 = this.imageBuffer.getInt((n7 * 2 + 0 + (n6 * 2 + 0) * n9) * 4);
                        n4 = this.imageBuffer.getInt((n7 * 2 + 1 + (n6 * 2 + 0) * n9) * 4);
                        n3 = this.imageBuffer.getInt((n7 * 2 + 1 + (n6 * 2 + 1) * n9) * 4);
                        n2 = this.imageBuffer.getInt((n7 * 2 + 0 + (n6 * 2 + 1) * n9) * 4);
                        n = this.smoothBlend(this.smoothBlend(n5, n4), this.smoothBlend(n3, n2));
                        this.imageBuffer.putInt((n7 + n6 * n8) * 4, n);
                    }
                }
                GL11.glTexSubImage2D(3553, n10, 0, 0, n8, n8, 6408, 5121, this.imageBuffer);
            }
        }
    }

    private int smoothBlend(int color1, int color2) {
        int n = (color1 & 0xFF000000) >> 24 & 0xFF;
        int n2 = (color2 & 0xFF000000) >> 24 & 0xFF;
        return (n + n2 >> 1 << 24) + ((color1 & 0xFEFEFE) + (color2 & 0xFEFEFE) >> 1);
    }

    private int crispBlend(int color1, int color2) {
        int n = (color1 & 0xFF000000) >> 24 & 0xFF;
        int n2 = (color2 & 0xFF000000) >> 24 & 0xFF;
        int n3 = 255;
        if (n + n2 == 0) {
            n = 1;
            n2 = 1;
            n3 = 0;
        }
        int n4 = (color1 >> 16 & 0xFF) * n;
        int n5 = (color1 >> 8 & 0xFF) * n;
        int n6 = (color1 & 0xFF) * n;
        int n7 = (color2 >> 16 & 0xFF) * n2;
        int n8 = (color2 >> 8 & 0xFF) * n2;
        int n9 = (color2 & 0xFF) * n2;
        int n10 = (n4 + n7) / (n + n2);
        int n11 = (n5 + n8) / (n + n2);
        int n12 = (n6 + n9) / (n + n2);
        return n3 << 24 | n10 << 16 | n11 << 8 | n12;
    }

    public void reload() {
        BufferedImage bufferedImage;
        TexturePack texturePack = this.texturePacks.selected;
        Iterator<Object> iterator = this.images.keySet().iterator();
        while (iterator.hasNext()) {
            int n = (Integer)iterator.next();
            bufferedImage = (BufferedImage)this.images.get(n);
            this.load(bufferedImage, n);
        }
        for (ImageDownload imageDownload : this.downloadedImages.values()) {
            imageDownload.uploaded = false;
        }
        for (String string : this.textures.keySet()) {
            try {
                if (string.startsWith("##")) {
                    bufferedImage = this.rescale(this.readImage(texturePack.getResource(string.substring(2))));
                } else if (string.startsWith("%clamp%")) {
                    this.clamp = true;
                    bufferedImage = this.readImage(texturePack.getResource(string.substring(7)));
                } else if (string.startsWith("%blur%")) {
                    this.blur = true;
                    bufferedImage = this.readImage(texturePack.getResource(string.substring(6)));
                } else {
                    bufferedImage = this.readImage(texturePack.getResource(string));
                }
                int n = (Integer)this.textures.get(string);
                this.load(bufferedImage, n);
                this.blur = false;
                this.clamp = false;
            }
            catch (IOException iOException) {
                iOException.printStackTrace();
            }
        }
        for (String string : this.colors.keySet()) {
            try {
                if (string.startsWith("##")) {
                    bufferedImage = this.rescale(this.readImage(texturePack.getResource(string.substring(2))));
                } else if (string.startsWith("%clamp%")) {
                    this.clamp = true;
                    bufferedImage = this.readImage(texturePack.getResource(string.substring(7)));
                } else if (string.startsWith("%blur%")) {
                    this.blur = true;
                    bufferedImage = this.readImage(texturePack.getResource(string.substring(6)));
                } else {
                    bufferedImage = this.readImage(texturePack.getResource(string));
                }
                this.readColors(bufferedImage, (int[])this.colors.get(string));
                this.blur = false;
                this.clamp = false;
            }
            catch (IOException iOException) {
                iOException.printStackTrace();
            }
        }
    }

    private BufferedImage readImage(InputStream stream) {
        BufferedImage bufferedImage = ImageIO.read(stream);
        stream.close();
        return bufferedImage;
    }

    public void bindTexture(int id) {
        if (id < 0) {
            return;
        }
        GL11.glBindTexture(3553, id);
    }
}

