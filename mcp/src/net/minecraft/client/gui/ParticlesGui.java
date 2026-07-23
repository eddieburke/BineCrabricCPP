/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.GuiParticle;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ParticlesGui
extends DrawContext {
    private List particles = new ArrayList();
    private Minecraft minecraft;

    public ParticlesGui(Minecraft minecraft) {
        this.minecraft = minecraft;
    }

    public void tick() {
        for (int i = 0; i < this.particles.size(); ++i) {
            GuiParticle guiParticle = (GuiParticle)this.particles.get(i);
            guiParticle.tickColor();
            guiParticle.tickPosition(this);
            if (!guiParticle.removed) continue;
            this.particles.remove(i--);
        }
    }

    public void render(float tickDelta) {
        this.minecraft.textureManager.bindTexture(this.minecraft.textureManager.getTextureId("/gui/particles.png"));
        for (int i = 0; i < this.particles.size(); ++i) {
            GuiParticle guiParticle = (GuiParticle)this.particles.get(i);
            int n = (int)(guiParticle.lastX + (guiParticle.x - guiParticle.lastX) * (double)tickDelta - 4.0);
            int n2 = (int)(guiParticle.lastY + (guiParticle.y - guiParticle.lastY) * (double)tickDelta - 4.0);
            float f = (float)(guiParticle.lastA + (guiParticle.a - guiParticle.lastA) * (double)tickDelta);
            float f2 = (float)(guiParticle.lastR + (guiParticle.r - guiParticle.lastR) * (double)tickDelta);
            float f3 = (float)(guiParticle.lastG + (guiParticle.g - guiParticle.lastG) * (double)tickDelta);
            float f4 = (float)(guiParticle.lastB + (guiParticle.b - guiParticle.lastB) * (double)tickDelta);
            GL11.glColor4f(f2, f3, f4, f);
            this.drawTexture(n, n2, 40, 0, 8, 8);
        }
    }
}

