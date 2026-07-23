/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.resource.pack;

import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.resource.pack.TexturePack;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ZippedTexturePack
extends TexturePack {
    private ZipFile zip;
    private int iconId = -1;
    private BufferedImage icon;
    private File file;

    public ZippedTexturePack(File file) {
        this.name = file.getName();
        this.file = file;
    }

    private String trim(String line) {
        if (line != null && line.length() > 34) {
            line = line.substring(0, 34);
        }
        return line;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void load(Minecraft minecraft) {
        ZipFile zipFile = null;
        InputStream inputStream = null;
        try {
            zipFile = new ZipFile(this.file);
            try {
                inputStream = zipFile.getInputStream(zipFile.getEntry("pack.txt"));
                BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream));
                this.descriptionLine1 = this.trim(bufferedReader.readLine());
                this.descriptionLine2 = this.trim(bufferedReader.readLine());
                bufferedReader.close();
                inputStream.close();
            }
            catch (Exception exception) {
                // empty catch block
            }
            try {
                inputStream = zipFile.getInputStream(zipFile.getEntry("pack.png"));
                this.icon = ImageIO.read(inputStream);
                inputStream.close();
            }
            catch (Exception exception) {
                // empty catch block
            }
            zipFile.close();
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        finally {
            try {
                inputStream.close();
            }
            catch (Exception exception) {}
            try {
                zipFile.close();
            }
            catch (Exception exception) {}
        }
    }

    public void unload(Minecraft minecraft) {
        if (this.icon != null) {
            minecraft.textureManager.delete(this.iconId);
        }
        this.close();
    }

    public void bindIcon(Minecraft minecraft) {
        if (this.icon != null && this.iconId < 0) {
            this.iconId = minecraft.textureManager.load(this.icon);
        }
        if (this.icon != null) {
            minecraft.textureManager.bindTexture(this.iconId);
        } else {
            GL11.glBindTexture(3553, minecraft.textureManager.getTextureId("/gui/unknown_pack.png"));
        }
    }

    public void open() {
        try {
            this.zip = new ZipFile(this.file);
        }
        catch (Exception exception) {
            // empty catch block
        }
    }

    public void close() {
        try {
            this.zip.close();
        }
        catch (Exception exception) {
            // empty catch block
        }
        this.zip = null;
    }

    public InputStream getResource(String path) {
        try {
            ZipEntry zipEntry = this.zip.getEntry(path.substring(1));
            if (zipEntry != null) {
                return this.zip.getInputStream(zipEntry);
            }
        }
        catch (Exception exception) {
            // empty catch block
        }
        return TexturePack.class.getResourceAsStream(path);
    }
}

