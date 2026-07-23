/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class FishingBobberEntityRenderer
extends EntityRenderer {
    public void render(FishingBobberEntity arg, double d, double e, double f, float g, float h) {
        GL11.glPushMatrix();
        GL11.glTranslatef((float)d, (float)e, (float)f);
        GL11.glEnable(32826);
        GL11.glScalef(0.5f, 0.5f, 0.5f);
        int n = 1;
        int n2 = 2;
        this.bindTexture("/particles.png");
        Tessellator tessellator = Tessellator.INSTANCE;
        float f2 = (float)(n * 8 + 0) / 128.0f;
        float f3 = (float)(n * 8 + 8) / 128.0f;
        float f4 = (float)(n2 * 8 + 0) / 128.0f;
        float f5 = (float)(n2 * 8 + 8) / 128.0f;
        float f6 = 1.0f;
        float f7 = 0.5f;
        float f8 = 0.5f;
        GL11.glRotatef(180.0f - this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-this.dispatcher.pitch, 1.0f, 0.0f, 0.0f);
        tessellator.startQuads();
        tessellator.normal(0.0f, 1.0f, 0.0f);
        tessellator.vertex(0.0f - f7, 0.0f - f8, 0.0, f2, f5);
        tessellator.vertex(f6 - f7, 0.0f - f8, 0.0, f3, f5);
        tessellator.vertex(f6 - f7, 1.0f - f8, 0.0, f3, f4);
        tessellator.vertex(0.0f - f7, 1.0f - f8, 0.0, f2, f4);
        tessellator.draw();
        GL11.glDisable(32826);
        GL11.glPopMatrix();
        if (arg.owner != null) {
            float f9 = (arg.owner.prevYaw + (arg.owner.yaw - arg.owner.prevYaw) * h) * (float)Math.PI / 180.0f;
            double d2 = MathHelper.sin(f9);
            double d3 = MathHelper.cos(f9);
            float f10 = arg.owner.getHandSwingProgress(h);
            float f11 = MathHelper.sin(MathHelper.sqrt(f10) * (float)Math.PI);
            Vec3d vec3d = Vec3d.createCached(-0.5, 0.03, 0.8);
            vec3d.rotateX(-(arg.owner.prevPitch + (arg.owner.pitch - arg.owner.prevPitch) * h) * (float)Math.PI / 180.0f);
            vec3d.rotateY(-(arg.owner.prevYaw + (arg.owner.yaw - arg.owner.prevYaw) * h) * (float)Math.PI / 180.0f);
            vec3d.rotateY(f11 * 0.5f);
            vec3d.rotateX(-f11 * 0.7f);
            double d4 = arg.owner.prevX + (arg.owner.x - arg.owner.prevX) * (double)h + vec3d.x;
            double d5 = arg.owner.prevY + (arg.owner.y - arg.owner.prevY) * (double)h + vec3d.y;
            double d6 = arg.owner.prevZ + (arg.owner.z - arg.owner.prevZ) * (double)h + vec3d.z;
            if (this.dispatcher.options.thirdPerson) {
                f9 = (arg.owner.lastBodyYaw + (arg.owner.bodyYaw - arg.owner.lastBodyYaw) * h) * (float)Math.PI / 180.0f;
                d2 = MathHelper.sin(f9);
                d3 = MathHelper.cos(f9);
                d4 = arg.owner.prevX + (arg.owner.x - arg.owner.prevX) * (double)h - d3 * 0.35 - d2 * 0.85;
                d5 = arg.owner.prevY + (arg.owner.y - arg.owner.prevY) * (double)h - 0.45;
                d6 = arg.owner.prevZ + (arg.owner.z - arg.owner.prevZ) * (double)h - d2 * 0.35 + d3 * 0.85;
            }
            double d7 = arg.prevX + (arg.x - arg.prevX) * (double)h;
            double d8 = arg.prevY + (arg.y - arg.prevY) * (double)h + 0.25;
            double d9 = arg.prevZ + (arg.z - arg.prevZ) * (double)h;
            double d10 = (float)(d4 - d7);
            double d11 = (float)(d5 - d8);
            double d12 = (float)(d6 - d9);
            GL11.glDisable(3553);
            GL11.glDisable(2896);
            tessellator.start(GL11.GL_LINE_STRIP);
            tessellator.color(0);
            int n3 = 16;
            for (int i = 0; i <= n3; ++i) {
                float f12 = (float)i / (float)n3;
                tessellator.vertex(d + d10 * (double)f12, e + d11 * (double)(f12 * f12 + f12) * 0.5 + 0.25, f + d12 * (double)f12);
            }
            tessellator.draw();
            GL11.glEnable(2896);
            GL11.glEnable(3553);
        }
    }
}

