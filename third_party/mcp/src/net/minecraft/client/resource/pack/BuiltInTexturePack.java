/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.resource.pack;

import java.awt.image.BufferedImage;
import java.io.IOException;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.resource.pack.TexturePack;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class BuiltInTexturePack
extends TexturePack {
    private int iconId = -1;
    private BufferedImage icon;

    public BuiltInTexturePack() {
        this.name = "Default";
        this.descriptionLine1 = "The default look of Minecraft";
        try {
            this.icon = ImageIO.read(BuiltInTexturePack.class.getResource("/pack.png"));
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
    }

    public void unload(Minecraft minecraft) {
        if (this.icon != null) {
            minecraft.textureManager.delete(this.iconId);
        }
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
}

