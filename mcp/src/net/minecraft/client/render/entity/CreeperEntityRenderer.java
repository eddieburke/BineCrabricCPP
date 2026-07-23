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
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.CreeperEntityModel;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.entity.mob.CreeperEntity;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class CreeperEntityRenderer
extends LivingEntityRenderer {
    private EntityModel model = new CreeperEntityModel(2.0f);

    public CreeperEntityRenderer() {
        super(new CreeperEntityModel(), 0.5f);
    }

    protected void applyScale(CreeperEntity arg, float f) {
        CreeperEntity creeperEntity = arg;
        float f2 = creeperEntity.getScale(f);
        float f3 = 1.0f + MathHelper.sin(f2 * 100.0f) * f2 * 0.01f;
        if (f2 < 0.0f) {
            f2 = 0.0f;
        }
        if (f2 > 1.0f) {
            f2 = 1.0f;
        }
        f2 *= f2;
        f2 *= f2;
        float f4 = (1.0f + f2 * 0.4f) * f3;
        float f5 = (1.0f + f2 * 0.1f) / f3;
        GL11.glScalef(f4, f5, f4);
    }

    protected int getOverlayColor(CreeperEntity arg, float f, float g) {
        CreeperEntity creeperEntity = arg;
        float f2 = creeperEntity.getScale(g);
        if ((int)(f2 * 10.0f) % 2 == 0) {
            return 0;
        }
        int n = (int)(f2 * 0.2f * 255.0f);
        if (n < 0) {
            n = 0;
        }
        if (n > 255) {
            n = 255;
        }
        int n2 = 255;
        int n3 = 255;
        int n4 = 255;
        return n << 24 | n2 << 16 | n3 << 8 | n4;
    }

    protected boolean bindTexture(CreeperEntity arg, int i, float f) {
        if (arg.isCharged()) {
            if (i == 1) {
                float f2 = (float)arg.age + f;
                this.bindTexture("/armor/power.png");
                GL11.glMatrixMode(5890);
                GL11.glLoadIdentity();
                float f3 = f2 * 0.01f;
                float f4 = f2 * 0.01f;
                GL11.glTranslatef(f3, f4, 0.0f);
                this.setDecorationModel(this.model);
                GL11.glMatrixMode(5888);
                GL11.glEnable(3042);
                float f5 = 0.5f;
                GL11.glColor4f(f5, f5, f5, 1.0f);
                GL11.glDisable(2896);
                GL11.glBlendFunc(1, 1);
                return true;
            }
            if (i == 2) {
                GL11.glMatrixMode(5890);
                GL11.glLoadIdentity();
                GL11.glMatrixMode(5888);
                GL11.glEnable(2896);
                GL11.glDisable(3042);
            }
        }
        return false;
    }

    protected boolean bindDecorationTexture(CreeperEntity arg, int i, float f) {
        return false;
    }
}

