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
import net.minecraft.block.Block;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.render.entity.model.MinecartEntityModel;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class MinecartEntityRenderer
extends EntityRenderer {
    protected EntityModel model;

    public MinecartEntityRenderer() {
        this.shadowRadius = 0.5f;
        this.model = new MinecartEntityModel();
    }

    public void render(MinecartEntity arg, double d, double e, double f, float g, float h) {
        GL11.glPushMatrix();
        double d2 = arg.lastTickX + (arg.x - arg.lastTickX) * (double)h;
        double d3 = arg.lastTickY + (arg.y - arg.lastTickY) * (double)h;
        double d4 = arg.lastTickZ + (arg.z - arg.lastTickZ) * (double)h;
        double d5 = 0.3f;
        Vec3d vec3d = arg.snapPositionToRail(d2, d3, d4);
        float f2 = arg.prevPitch + (arg.pitch - arg.prevPitch) * h;
        if (vec3d != null) {
            Vec3d vec3d2 = arg.snapPositionToRailWithOffset(d2, d3, d4, d5);
            Vec3d vec3d3 = arg.snapPositionToRailWithOffset(d2, d3, d4, -d5);
            if (vec3d2 == null) {
                vec3d2 = vec3d;
            }
            if (vec3d3 == null) {
                vec3d3 = vec3d;
            }
            d += vec3d.x - d2;
            e += (vec3d2.y + vec3d3.y) / 2.0 - d3;
            f += vec3d.z - d4;
            Vec3d vec3d4 = vec3d3.add(-vec3d2.x, -vec3d2.y, -vec3d2.z);
            if (vec3d4.length() != 0.0) {
                vec3d4 = vec3d4.normalize();
                g = (float)(Math.atan2(vec3d4.z, vec3d4.x) * 180.0 / Math.PI);
                f2 = (float)(Math.atan(vec3d4.y) * 73.0);
            }
        }
        GL11.glTranslatef((float)d, (float)e, (float)f);
        GL11.glRotatef(180.0f - g, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-f2, 0.0f, 0.0f, 1.0f);
        float f3 = (float)arg.damageWobbleTicks - h;
        float f4 = (float)arg.damageWobbleStrength - h;
        if (f4 < 0.0f) {
            f4 = 0.0f;
        }
        if (f3 > 0.0f) {
            GL11.glRotatef(MathHelper.sin(f3) * f3 * f4 / 10.0f * (float)arg.damageWobbleSide, 1.0f, 0.0f, 0.0f);
        }
        if (arg.type != 0) {
            this.bindTexture("/terrain.png");
            float f5 = 0.75f;
            GL11.glScalef(f5, f5, f5);
            GL11.glTranslatef(0.0f, 0.3125f, 0.0f);
            GL11.glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            if (arg.type == 1) {
                new BlockRenderManager().render(Block.CHEST, 0, arg.getBrightnessAtEyes(h));
            } else if (arg.type == 2) {
                new BlockRenderManager().render(Block.FURNACE, 0, arg.getBrightnessAtEyes(h));
            }
            GL11.glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
            GL11.glTranslatef(0.0f, -0.3125f, 0.0f);
            GL11.glScalef(1.0f / f5, 1.0f / f5, 1.0f / f5);
        }
        this.bindTexture("/item/cart.png");
        GL11.glScalef(-1.0f, -1.0f, 1.0f);
        this.model.render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
        GL11.glPopMatrix();
    }
}

