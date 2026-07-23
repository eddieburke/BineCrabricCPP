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
import net.minecraft.item.Item;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class ItemParticle
extends Particle {
    public ItemParticle(World world, double x, double y, double z, Item item) {
        super(world, x, y, z, 0.0, 0.0, 0.0);
        this.textureId = item.getTextureId(0);
        this.blue = 1.0f;
        this.green = 1.0f;
        this.red = 1.0f;
        this.gravityStrength = Block.SNOW_BLOCK.particleFallSpeedModifier;
        this.scale /= 2.0f;
    }

    public int getGroup() {
        return 2;
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

