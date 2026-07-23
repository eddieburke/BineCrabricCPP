/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import java.awt.image.BufferedImage;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.MapColor;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.map.MapState;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class MapRenderer {
    private int[] colors = new int[16384];
    private int texture;
    private GameOptions options;
    private TextRenderer textRenderer;

    public MapRenderer(TextRenderer textRenderer, GameOptions options, TextureManager textureManager) {
        this.options = options;
        this.textRenderer = textRenderer;
        this.texture = textureManager.load(new BufferedImage(128, 128, 2));
        for (int i = 0; i < 16384; ++i) {
            this.colors[i] = 0;
        }
    }

    public void render(PlayerEntity player, TextureManager textureManager, MapState mapState) {
        byte by;
        int n;
        for (n = 0; n < 16384; ++n) {
            by = mapState.colors[n];
            if (by / 4 == 0) {
                this.colors[n] = (n + n / 128 & 1) * 8 + 16 << 24;
                continue;
            }
            int n2 = MapColor.COLORS[by / 4].color;
            int n3 = by & 3;
            int n4 = 220;
            if (n3 == 2) {
                n4 = 255;
            }
            if (n3 == 0) {
                n4 = 180;
            }
            int n5 = (n2 >> 16 & 0xFF) * n4 / 255;
            int n6 = (n2 >> 8 & 0xFF) * n4 / 255;
            int n7 = (n2 & 0xFF) * n4 / 255;
            if (this.options.anaglyph3d) {
                int n8 = (n5 * 30 + n6 * 59 + n7 * 11) / 100;
                int n9 = (n5 * 30 + n6 * 70) / 100;
                int n10 = (n5 * 30 + n7 * 70) / 100;
                n5 = n8;
                n6 = n9;
                n7 = n10;
            }
            this.colors[n] = 0xFF000000 | n5 << 16 | n6 << 8 | n7;
        }
        textureManager.bind(this.colors, 128, 128, this.texture);
        n = 0;
        by = 0;
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = 0.0f;
        GL11.glBindTexture(3553, this.texture);
        GL11.glEnable(3042);
        GL11.glDisable(3008);
        tessellator.startQuads();
        tessellator.vertex((float)(n + 0) + f, (float)(by + 128) - f, -0.01f, 0.0, 1.0);
        tessellator.vertex((float)(n + 128) - f, (float)(by + 128) - f, -0.01f, 1.0, 1.0);
        tessellator.vertex((float)(n + 128) - f, (float)(by + 0) + f, -0.01f, 1.0, 0.0);
        tessellator.vertex((float)(n + 0) + f, (float)(by + 0) + f, -0.01f, 0.0, 0.0);
        tessellator.draw();
        GL11.glEnable(3008);
        GL11.glDisable(3042);
        textureManager.bindTexture(textureManager.getTextureId("/misc/mapicons.png"));
        for (MapState.MapIcon mapIcon : mapState.icons) {
            GL11.glPushMatrix();
            GL11.glTranslatef((float)n + (float)mapIcon.x / 2.0f + 64.0f, (float)by + (float)mapIcon.z / 2.0f + 64.0f, -0.02f);
            GL11.glRotatef((float)(mapIcon.rotation * 360) / 16.0f, 0.0f, 0.0f, 1.0f);
            GL11.glScalef(4.0f, 4.0f, 3.0f);
            GL11.glTranslatef(-0.125f, 0.125f, 0.0f);
            float f2 = (float)(mapIcon.type % 4 + 0) / 4.0f;
            float f3 = (float)(mapIcon.type / 4 + 0) / 4.0f;
            float f4 = (float)(mapIcon.type % 4 + 1) / 4.0f;
            float f5 = (float)(mapIcon.type / 4 + 1) / 4.0f;
            tessellator.startQuads();
            tessellator.vertex(-1.0, 1.0, 0.0, f2, f3);
            tessellator.vertex(1.0, 1.0, 0.0, f4, f3);
            tessellator.vertex(1.0, -1.0, 0.0, f4, f5);
            tessellator.vertex(-1.0, -1.0, 0.0, f2, f5);
            tessellator.draw();
            GL11.glPopMatrix();
        }
        GL11.glPushMatrix();
        GL11.glTranslatef(0.0f, 0.0f, -0.04f);
        GL11.glScalef(1.0f, 1.0f, 1.0f);
        this.textRenderer.draw(mapState.id, n, by, -16777216);
        GL11.glPopMatrix();
    }
}

