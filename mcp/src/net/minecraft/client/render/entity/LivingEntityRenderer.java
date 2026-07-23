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
import net.minecraft.client.Minecraft;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class LivingEntityRenderer
extends EntityRenderer {
    protected EntityModel model;
    protected EntityModel decorationModel;

    public LivingEntityRenderer(EntityModel entityModel, float shadowRadius) {
        this.model = entityModel;
        this.shadowRadius = shadowRadius;
    }

    public void setDecorationModel(EntityModel model) {
        this.decorationModel = model;
    }

    public void render(LivingEntity arg, double d, double e, double f, float g, float h) {
        GL11.glPushMatrix();
        GL11.glDisable(2884);
        this.model.handSwingProgress = this.getHandSwingProgress(arg, h);
        if (this.decorationModel != null) {
            this.decorationModel.handSwingProgress = this.model.handSwingProgress;
        }
        this.model.riding = arg.hasVehicle();
        if (this.decorationModel != null) {
            this.decorationModel.riding = this.model.riding;
        }
        try {
            float f2 = arg.lastBodyYaw + (arg.bodyYaw - arg.lastBodyYaw) * h;
            float f3 = arg.prevYaw + (arg.yaw - arg.prevYaw) * h;
            float f4 = arg.prevPitch + (arg.pitch - arg.prevPitch) * h;
            this.applyTranslation(arg, d, e, f);
            float f5 = this.getHeadBob(arg, h);
            this.applyHandSwingRotation(arg, f5, f2, h);
            float f6 = 0.0625f;
            GL11.glEnable(32826);
            GL11.glScalef(-1.0f, -1.0f, 1.0f);
            this.applyScale(arg, h);
            GL11.glTranslatef(0.0f, -24.0f * f6 - 0.0078125f, 0.0f);
            float f7 = arg.lastWalkAnimationSpeed + (arg.walkAnimationSpeed - arg.lastWalkAnimationSpeed) * h;
            float f8 = arg.walkAnimationProgress - arg.walkAnimationSpeed * (1.0f - h);
            if (f7 > 1.0f) {
                f7 = 1.0f;
            }
            this.bindDownloadedTexture(arg.skinUrl, arg.getTexture());
            GL11.glEnable(3008);
            this.model.animateModel(arg, f8, f7, h);
            this.model.render(f8, f7, f5, f3 - f2, f4, f6);
            for (int i = 0; i < 4; ++i) {
                if (!this.bindTexture(arg, i, h)) continue;
                this.decorationModel.render(f8, f7, f5, f3 - f2, f4, f6);
                GL11.glDisable(3042);
                GL11.glEnable(3008);
            }
            this.renderMore(arg, h);
            float f9 = arg.getBrightnessAtEyes(h);
            int n = this.getOverlayColor(arg, f9, h);
            if ((n >> 24 & 0xFF) > 0 || arg.hurtTime > 0 || arg.deathTime > 0) {
                GL11.glDisable(3553);
                GL11.glDisable(3008);
                GL11.glEnable(3042);
                GL11.glBlendFunc(770, 771);
                GL11.glDepthFunc(514);
                if (arg.hurtTime > 0 || arg.deathTime > 0) {
                    GL11.glColor4f(f9, 0.0f, 0.0f, 0.4f);
                    this.model.render(f8, f7, f5, f3 - f2, f4, f6);
                    for (int i = 0; i < 4; ++i) {
                        if (!this.bindDecorationTexture(arg, i, h)) continue;
                        GL11.glColor4f(f9, 0.0f, 0.0f, 0.4f);
                        this.decorationModel.render(f8, f7, f5, f3 - f2, f4, f6);
                    }
                }
                if ((n >> 24 & 0xFF) > 0) {
                    float f10 = (float)(n >> 16 & 0xFF) / 255.0f;
                    float f11 = (float)(n >> 8 & 0xFF) / 255.0f;
                    float f12 = (float)(n & 0xFF) / 255.0f;
                    float f13 = (float)(n >> 24 & 0xFF) / 255.0f;
                    GL11.glColor4f(f10, f11, f12, f13);
                    this.model.render(f8, f7, f5, f3 - f2, f4, f6);
                    for (int i = 0; i < 4; ++i) {
                        if (!this.bindDecorationTexture(arg, i, h)) continue;
                        GL11.glColor4f(f10, f11, f12, f13);
                        this.decorationModel.render(f8, f7, f5, f3 - f2, f4, f6);
                    }
                }
                GL11.glDepthFunc(515);
                GL11.glDisable(3042);
                GL11.glEnable(3008);
                GL11.glEnable(3553);
            }
            GL11.glDisable(32826);
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        GL11.glEnable(2884);
        GL11.glPopMatrix();
        this.renderNameTag(arg, d, e, f);
    }

    protected void applyTranslation(LivingEntity entity, double headBob, double bodyYaw, double tickDelta) {
        GL11.glTranslatef((float)headBob, (float)bodyYaw, (float)tickDelta);
    }

    protected void applyHandSwingRotation(LivingEntity entity, float dx, float dy, float dz) {
        GL11.glRotatef(180.0f - dy, 0.0f, 1.0f, 0.0f);
        if (entity.deathTime > 0) {
            float f = ((float)entity.deathTime + dz - 1.0f) / 20.0f * 1.6f;
            if ((f = MathHelper.sqrt(f)) > 1.0f) {
                f = 1.0f;
            }
            GL11.glRotatef(f * this.getDeathYaw(entity), 0.0f, 0.0f, 1.0f);
        }
    }

    protected float getHandSwingProgress(LivingEntity entity, float tickDelta) {
        return entity.getHandSwingProgress(tickDelta);
    }

    protected float getHeadBob(LivingEntity entity, float tickDelta) {
        return (float)entity.age + tickDelta;
    }

    protected void renderMore(LivingEntity entity, float tickDelta) {
    }

    protected boolean bindDecorationTexture(LivingEntity entity, int i, float f) {
        return this.bindTexture(entity, i, f);
    }

    protected boolean bindTexture(LivingEntity mob, int layer, float tickDelta) {
        return false;
    }

    protected float getDeathYaw(LivingEntity entity) {
        return 90.0f;
    }

    protected int getOverlayColor(LivingEntity entity, float brightness, float timeDelta) {
        return 0;
    }

    protected void applyScale(LivingEntity entity, float scale) {
    }

    protected void renderNameTag(LivingEntity entity, double dx, double dy, double dz) {
        if (Minecraft.isDebugProfilerEnabled()) {
            this.renderNameTag(entity, Integer.toString(entity.id), dx, dy, dz, 64);
        }
    }

    protected void renderNameTag(LivingEntity entity, String name, double dx, double dy, double dz, int range) {
        float f = entity.getDistance(this.dispatcher.cameraEntity);
        if (f > (float)range) {
            return;
        }
        TextRenderer textRenderer = this.getTextRenderer();
        float f2 = 1.6f;
        float f3 = 0.016666668f * f2;
        GL11.glPushMatrix();
        GL11.glTranslatef((float)dx + 0.0f, (float)dy + 2.3f, (float)dz);
        GL11.glNormal3f(0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(this.dispatcher.pitch, 1.0f, 0.0f, 0.0f);
        GL11.glScalef(-f3, -f3, f3);
        GL11.glDisable(2896);
        GL11.glDepthMask(false);
        GL11.glDisable(2929);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = 0;
        if (name.equals("deadmau5")) {
            n = -10;
        }
        GL11.glDisable(3553);
        tessellator.startQuads();
        int n2 = textRenderer.getWidth(name) / 2;
        tessellator.color(0.0f, 0.0f, 0.0f, 0.25f);
        tessellator.vertex(-n2 - 1, -1 + n, 0.0);
        tessellator.vertex(-n2 - 1, 8 + n, 0.0);
        tessellator.vertex(n2 + 1, 8 + n, 0.0);
        tessellator.vertex(n2 + 1, -1 + n, 0.0);
        tessellator.draw();
        GL11.glEnable(3553);
        textRenderer.draw(name, -textRenderer.getWidth(name) / 2, n, 0x20FFFFFF);
        GL11.glEnable(2929);
        GL11.glDepthMask(true);
        textRenderer.draw(name, -textRenderer.getWidth(name) / 2, n, -1);
        GL11.glEnable(2896);
        GL11.glDisable(3042);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glPopMatrix();
    }
}

