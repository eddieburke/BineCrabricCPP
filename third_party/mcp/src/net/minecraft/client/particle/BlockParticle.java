/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.particle;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.particle.Particle;
import net.minecraft.client.render.Tessellator;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class BlockParticle
extends Particle {
    private Block block;
    private int side = 0;

    public BlockParticle(World world, double x, double y, double z, double velocityX, double velocityY, double velocityZ, Block block, int side, int meta) {
        super(world, x, y, z, velocityX, velocityY, velocityZ);
        this.block = block;
        this.textureId = block.getTexture(0, meta);
        this.gravityStrength = block.particleFallSpeedModifier;
        this.blue = 0.6f;
        this.green = 0.6f;
        this.red = 0.6f;
        this.scale /= 2.0f;
        this.side = side;
    }

    public BlockParticle color(int x, int y, int z) {
        if (this.block == Block.GRASS_BLOCK) {
            return this;
        }
        int n = this.block.getColorMultiplier(this.world, x, y, z);
        this.red *= (float)(n >> 16 & 0xFF) / 255.0f;
        this.green *= (float)(n >> 8 & 0xFF) / 255.0f;
        this.blue *= (float)(n & 0xFF) / 255.0f;
        return this;
    }

    public int getGroup() {
        return 1;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)(this.textureId % 16) + this.prevU / 4.0f) / 16.0f;
        float f2 = f + 0.015609375f;
        float f3 = ((float)(this.textureId / 16) + this.prevV / 4.0f) / 16.0f;
        float f4 = f3 + 0.015609375f;
        float f5 = 0.1f * this.scale;
        float f6 = (float)(this.prevX + (this.x - this.prevX) * (double)partialTicks - xOffset);
        float f7 = (float)(this.prevY + (this.y - this.prevY) * (double)partialTicks - yOffset);
        float f8 = (float)(this.prevZ + (this.z - this.prevZ) * (double)partialTicks - zOffset);
        float f9 = this.getBrightnessAtEyes(partialTicks);
        tessellator.color(f9 * this.red, f9 * this.green, f9 * this.blue);
        tessellator.vertex(f6 - horizontalSize * f5 - widthOffset * f5, f7 - verticalSize * f5, f8 - depthSize * f5 - heightOffset * f5, f, f4);
        tessellator.vertex(f6 - horizontalSize * f5 + widthOffset * f5, f7 + verticalSize * f5, f8 - depthSize * f5 + heightOffset * f5, f, f3);
        tessellator.vertex(f6 + horizontalSize * f5 + widthOffset * f5, f7 + verticalSize * f5, f8 + depthSize * f5 + heightOffset * f5, f2, f3);
        tessellator.vertex(f6 + horizontalSize * f5 - widthOffset * f5, f7 - verticalSize * f5, f8 + depthSize * f5 - heightOffset * f5, f2, f4);
    }
}

