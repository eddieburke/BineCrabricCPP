/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class FrustumData {
    public float[][] frustum = new float[16][16];
    public float[] projectionMatrix = new float[16];
    public float[] modelMatrix = new float[16];
    public float[] clipMatrix = new float[16];

    public boolean intersects(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
        for (int i = 0; i < 6; ++i) {
            if ((double)this.frustum[i][0] * minX + (double)this.frustum[i][1] * minY + (double)this.frustum[i][2] * minZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * maxX + (double)this.frustum[i][1] * minY + (double)this.frustum[i][2] * minZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * minX + (double)this.frustum[i][1] * maxY + (double)this.frustum[i][2] * minZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * maxX + (double)this.frustum[i][1] * maxY + (double)this.frustum[i][2] * minZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * minX + (double)this.frustum[i][1] * minY + (double)this.frustum[i][2] * maxZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * maxX + (double)this.frustum[i][1] * minY + (double)this.frustum[i][2] * maxZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * minX + (double)this.frustum[i][1] * maxY + (double)this.frustum[i][2] * maxZ + (double)this.frustum[i][3] > 0.0 || (double)this.frustum[i][0] * maxX + (double)this.frustum[i][1] * maxY + (double)this.frustum[i][2] * maxZ + (double)this.frustum[i][3] > 0.0) continue;
            return false;
        }
        return true;
    }
}

