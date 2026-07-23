/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block.entity;

import java.util.HashMap;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.MobSpawnerBlockEntity;
import net.minecraft.block.entity.PistonBlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.block.entity.BlockEntityRenderer;
import net.minecraft.client.render.block.entity.MobSpawnerBlockEntityRenderer;
import net.minecraft.client.render.block.entity.PistonBlockEntityRenderer;
import net.minecraft.client.render.block.entity.SignBlockEntityRenderer;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.LivingEntity;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class BlockEntityRenderDispatcher {
    private Map renderers = new HashMap();
    public static BlockEntityRenderDispatcher INSTANCE = new BlockEntityRenderDispatcher();
    private TextRenderer textRenderer;
    public static double offsetX;
    public static double offsetY;
    public static double offsetZ;
    public TextureManager textureManager;
    public World world;
    public LivingEntity camera;
    public float cameraYaw;
    public float cameraPitch;
    public double cameraX;
    public double cameraY;
    public double cameraZ;

    private BlockEntityRenderDispatcher() {
        this.renderers.put(SignBlockEntity.class, new SignBlockEntityRenderer());
        this.renderers.put(MobSpawnerBlockEntity.class, new MobSpawnerBlockEntityRenderer());
        this.renderers.put(PistonBlockEntity.class, new PistonBlockEntityRenderer());
        for (BlockEntityRenderer blockEntityRenderer : this.renderers.values()) {
            blockEntityRenderer.setDispatcher(this);
        }
    }

    public BlockEntityRenderer getRenderer(Class type) {
        BlockEntityRenderer blockEntityRenderer = (BlockEntityRenderer)this.renderers.get(type);
        if (blockEntityRenderer == null && type != BlockEntity.class) {
            blockEntityRenderer = this.getRenderer(type.getSuperclass());
            this.renderers.put(type, blockEntityRenderer);
        }
        return blockEntityRenderer;
    }

    public boolean hasRenderer(BlockEntity blockEntity) {
        return this.getRenderer(blockEntity) != null;
    }

    public BlockEntityRenderer getRenderer(BlockEntity blockEntity) {
        if (blockEntity == null) {
            return null;
        }
        return this.getRenderer(blockEntity.getClass());
    }

    public void prepare(World world, TextureManager textureManager, TextRenderer textRenderer, LivingEntity camera, float tickDelta) {
        if (this.world != world) {
            this.setWorld(world);
        }
        this.textureManager = textureManager;
        this.camera = camera;
        this.textRenderer = textRenderer;
        this.cameraYaw = camera.prevYaw + (camera.yaw - camera.prevYaw) * tickDelta;
        this.cameraPitch = camera.prevPitch + (camera.pitch - camera.prevPitch) * tickDelta;
        this.cameraX = camera.lastTickX + (camera.x - camera.lastTickX) * (double)tickDelta;
        this.cameraY = camera.lastTickY + (camera.y - camera.lastTickY) * (double)tickDelta;
        this.cameraZ = camera.lastTickZ + (camera.z - camera.lastTickZ) * (double)tickDelta;
    }

    public void render(BlockEntity blockEntity, float tickDelta) {
        if (blockEntity.distanceFrom(this.cameraX, this.cameraY, this.cameraZ) < 4096.0) {
            float f = this.world.getLightBrightness(blockEntity.x, blockEntity.y, blockEntity.z);
            GL11.glColor3f(f, f, f);
            this.render(blockEntity, (double)blockEntity.x - offsetX, (double)blockEntity.y - offsetY, (double)blockEntity.z - offsetZ, tickDelta);
        }
    }

    public void render(BlockEntity blockEntity, double dx, double dy, double dz, float tickDelta) {
        BlockEntityRenderer blockEntityRenderer = this.getRenderer(blockEntity);
        if (blockEntityRenderer != null) {
            blockEntityRenderer.render(blockEntity, dx, dy, dz, tickDelta);
        }
    }

    public void setWorld(World world) {
        this.world = world;
        for (BlockEntityRenderer blockEntityRenderer : this.renderers.values()) {
            if (blockEntityRenderer == null) continue;
            blockEntityRenderer.setWorld(world);
        }
    }

    public TextRenderer getTextRenderer() {
        return this.textRenderer;
    }
}

