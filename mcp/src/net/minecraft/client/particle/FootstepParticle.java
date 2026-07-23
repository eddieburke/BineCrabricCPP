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
import net.minecraft.client.particle.Particle;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class FootstepParticle
extends Particle {
    private int footstepAge = 0;
    private int footstepMaxAge = 0;
    private TextureManager textureManager;

    public FootstepParticle(TextureManager textureManager, World world, double x, double y, double z) {
        super(world, x, y, z, 0.0, 0.0, 0.0);
        this.textureManager = textureManager;
        this.velocityZ = 0.0;
        this.velocityY = 0.0;
        this.velocityX = 0.0;
        this.footstepMaxAge = 200;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f;
        float f2 = ((float)this.footstepAge + partialTicks) / (float)this.footstepMaxAge;
        if ((f = 2.0f - (f2 *= f2) * 2.0f) > 1.0f) {
            f = 1.0f;
        }
        f *= 0.2f;
        GL11.glDisable(2896);
        float f3 = 0.125f;
        float f4 = (float)(this.x - xOffset);
        float f5 = (float)(this.y - yOffset);
        float f6 = (float)(this.z - zOffset);
        float f7 = this.world.getLightBrightness(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z));
        this.textureManager.bindTexture(this.textureManager.getTextureId("/misc/footprint.png"));
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        tessellator.startQuads();
        tessellator.color(f7, f7, f7, f);
        tessellator.vertex(f4 - f3, f5, f6 + f3, 0.0, 1.0);
        tessellator.vertex(f4 + f3, f5, f6 + f3, 1.0, 1.0);
        tessellator.vertex(f4 + f3, f5, f6 - f3, 1.0, 0.0);
        tessellator.vertex(f4 - f3, f5, f6 - f3, 0.0, 0.0);
        tessellator.draw();
        GL11.glDisable(3042);
        GL11.glEnable(2896);
    }

    public void tick() {
        ++this.footstepAge;
        if (this.footstepAge == this.footstepMaxAge) {
            this.markDead();
        }
    }

    public int getGroup() {
        return 3;
    }
}

