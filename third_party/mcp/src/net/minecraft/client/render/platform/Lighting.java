/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.platform;

import java.nio.FloatBuffer;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.util.math.Vec3d;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class Lighting {
    private static FloatBuffer BUFFER = GlAllocationUtils.allocateFloatBuffer(16);

    public static void turnOff() {
        GL11.glDisable(2896);
        GL11.glDisable(16384);
        GL11.glDisable(16385);
        GL11.glDisable(2903);
    }

    public static void turnOn() {
        GL11.glEnable(2896);
        GL11.glEnable(16384);
        GL11.glEnable(16385);
        GL11.glEnable(2903);
        GL11.glColorMaterial(1032, 5634);
        float f = 0.4f;
        float f2 = 0.6f;
        float f3 = 0.0f;
        Vec3d vec3d = Vec3d.createCached(0.2f, 1.0, -0.7f).normalize();
        GL11.glLight(16384, 4611, Lighting.getBuffer(vec3d.x, vec3d.y, vec3d.z, 0.0));
        GL11.glLight(16384, 4609, Lighting.getBuffer(f2, f2, f2, 1.0f));
        GL11.glLight(16384, 4608, Lighting.getBuffer(0.0f, 0.0f, 0.0f, 1.0f));
        GL11.glLight(16384, 4610, Lighting.getBuffer(f3, f3, f3, 1.0f));
        vec3d = Vec3d.createCached(-0.2f, 1.0, 0.7f).normalize();
        GL11.glLight(16385, 4611, Lighting.getBuffer(vec3d.x, vec3d.y, vec3d.z, 0.0));
        GL11.glLight(16385, 4609, Lighting.getBuffer(f2, f2, f2, 1.0f));
        GL11.glLight(16385, 4608, Lighting.getBuffer(0.0f, 0.0f, 0.0f, 1.0f));
        GL11.glLight(16385, 4610, Lighting.getBuffer(f3, f3, f3, 1.0f));
        GL11.glShadeModel(7424);
        GL11.glLightModel(2899, Lighting.getBuffer(f, f, f, 1.0f));
    }

    private static FloatBuffer getBuffer(double r, double g, double b, double a) {
        return Lighting.getBuffer((float)r, (float)g, (float)b, (float)a);
    }

    private static FloatBuffer getBuffer(float r, float g, float b, float a) {
        BUFFER.clear();
        BUFFER.put(r).put(g).put(b).put(a);
        BUFFER.flip();
        return BUFFER;
    }
}

