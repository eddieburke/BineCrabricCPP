/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import java.nio.FloatBuffer;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.FrustumData;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class Frustum
extends FrustumData {
    private static Frustum INSTANCE = new Frustum();
    private FloatBuffer projectionBuffer = GlAllocationUtils.allocateFloatBuffer(16);
    private FloatBuffer modelBuffer = GlAllocationUtils.allocateFloatBuffer(16);
    private FloatBuffer clipBuffer = GlAllocationUtils.allocateFloatBuffer(16);

    public static FrustumData getInstance() {
        INSTANCE.compute();
        return INSTANCE;
    }

    private void normalize(float[][] frustum, int face) {
        float f = MathHelper.sqrt(frustum[face][0] * frustum[face][0] + frustum[face][1] * frustum[face][1] + frustum[face][2] * frustum[face][2]);
        float[] fArray = frustum[face];
        fArray[0] = fArray[0] / f;
        float[] fArray2 = frustum[face];
        fArray2[1] = fArray2[1] / f;
        float[] fArray3 = frustum[face];
        fArray3[2] = fArray3[2] / f;
        float[] fArray4 = frustum[face];
        fArray4[3] = fArray4[3] / f;
    }

    private void compute() {
        this.projectionBuffer.clear();
        this.modelBuffer.clear();
        this.clipBuffer.clear();
        GL11.glGetFloat(2983, this.projectionBuffer);
        GL11.glGetFloat(2982, this.modelBuffer);
        this.projectionBuffer.flip().limit(16);
        this.projectionBuffer.get(this.projectionMatrix);
        this.modelBuffer.flip().limit(16);
        this.modelBuffer.get(this.modelMatrix);
        this.clipMatrix[0] = this.modelMatrix[0] * this.projectionMatrix[0] + this.modelMatrix[1] * this.projectionMatrix[4] + this.modelMatrix[2] * this.projectionMatrix[8] + this.modelMatrix[3] * this.projectionMatrix[12];
        this.clipMatrix[1] = this.modelMatrix[0] * this.projectionMatrix[1] + this.modelMatrix[1] * this.projectionMatrix[5] + this.modelMatrix[2] * this.projectionMatrix[9] + this.modelMatrix[3] * this.projectionMatrix[13];
        this.clipMatrix[2] = this.modelMatrix[0] * this.projectionMatrix[2] + this.modelMatrix[1] * this.projectionMatrix[6] + this.modelMatrix[2] * this.projectionMatrix[10] + this.modelMatrix[3] * this.projectionMatrix[14];
        this.clipMatrix[3] = this.modelMatrix[0] * this.projectionMatrix[3] + this.modelMatrix[1] * this.projectionMatrix[7] + this.modelMatrix[2] * this.projectionMatrix[11] + this.modelMatrix[3] * this.projectionMatrix[15];
        this.clipMatrix[4] = this.modelMatrix[4] * this.projectionMatrix[0] + this.modelMatrix[5] * this.projectionMatrix[4] + this.modelMatrix[6] * this.projectionMatrix[8] + this.modelMatrix[7] * this.projectionMatrix[12];
        this.clipMatrix[5] = this.modelMatrix[4] * this.projectionMatrix[1] + this.modelMatrix[5] * this.projectionMatrix[5] + this.modelMatrix[6] * this.projectionMatrix[9] + this.modelMatrix[7] * this.projectionMatrix[13];
        this.clipMatrix[6] = this.modelMatrix[4] * this.projectionMatrix[2] + this.modelMatrix[5] * this.projectionMatrix[6] + this.modelMatrix[6] * this.projectionMatrix[10] + this.modelMatrix[7] * this.projectionMatrix[14];
        this.clipMatrix[7] = this.modelMatrix[4] * this.projectionMatrix[3] + this.modelMatrix[5] * this.projectionMatrix[7] + this.modelMatrix[6] * this.projectionMatrix[11] + this.modelMatrix[7] * this.projectionMatrix[15];
        this.clipMatrix[8] = this.modelMatrix[8] * this.projectionMatrix[0] + this.modelMatrix[9] * this.projectionMatrix[4] + this.modelMatrix[10] * this.projectionMatrix[8] + this.modelMatrix[11] * this.projectionMatrix[12];
        this.clipMatrix[9] = this.modelMatrix[8] * this.projectionMatrix[1] + this.modelMatrix[9] * this.projectionMatrix[5] + this.modelMatrix[10] * this.projectionMatrix[9] + this.modelMatrix[11] * this.projectionMatrix[13];
        this.clipMatrix[10] = this.modelMatrix[8] * this.projectionMatrix[2] + this.modelMatrix[9] * this.projectionMatrix[6] + this.modelMatrix[10] * this.projectionMatrix[10] + this.modelMatrix[11] * this.projectionMatrix[14];
        this.clipMatrix[11] = this.modelMatrix[8] * this.projectionMatrix[3] + this.modelMatrix[9] * this.projectionMatrix[7] + this.modelMatrix[10] * this.projectionMatrix[11] + this.modelMatrix[11] * this.projectionMatrix[15];
        this.clipMatrix[12] = this.modelMatrix[12] * this.projectionMatrix[0] + this.modelMatrix[13] * this.projectionMatrix[4] + this.modelMatrix[14] * this.projectionMatrix[8] + this.modelMatrix[15] * this.projectionMatrix[12];
        this.clipMatrix[13] = this.modelMatrix[12] * this.projectionMatrix[1] + this.modelMatrix[13] * this.projectionMatrix[5] + this.modelMatrix[14] * this.projectionMatrix[9] + this.modelMatrix[15] * this.projectionMatrix[13];
        this.clipMatrix[14] = this.modelMatrix[12] * this.projectionMatrix[2] + this.modelMatrix[13] * this.projectionMatrix[6] + this.modelMatrix[14] * this.projectionMatrix[10] + this.modelMatrix[15] * this.projectionMatrix[14];
        this.clipMatrix[15] = this.modelMatrix[12] * this.projectionMatrix[3] + this.modelMatrix[13] * this.projectionMatrix[7] + this.modelMatrix[14] * this.projectionMatrix[11] + this.modelMatrix[15] * this.projectionMatrix[15];
        this.frustum[0][0] = this.clipMatrix[3] - this.clipMatrix[0];
        this.frustum[0][1] = this.clipMatrix[7] - this.clipMatrix[4];
        this.frustum[0][2] = this.clipMatrix[11] - this.clipMatrix[8];
        this.frustum[0][3] = this.clipMatrix[15] - this.clipMatrix[12];
        this.normalize(this.frustum, 0);
        this.frustum[1][0] = this.clipMatrix[3] + this.clipMatrix[0];
        this.frustum[1][1] = this.clipMatrix[7] + this.clipMatrix[4];
        this.frustum[1][2] = this.clipMatrix[11] + this.clipMatrix[8];
        this.frustum[1][3] = this.clipMatrix[15] + this.clipMatrix[12];
        this.normalize(this.frustum, 1);
        this.frustum[2][0] = this.clipMatrix[3] + this.clipMatrix[1];
        this.frustum[2][1] = this.clipMatrix[7] + this.clipMatrix[5];
        this.frustum[2][2] = this.clipMatrix[11] + this.clipMatrix[9];
        this.frustum[2][3] = this.clipMatrix[15] + this.clipMatrix[13];
        this.normalize(this.frustum, 2);
        this.frustum[3][0] = this.clipMatrix[3] - this.clipMatrix[1];
        this.frustum[3][1] = this.clipMatrix[7] - this.clipMatrix[5];
        this.frustum[3][2] = this.clipMatrix[11] - this.clipMatrix[9];
        this.frustum[3][3] = this.clipMatrix[15] - this.clipMatrix[13];
        this.normalize(this.frustum, 3);
        this.frustum[4][0] = this.clipMatrix[3] - this.clipMatrix[2];
        this.frustum[4][1] = this.clipMatrix[7] - this.clipMatrix[6];
        this.frustum[4][2] = this.clipMatrix[11] - this.clipMatrix[10];
        this.frustum[4][3] = this.clipMatrix[15] - this.clipMatrix[14];
        this.normalize(this.frustum, 4);
        this.frustum[5][0] = this.clipMatrix[3] + this.clipMatrix[2];
        this.frustum[5][1] = this.clipMatrix[7] + this.clipMatrix[6];
        this.frustum[5][2] = this.clipMatrix[11] + this.clipMatrix[10];
        this.frustum[5][3] = this.clipMatrix[15] + this.clipMatrix[14];
        this.normalize(this.frustum, 5);
    }
}

