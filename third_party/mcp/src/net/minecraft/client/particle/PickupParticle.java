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
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class PickupParticle
extends Particle {
    private Entity entity;
    private Entity collector;
    private int pickupAge = 0;
    private int lifetime = 0;
    private float offsetY;

    public PickupParticle(World world, Entity entity, Entity collector, float offsetY) {
        super(world, entity.x, entity.y, entity.z, entity.velocityX, entity.velocityY, entity.velocityZ);
        this.entity = entity;
        this.collector = collector;
        this.lifetime = 3;
        this.offsetY = offsetY;
    }

    public void render(Tessellator tessellator, float partialTicks, float horizontalSize, float verticalSize, float depthSize, float widthOffset, float heightOffset) {
        float f = ((float)this.pickupAge + partialTicks) / (float)this.lifetime;
        f *= f;
        double d = this.entity.x;
        double d2 = this.entity.y;
        double d3 = this.entity.z;
        double d4 = this.collector.lastTickX + (this.collector.x - this.collector.lastTickX) * (double)partialTicks;
        double d5 = this.collector.lastTickY + (this.collector.y - this.collector.lastTickY) * (double)partialTicks + (double)this.offsetY;
        double d6 = this.collector.lastTickZ + (this.collector.z - this.collector.lastTickZ) * (double)partialTicks;
        double d7 = d + (d4 - d) * (double)f;
        double d8 = d2 + (d5 - d2) * (double)f;
        double d9 = d3 + (d6 - d3) * (double)f;
        int n = MathHelper.floor(d7);
        int n2 = MathHelper.floor(d8 + (double)(this.standingEyeHeight / 2.0f));
        int n3 = MathHelper.floor(d9);
        float f2 = this.world.getLightBrightness(n, n2, n3);
        GL11.glColor4f(f2, f2, f2, 1.0f);
        EntityRenderDispatcher.INSTANCE.render(this.entity, (float)(d7 -= xOffset), (float)(d8 -= yOffset), (float)(d9 -= zOffset), this.entity.yaw, partialTicks);
    }

    public void tick() {
        ++this.pickupAge;
        if (this.pickupAge == this.lifetime) {
            this.markDead();
        }
    }

    public int getGroup() {
        return 3;
    }
}

