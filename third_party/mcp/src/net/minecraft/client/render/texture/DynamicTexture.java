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
import net.minecraft.client.texture.TextureManager;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class DynamicTexture {
    public byte[] pixels = new byte[1024];
    public int sprite;
    public boolean anaglyph = false;
    public int copyTo = 0;
    public int replicate = 1;
    public int atlas = 0;

    public DynamicTexture(int sprite) {
        this.sprite = sprite;
    }

    public void tick() {
    }

    public void bind(TextureManager textureManager) {
        if (this.atlas == 0) {
            GL11.glBindTexture(3553, textureManager.getTextureId("/terrain.png"));
        } else if (this.atlas == 1) {
            GL11.glBindTexture(3553, textureManager.getTextureId("/gui/items.png"));
        }
    }
}

