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
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public abstract class EntityRenderer {
    protected EntityRenderDispatcher dispatcher;
    private EntityModel bipedModel = new BipedEntityModel();
    private BlockRenderManager renderManager = new BlockRenderManager();
    protected float shadowRadius = 0.0f;
    protected float shadowDarkness = 1.0f;

    public abstract void render(Entity var1, double var2, double var4, double var6, float var8, float var9);

    protected void bindTexture(String texturePath) {
        TextureManager textureManager = this.dispatcher.textureManager;
        textureManager.bindTexture(textureManager.getTextureId(texturePath));
    }

    protected boolean bindDownloadedTexture(String url, String backup) {
        TextureManager textureManager = this.dispatcher.textureManager;
        int n = textureManager.downloadTexture(url, backup);
        if (n >= 0) {
            textureManager.bindTexture(n);
            return true;
        }
        return false;
    }

    private void renderOnFire(Entity entity, double dx, double dy, double dz, float tickDelta) {
        GL11.glDisable(2896);
        int n = Block.FIRE.textureId;
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        float f = (float)n2 / 256.0f;
        float f2 = ((float)n2 + 15.99f) / 256.0f;
        float f3 = (float)n3 / 256.0f;
        float f4 = ((float)n3 + 15.99f) / 256.0f;
        GL11.glPushMatrix();
        GL11.glTranslatef((float)dx, (float)dy, (float)dz);
        float f5 = entity.width * 1.4f;
        GL11.glScalef(f5, f5, f5);
        this.bindTexture("/terrain.png");
        Tessellator tessellator = Tessellator.INSTANCE;
        float f6 = 0.5f;
        float f7 = 0.0f;
        float f8 = entity.height / f5;
        float f9 = (float)(entity.y - entity.boundingBox.minY);
        GL11.glRotatef(-this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
        GL11.glTranslatef(0.0f, 0.0f, -0.3f + (float)((int)f8) * 0.02f);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        float f10 = 0.0f;
        int n4 = 0;
        tessellator.startQuads();
        while (f8 > 0.0f) {
            if (n4 % 2 == 0) {
                f = (float)n2 / 256.0f;
                f2 = ((float)n2 + 15.99f) / 256.0f;
                f3 = (float)n3 / 256.0f;
                f4 = ((float)n3 + 15.99f) / 256.0f;
            } else {
                f = (float)n2 / 256.0f;
                f2 = ((float)n2 + 15.99f) / 256.0f;
                f3 = (float)(n3 + 16) / 256.0f;
                f4 = ((float)(n3 + 16) + 15.99f) / 256.0f;
            }
            if (n4 / 2 % 2 == 0) {
                float f11 = f2;
                f2 = f;
                f = f11;
            }
            tessellator.vertex(f6 - f7, 0.0f - f9, f10, f2, f4);
            tessellator.vertex(-f6 - f7, 0.0f - f9, f10, f, f4);
            tessellator.vertex(-f6 - f7, 1.4f - f9, f10, f, f3);
            tessellator.vertex(f6 - f7, 1.4f - f9, f10, f2, f3);
            f8 -= 0.45f;
            f9 -= 0.45f;
            f6 *= 0.9f;
            f10 += 0.03f;
            ++n4;
        }
        tessellator.draw();
        GL11.glPopMatrix();
        GL11.glEnable(2896);
    }

    private void renderShadow(Entity entity, double dx, double dy, double dz, float yaw, float tickDelta) {
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        TextureManager textureManager = this.dispatcher.textureManager;
        textureManager.bindTexture(textureManager.getTextureId("%clamp%/misc/shadow.png"));
        World world = this.getWorld();
        GL11.glDepthMask(false);
        float f = this.shadowRadius;
        double d = entity.lastTickX + (entity.x - entity.lastTickX) * (double)tickDelta;
        double d2 = entity.lastTickY + (entity.y - entity.lastTickY) * (double)tickDelta + (double)entity.getShadowRadius();
        double d3 = entity.lastTickZ + (entity.z - entity.lastTickZ) * (double)tickDelta;
        int n = MathHelper.floor(d - (double)f);
        int n2 = MathHelper.floor(d + (double)f);
        int n3 = MathHelper.floor(d2 - (double)f);
        int n4 = MathHelper.floor(d2);
        int n5 = MathHelper.floor(d3 - (double)f);
        int n6 = MathHelper.floor(d3 + (double)f);
        double d4 = dx - d;
        double d5 = dy - d2;
        double d6 = dz - d3;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        for (int i = n; i <= n2; ++i) {
            for (int j = n3; j <= n4; ++j) {
                for (int k = n5; k <= n6; ++k) {
                    int n7 = world.getBlockId(i, j - 1, k);
                    if (n7 <= 0 || world.getLightLevel(i, j, k) <= 3) continue;
                    this.renderShadowOnBlock(Block.BLOCKS[n7], dx, dy + (double)entity.getShadowRadius(), dz, i, j, k, yaw, f, d4, d5 + (double)entity.getShadowRadius(), d6);
                }
            }
        }
        tessellator.draw();
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
        GL11.glDepthMask(true);
    }

    private World getWorld() {
        return this.dispatcher.world;
    }

    private void renderShadowOnBlock(Block block, double dx, double dy, double dz, int x, int y, int z, float yaw, float shadowSize, double cx, double cy, double cz) {
        Tessellator tessellator = Tessellator.INSTANCE;
        if (!block.isFullCube()) {
            return;
        }
        double d = ((double)yaw - (dy - ((double)y + cy)) / 2.0) * 0.5 * (double)this.getWorld().getLightBrightness(x, y, z);
        if (d < 0.0) {
            return;
        }
        if (d > 1.0) {
            d = 1.0;
        }
        tessellator.color(1.0f, 1.0f, 1.0f, (float)d);
        double d2 = (double)x + block.minX + cx;
        double d3 = (double)x + block.maxX + cx;
        double d4 = (double)y + block.minY + cy + 0.015625;
        double d5 = (double)z + block.minZ + cz;
        double d6 = (double)z + block.maxZ + cz;
        float f = (float)((dx - d2) / 2.0 / (double)shadowSize + 0.5);
        float f2 = (float)((dx - d3) / 2.0 / (double)shadowSize + 0.5);
        float f3 = (float)((dz - d5) / 2.0 / (double)shadowSize + 0.5);
        float f4 = (float)((dz - d6) / 2.0 / (double)shadowSize + 0.5);
        tessellator.vertex(d2, d4, d5, f, f3);
        tessellator.vertex(d2, d4, d6, f, f4);
        tessellator.vertex(d3, d4, d6, f2, f4);
        tessellator.vertex(d3, d4, d5, f2, f3);
    }

    public static void renderShape(Box box, double x, double y, double z) {
        GL11.glDisable(3553);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        tessellator.startQuads();
        tessellator.translate(x, y, z);
        tessellator.normal(0.0f, 0.0f, -1.0f);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.normal(0.0f, 0.0f, 1.0f);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.normal(0.0f, -1.0f, 0.0f);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.normal(0.0f, 1.0f, 0.0f);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.normal(-1.0f, 0.0f, 0.0f);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.normal(1.0f, 0.0f, 0.0f);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.translate(0.0, 0.0, 0.0);
        tessellator.draw();
        GL11.glEnable(3553);
    }

    public static void renderShapeFlat(Box box) {
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.draw();
    }

    public void setDispatcher(EntityRenderDispatcher dispatcher) {
        this.dispatcher = dispatcher;
    }

    public void postRender(Entity entity, double dx, double dy, double dz, float yaw, float tickDelta) {
        double d;
        float f;
        if (this.dispatcher.options.fancyGraphics && this.shadowRadius > 0.0f && (f = (float)((1.0 - (d = this.dispatcher.squaredDistanceTo(entity.x, entity.y, entity.z)) / 256.0) * (double)this.shadowDarkness)) > 0.0f) {
            this.renderShadow(entity, dx, dy, dz, f, tickDelta);
        }
        if (entity.isOnFire()) {
            this.renderOnFire(entity, dx, dy, dz, tickDelta);
        }
    }

    public TextRenderer getTextRenderer() {
        return this.dispatcher.getTextRenderer();
    }
}

