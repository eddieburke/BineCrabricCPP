/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import java.nio.FloatBuffer;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.client.Minecraft;
import net.minecraft.client.TestInteractionManager;
import net.minecraft.client.particle.FireSmokeParticle;
import net.minecraft.client.particle.ParticleManager;
import net.minecraft.client.particle.RainSplashParticle;
import net.minecraft.client.render.Frustum;
import net.minecraft.client.render.FrustumCuller;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.WorldRenderer;
import net.minecraft.client.render.item.HeldItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.client.util.ScreenScaler;
import net.minecraft.client.util.SmoothUtil;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.LegacyChunkCache;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.Display;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GLContext;
import org.lwjgl.util.glu.GLU;

@Environment(value=EnvType.CLIENT)
public class GameRenderer {
    public static boolean anaglyph3d = false;
    public static int activeEye;
    private Minecraft client;
    private float viewDistance = 0.0f;
    public HeldItemRenderer heldItemRenderer;
    private int ticks;
    private Entity targetedEntity = null;
    private SmoothUtil cinematicCameraYawSmoother = new SmoothUtil();
    private SmoothUtil cinematicCameraPitchSmoother = new SmoothUtil();
    private SmoothUtil yawSmoother = new SmoothUtil();
    private SmoothUtil pitchSmoother = new SmoothUtil();
    private SmoothUtil rollSmoother = new SmoothUtil();
    private SmoothUtil fovSmoother = new SmoothUtil();
    private float thirdPersonDistance = 4.0f;
    private float prevThirdPersonDistance = 4.0f;
    private float thirdPersonYaw = 0.0f;
    private float prevThirdPersonYaw = 0.0f;
    private float thirdPersonPitch = 0.0f;
    private float prevThirdPersonPitch = 0.0f;
    private float cameraRoll = 0.0f;
    private float prevCameraRoll = 0.0f;
    private float cameraRollAmount = 0.0f;
    private float prevCameraRollAmount = 0.0f;
    private boolean renderFog = false;
    private double zoom = 1.0;
    private double zoomX = 0.0;
    private double zoomY = 0.0;
    private long lastInactiveTime = System.currentTimeMillis();
    private long lastFrameTime = 0L;
    private Random random = new Random();
    private int rainSoundCounter = 0;
    volatile int leftEyeRenderPass = 0;
    volatile int rightEyeRenderPass = 0;
    FloatBuffer fogColorBuffer = GlAllocationUtils.allocateFloatBuffer(16);
    float fogRed;
    float fogGreen;
    float fogBlue;
    private float lastViewBob;
    private float viewBob;

    public GameRenderer(Minecraft client) {
        this.client = client;
        this.heldItemRenderer = new HeldItemRenderer(client);
    }

    public void updateCamera() {
        this.lastViewBob = this.viewBob;
        this.prevThirdPersonDistance = this.thirdPersonDistance;
        this.prevThirdPersonYaw = this.thirdPersonYaw;
        this.prevThirdPersonPitch = this.thirdPersonPitch;
        this.prevCameraRoll = this.cameraRoll;
        this.prevCameraRollAmount = this.cameraRollAmount;
        if (this.client.camera == null) {
            this.client.camera = this.client.player;
        }
        float f = this.client.world.getLightBrightness(MathHelper.floor(this.client.camera.x), MathHelper.floor(this.client.camera.y), MathHelper.floor(this.client.camera.z));
        float f2 = (float)(3 - this.client.options.viewDistance) / 3.0f;
        float f3 = f * (1.0f - f2) + f2;
        this.viewBob += (f3 - this.viewBob) * 0.1f;
        ++this.ticks;
        this.heldItemRenderer.tick();
        this.renderRain();
    }

    public void updateTargetedEntity(float tickDelta) {
        if (this.client.camera == null) {
            return;
        }
        if (this.client.world == null) {
            return;
        }
        double d = this.client.interactionManager.getReachDistance();
        this.client.crosshairTarget = this.client.camera.raycast(d, tickDelta);
        double d2 = d;
        Vec3d vec3d = this.client.camera.getPosition(tickDelta);
        if (this.client.crosshairTarget != null) {
            d2 = this.client.crosshairTarget.pos.distanceTo(vec3d);
        }
        if (this.client.interactionManager instanceof TestInteractionManager) {
            d = 32.0;
            d2 = 32.0;
        } else {
            if (d2 > 3.0) {
                d2 = 3.0;
            }
            d = d2;
        }
        Vec3d vec3d2 = this.client.camera.getLookVector(tickDelta);
        Vec3d vec3d3 = vec3d.add(vec3d2.x * d, vec3d2.y * d, vec3d2.z * d);
        this.targetedEntity = null;
        float f = 1.0f;
        List list = this.client.world.getEntities(this.client.camera, this.client.camera.boundingBox.stretch(vec3d2.x * d, vec3d2.y * d, vec3d2.z * d).expand(f, f, f));
        double d3 = 0.0;
        for (int i = 0; i < list.size(); ++i) {
            double d4;
            Entity entity = (Entity)list.get(i);
            if (!entity.isCollidable()) continue;
            float f2 = entity.getTargetingMargin();
            Box box = entity.boundingBox.expand(f2, f2, f2);
            HitResult hitResult = box.raycast(vec3d, vec3d3);
            if (box.contains(vec3d)) {
                if (!(0.0 < d3) && d3 != 0.0) continue;
                this.targetedEntity = entity;
                d3 = 0.0;
                continue;
            }
            if (hitResult == null || !((d4 = vec3d.distanceTo(hitResult.pos)) < d3) && d3 != 0.0) continue;
            this.targetedEntity = entity;
            d3 = d4;
        }
        if (this.targetedEntity != null && !(this.client.interactionManager instanceof TestInteractionManager)) {
            this.client.crosshairTarget = new HitResult(this.targetedEntity);
        }
    }

    private float getFov(float tickDelta) {
        LivingEntity livingEntity = this.client.camera;
        float f = 70.0f;
        if (livingEntity.isInFluid(Material.WATER)) {
            f = 60.0f;
        }
        if (livingEntity.health <= 0) {
            float f2 = (float)livingEntity.deathTime + tickDelta;
            f /= (1.0f - 500.0f / (f2 + 500.0f)) * 2.0f + 1.0f;
        }
        return f + this.prevCameraRoll + (this.cameraRoll - this.prevCameraRoll) * tickDelta;
    }

    private void applyDamageTiltEffect(float tickDelta) {
        float f;
        LivingEntity livingEntity = this.client.camera;
        float f2 = (float)livingEntity.hurtTime - tickDelta;
        if (livingEntity.health <= 0) {
            f = (float)livingEntity.deathTime + tickDelta;
            GL11.glRotatef(40.0f - 8000.0f / (f + 200.0f), 0.0f, 0.0f, 1.0f);
        }
        if (f2 < 0.0f) {
            return;
        }
        f2 /= (float)livingEntity.damagedTime;
        f2 = MathHelper.sin(f2 * f2 * f2 * f2 * (float)Math.PI);
        f = livingEntity.damagedSwingDir;
        GL11.glRotatef(-f, 0.0f, 1.0f, 0.0f);
        GL11.glRotatef(-f2 * 14.0f, 0.0f, 0.0f, 1.0f);
        GL11.glRotatef(f, 0.0f, 1.0f, 0.0f);
    }

    private void applyViewBobbing(float tickDelta) {
        if (!(this.client.camera instanceof PlayerEntity)) {
            return;
        }
        PlayerEntity playerEntity = (PlayerEntity)this.client.camera;
        float f = playerEntity.horizontalSpeed - playerEntity.prevHorizontalSpeed;
        float f2 = -(playerEntity.horizontalSpeed + f * tickDelta);
        float f3 = playerEntity.prevStepBobbingAmount + (playerEntity.stepBobbingAmount - playerEntity.prevStepBobbingAmount) * tickDelta;
        float f4 = playerEntity.prevTilt + (playerEntity.tilt - playerEntity.prevTilt) * tickDelta;
        GL11.glTranslatef(MathHelper.sin(f2 * (float)Math.PI) * f3 * 0.5f, -Math.abs(MathHelper.cos(f2 * (float)Math.PI) * f3), 0.0f);
        GL11.glRotatef(MathHelper.sin(f2 * (float)Math.PI) * f3 * 3.0f, 0.0f, 0.0f, 1.0f);
        GL11.glRotatef(Math.abs(MathHelper.cos(f2 * (float)Math.PI - 0.2f) * f3) * 5.0f, 1.0f, 0.0f, 0.0f);
        GL11.glRotatef(f4, 1.0f, 0.0f, 0.0f);
    }

    private void applyCameraTransform(float tickDelta) {
        LivingEntity livingEntity = this.client.camera;
        float f = livingEntity.standingEyeHeight - 1.62f;
        double d = livingEntity.prevX + (livingEntity.x - livingEntity.prevX) * (double)tickDelta;
        double d2 = livingEntity.prevY + (livingEntity.y - livingEntity.prevY) * (double)tickDelta - (double)f;
        double d3 = livingEntity.prevZ + (livingEntity.z - livingEntity.prevZ) * (double)tickDelta;
        GL11.glRotatef(this.prevCameraRollAmount + (this.cameraRollAmount - this.prevCameraRollAmount) * tickDelta, 0.0f, 0.0f, 1.0f);
        if (livingEntity.isSleeping()) {
            f = (float)((double)f + 1.0);
            GL11.glTranslatef(0.0f, 0.3f, 0.0f);
            if (!this.client.options.debugCamera) {
                int n = this.client.world.getBlockId(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.y), MathHelper.floor(livingEntity.z));
                if (n == Block.BED.id) {
                    int n2 = this.client.world.getBlockMeta(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.y), MathHelper.floor(livingEntity.z));
                    int n3 = n2 & 3;
                    GL11.glRotatef(n3 * 90, 0.0f, 1.0f, 0.0f);
                }
                GL11.glRotatef(livingEntity.prevYaw + (livingEntity.yaw - livingEntity.prevYaw) * tickDelta + 180.0f, 0.0f, -1.0f, 0.0f);
                GL11.glRotatef(livingEntity.prevPitch + (livingEntity.pitch - livingEntity.prevPitch) * tickDelta, -1.0f, 0.0f, 0.0f);
            }
        } else if (this.client.options.thirdPerson) {
            double d4 = this.prevThirdPersonDistance + (this.thirdPersonDistance - this.prevThirdPersonDistance) * tickDelta;
            if (this.client.options.debugCamera) {
                float f2 = this.prevThirdPersonYaw + (this.thirdPersonYaw - this.prevThirdPersonYaw) * tickDelta;
                float f3 = this.prevThirdPersonPitch + (this.thirdPersonPitch - this.prevThirdPersonPitch) * tickDelta;
                GL11.glTranslatef(0.0f, 0.0f, (float)(-d4));
                GL11.glRotatef(f3, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(f2, 0.0f, 1.0f, 0.0f);
            } else {
                float f4 = livingEntity.yaw;
                float f5 = livingEntity.pitch;
                double d5 = (double)(-MathHelper.sin(f4 / 180.0f * (float)Math.PI) * MathHelper.cos(f5 / 180.0f * (float)Math.PI)) * d4;
                double d6 = (double)(MathHelper.cos(f4 / 180.0f * (float)Math.PI) * MathHelper.cos(f5 / 180.0f * (float)Math.PI)) * d4;
                double d7 = (double)(-MathHelper.sin(f5 / 180.0f * (float)Math.PI)) * d4;
                for (int i = 0; i < 8; ++i) {
                    double d8;
                    HitResult hitResult;
                    float f6 = (i & 1) * 2 - 1;
                    float f7 = (i >> 1 & 1) * 2 - 1;
                    float f8 = (i >> 2 & 1) * 2 - 1;
                    if ((hitResult = this.client.world.raycast(Vec3d.createCached(d + (double)(f6 *= 0.1f), d2 + (double)(f7 *= 0.1f), d3 + (double)(f8 *= 0.1f)), Vec3d.createCached(d - d5 + (double)f6 + (double)f8, d2 - d7 + (double)f7, d3 - d6 + (double)f8))) == null || !((d8 = hitResult.pos.distanceTo(Vec3d.createCached(d, d2, d3))) < d4)) continue;
                    d4 = d8;
                }
                GL11.glRotatef(livingEntity.pitch - f5, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(livingEntity.yaw - f4, 0.0f, 1.0f, 0.0f);
                GL11.glTranslatef(0.0f, 0.0f, (float)(-d4));
                GL11.glRotatef(f4 - livingEntity.yaw, 0.0f, 1.0f, 0.0f);
                GL11.glRotatef(f5 - livingEntity.pitch, 1.0f, 0.0f, 0.0f);
            }
        } else {
            GL11.glTranslatef(0.0f, 0.0f, -0.1f);
        }
        if (!this.client.options.debugCamera) {
            GL11.glRotatef(livingEntity.prevPitch + (livingEntity.pitch - livingEntity.prevPitch) * tickDelta, 1.0f, 0.0f, 0.0f);
            GL11.glRotatef(livingEntity.prevYaw + (livingEntity.yaw - livingEntity.prevYaw) * tickDelta + 180.0f, 0.0f, 1.0f, 0.0f);
        }
        GL11.glTranslatef(0.0f, f, 0.0f);
        d = livingEntity.prevX + (livingEntity.x - livingEntity.prevX) * (double)tickDelta;
        d2 = livingEntity.prevY + (livingEntity.y - livingEntity.prevY) * (double)tickDelta - (double)f;
        d3 = livingEntity.prevZ + (livingEntity.z - livingEntity.prevZ) * (double)tickDelta;
        this.renderFog = this.client.worldRenderer.hasFog(d, d2, d3, tickDelta);
    }

    private void renderWorld(float tickDelta, int eye) {
        float f;
        this.viewDistance = 256 >> this.client.options.viewDistance;
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        float f2 = 0.07f;
        if (this.client.options.anaglyph3d) {
            GL11.glTranslatef((float)(-(eye * 2 - 1)) * f2, 0.0f, 0.0f);
        }
        if (this.zoom != 1.0) {
            GL11.glTranslatef((float)this.zoomX, (float)(-this.zoomY), 0.0f);
            GL11.glScaled(this.zoom, this.zoom, 1.0);
            GLU.gluPerspective(this.getFov(tickDelta), (float)this.client.displayWidth / (float)this.client.displayHeight, 0.05f, this.viewDistance * 2.0f);
        } else {
            GLU.gluPerspective(this.getFov(tickDelta), (float)this.client.displayWidth / (float)this.client.displayHeight, 0.05f, this.viewDistance * 2.0f);
        }
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        if (this.client.options.anaglyph3d) {
            GL11.glTranslatef((float)(eye * 2 - 1) * 0.1f, 0.0f, 0.0f);
        }
        this.applyDamageTiltEffect(tickDelta);
        if (this.client.options.bobView) {
            this.applyViewBobbing(tickDelta);
        }
        if ((f = this.client.player.lastScreenDistortion + (this.client.player.screenDistortion - this.client.player.lastScreenDistortion) * tickDelta) > 0.0f) {
            float f3 = 5.0f / (f * f + 5.0f) - f * 0.04f;
            f3 *= f3;
            GL11.glRotatef(((float)this.ticks + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
            GL11.glScalef(1.0f / f3, 1.0f, 1.0f);
            GL11.glRotatef(-((float)this.ticks + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
        }
        this.applyCameraTransform(tickDelta);
    }

    private void renderFirstPersonHand(float tickDelta, int eye) {
        GL11.glLoadIdentity();
        if (this.client.options.anaglyph3d) {
            GL11.glTranslatef((float)(eye * 2 - 1) * 0.1f, 0.0f, 0.0f);
        }
        GL11.glPushMatrix();
        this.applyDamageTiltEffect(tickDelta);
        if (this.client.options.bobView) {
            this.applyViewBobbing(tickDelta);
        }
        if (!(this.client.options.thirdPerson || this.client.camera.isSleeping() || this.client.options.hideHud)) {
            this.heldItemRenderer.render(tickDelta);
        }
        GL11.glPopMatrix();
        if (!this.client.options.thirdPerson && !this.client.camera.isSleeping()) {
            this.heldItemRenderer.renderScreenOverlays(tickDelta);
            this.applyDamageTiltEffect(tickDelta);
        }
        if (this.client.options.bobView) {
            this.applyViewBobbing(tickDelta);
        }
    }

    public void onFrameUpdate(float tickDelta) {
        int n;
        if (!Display.isActive()) {
            if (System.currentTimeMillis() - this.lastInactiveTime > 500L) {
                this.client.pauseGame();
            }
        } else {
            this.lastInactiveTime = System.currentTimeMillis();
        }
        if (this.client.focused) {
            this.client.mouse.poll();
            float f = this.client.options.mouseSensitivity * 0.6f + 0.2f;
            float f2 = f * f * f * 8.0f;
            float f3 = (float)this.client.mouse.deltaX * f2;
            float f4 = (float)this.client.mouse.deltaY * f2;
            n = 1;
            if (this.client.options.invertYMouse) {
                n = -1;
            }
            if (this.client.options.cinematicMode) {
                f3 = this.cinematicCameraYawSmoother.smooth(f3, 0.05f * f2);
                f4 = this.cinematicCameraPitchSmoother.smooth(f4, 0.05f * f2);
            }
            this.client.player.changeLookDirection(f3, f4 * (float)n);
        }
        if (this.client.skipGameRender) {
            return;
        }
        anaglyph3d = this.client.options.anaglyph3d;
        ScreenScaler screenScaler = new ScreenScaler(this.client.options, this.client.displayWidth, this.client.displayHeight);
        int n2 = screenScaler.getScaledWidth();
        int n3 = screenScaler.getScaledHeight();
        int n4 = Mouse.getX() * n2 / this.client.displayWidth;
        n = n3 - Mouse.getY() * n3 / this.client.displayHeight - 1;
        int n5 = 200;
        if (this.client.options.fpsLimit == 1) {
            n5 = 120;
        }
        if (this.client.options.fpsLimit == 2) {
            n5 = 40;
        }
        if (this.client.world != null) {
            long l;
            if (this.client.options.fpsLimit == 0) {
                this.renderFrame(tickDelta, 0L);
            } else {
                this.renderFrame(tickDelta, this.lastFrameTime + (long)(1000000000 / n5));
            }
            if (this.client.options.fpsLimit == 2 && (l = (this.lastFrameTime + (long)(1000000000 / n5) - System.nanoTime()) / 1000000L) > 0L && l < 500L) {
                try {
                    Thread.sleep(l);
                }
                catch (InterruptedException interruptedException) {
                    interruptedException.printStackTrace();
                }
            }
            this.lastFrameTime = System.nanoTime();
            if (!this.client.options.hideHud || this.client.currentScreen != null) {
                this.client.inGameHud.render(tickDelta, this.client.currentScreen != null, n4, n);
            }
        } else {
            GL11.glViewport(0, 0, this.client.displayWidth, this.client.displayHeight);
            GL11.glMatrixMode(5889);
            GL11.glLoadIdentity();
            GL11.glMatrixMode(5888);
            GL11.glLoadIdentity();
            this.setupHudRender();
            if (this.client.options.fpsLimit == 2) {
                long l = (this.lastFrameTime + (long)(1000000000 / n5) - System.nanoTime()) / 1000000L;
                if (l < 0L) {
                    l += 10L;
                }
                if (l > 0L && l < 500L) {
                    try {
                        Thread.sleep(l);
                    }
                    catch (InterruptedException interruptedException) {
                        interruptedException.printStackTrace();
                    }
                }
            }
            this.lastFrameTime = System.nanoTime();
        }
        if (this.client.currentScreen != null) {
            GL11.glClear(256);
            this.client.currentScreen.render(n4, n, tickDelta);
            if (this.client.currentScreen != null && this.client.currentScreen.particlesGui != null) {
                this.client.currentScreen.particlesGui.render(tickDelta);
            }
        }
    }

    public void renderFrame(float tickDelta, long time) {
        int n;
        GL11.glEnable(2884);
        GL11.glEnable(2929);
        if (this.client.camera == null) {
            this.client.camera = this.client.player;
        }
        this.updateTargetedEntity(tickDelta);
        LivingEntity livingEntity = this.client.camera;
        WorldRenderer worldRenderer = this.client.worldRenderer;
        ParticleManager particleManager = this.client.particleManager;
        double d = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * (double)tickDelta;
        double d2 = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * (double)tickDelta;
        double d3 = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * (double)tickDelta;
        ChunkSource chunkSource = this.client.world.getChunkSource();
        if (chunkSource instanceof LegacyChunkCache) {
            LegacyChunkCache legacyChunkCache = (LegacyChunkCache)chunkSource;
            int n2 = MathHelper.floor((int)d) >> 4;
            n = MathHelper.floor((int)d3) >> 4;
            legacyChunkCache.setSpawnPoint(n2, n);
        }
        for (int i = 0; i < 2; ++i) {
            if (this.client.options.anaglyph3d) {
                activeEye = i;
                if (activeEye == 0) {
                    GL11.glColorMask(false, true, true, false);
                } else {
                    GL11.glColorMask(true, false, false, false);
                }
            }
            GL11.glViewport(0, 0, this.client.displayWidth, this.client.displayHeight);
            this.updateSkyAndFogColors(tickDelta);
            GL11.glClear(16640);
            GL11.glEnable(2884);
            this.renderWorld(tickDelta, i);
            Frustum.getInstance();
            if (this.client.options.viewDistance < 2) {
                this.applyFog(-1, tickDelta);
                worldRenderer.renderSky(tickDelta);
            }
            GL11.glEnable(2912);
            this.applyFog(1, tickDelta);
            if (this.client.options.ao) {
                GL11.glShadeModel(7425);
            }
            FrustumCuller frustumCuller = new FrustumCuller();
            frustumCuller.prepare(d, d2, d3);
            this.client.worldRenderer.cullChunks(frustumCuller, tickDelta);
            if (i == 0) {
                long l;
                while (!this.client.worldRenderer.compileChunks(livingEntity, false) && time != 0L && (l = time - System.nanoTime()) >= 0L && l <= 1000000000L) {
                }
            }
            this.applyFog(0, tickDelta);
            GL11.glEnable(2912);
            GL11.glBindTexture(3553, this.client.textureManager.getTextureId("/terrain.png"));
            Lighting.turnOff();
            worldRenderer.render(livingEntity, 0, tickDelta);
            GL11.glShadeModel(7424);
            Lighting.turnOn();
            worldRenderer.renderEntities(livingEntity.getPosition(tickDelta), frustumCuller, tickDelta);
            particleManager.renderLit(livingEntity, tickDelta);
            Lighting.turnOff();
            this.applyFog(0, tickDelta);
            particleManager.render(livingEntity, tickDelta);
            if (this.client.crosshairTarget != null && livingEntity.isInFluid(Material.WATER) && livingEntity instanceof PlayerEntity) {
                PlayerEntity playerEntity = (PlayerEntity)livingEntity;
                GL11.glDisable(3008);
                worldRenderer.renderMiningProgress(playerEntity, this.client.crosshairTarget, 0, playerEntity.inventory.getSelectedItem(), tickDelta);
                worldRenderer.renderBlockOutline(playerEntity, this.client.crosshairTarget, 0, playerEntity.inventory.getSelectedItem(), tickDelta);
                GL11.glEnable(3008);
            }
            GL11.glBlendFunc(770, 771);
            this.applyFog(0, tickDelta);
            GL11.glEnable(3042);
            GL11.glDisable(2884);
            GL11.glBindTexture(3553, this.client.textureManager.getTextureId("/terrain.png"));
            if (this.client.options.fancyGraphics) {
                if (this.client.options.ao) {
                    GL11.glShadeModel(7425);
                }
                GL11.glColorMask(false, false, false, false);
                n = worldRenderer.render(livingEntity, 1, tickDelta);
                if (this.client.options.anaglyph3d) {
                    if (activeEye == 0) {
                        GL11.glColorMask(false, true, true, true);
                    } else {
                        GL11.glColorMask(true, false, false, true);
                    }
                } else {
                    GL11.glColorMask(true, true, true, true);
                }
                if (n > 0) {
                    worldRenderer.renderLastChunks(1, tickDelta);
                }
                GL11.glShadeModel(7424);
            } else {
                worldRenderer.render(livingEntity, 1, tickDelta);
            }
            GL11.glDepthMask(true);
            GL11.glEnable(2884);
            GL11.glDisable(3042);
            if (this.zoom == 1.0 && livingEntity instanceof PlayerEntity && this.client.crosshairTarget != null && !livingEntity.isInFluid(Material.WATER)) {
                PlayerEntity playerEntity = (PlayerEntity)livingEntity;
                GL11.glDisable(3008);
                worldRenderer.renderMiningProgress(playerEntity, this.client.crosshairTarget, 0, playerEntity.inventory.getSelectedItem(), tickDelta);
                worldRenderer.renderBlockOutline(playerEntity, this.client.crosshairTarget, 0, playerEntity.inventory.getSelectedItem(), tickDelta);
                GL11.glEnable(3008);
            }
            this.renderSnow(tickDelta);
            GL11.glDisable(2912);
            if (this.targetedEntity != null) {
                // empty if block
            }
            this.applyFog(0, tickDelta);
            GL11.glEnable(2912);
            worldRenderer.renderClouds(tickDelta);
            GL11.glDisable(2912);
            this.applyFog(1, tickDelta);
            if (this.zoom == 1.0) {
                GL11.glClear(256);
                this.renderFirstPersonHand(tickDelta, i);
            }
            if (this.client.options.anaglyph3d) continue;
            return;
        }
        GL11.glColorMask(true, true, true, false);
    }

    private void renderRain() {
        float f = this.client.world.getRainGradient(1.0f);
        if (!this.client.options.fancyGraphics) {
            f /= 2.0f;
        }
        if (f == 0.0f) {
            return;
        }
        this.random.setSeed((long)this.ticks * 312987231L);
        LivingEntity livingEntity = this.client.camera;
        World world = this.client.world;
        int n = MathHelper.floor(livingEntity.x);
        int n2 = MathHelper.floor(livingEntity.y);
        int n3 = MathHelper.floor(livingEntity.z);
        int n4 = 10;
        double d = 0.0;
        double d2 = 0.0;
        double d3 = 0.0;
        int n5 = 0;
        for (int i = 0; i < (int)(100.0f * f * f); ++i) {
            int n6 = n + this.random.nextInt(n4) - this.random.nextInt(n4);
            int n7 = n3 + this.random.nextInt(n4) - this.random.nextInt(n4);
            int n8 = world.getTopSolidBlockY(n6, n7);
            int n9 = world.getBlockId(n6, n8 - 1, n7);
            if (n8 > n2 + n4 || n8 < n2 - n4 || !world.getBiomeSource().getBiome(n6, n7).canRain()) continue;
            float f2 = this.random.nextFloat();
            float f3 = this.random.nextFloat();
            if (n9 <= 0) continue;
            if (Block.BLOCKS[n9].material == Material.LAVA) {
                this.client.particleManager.addParticle(new FireSmokeParticle(world, (float)n6 + f2, (double)((float)n8 + 0.1f) - Block.BLOCKS[n9].minY, (float)n7 + f3, 0.0, 0.0, 0.0));
                continue;
            }
            if (this.random.nextInt(++n5) == 0) {
                d = (float)n6 + f2;
                d2 = (double)((float)n8 + 0.1f) - Block.BLOCKS[n9].minY;
                d3 = (float)n7 + f3;
            }
            this.client.particleManager.addParticle(new RainSplashParticle(world, (float)n6 + f2, (double)((float)n8 + 0.1f) - Block.BLOCKS[n9].minY, (float)n7 + f3));
        }
        if (n5 > 0 && this.random.nextInt(3) < this.rainSoundCounter++) {
            this.rainSoundCounter = 0;
            if (d2 > livingEntity.y + 1.0 && world.getTopSolidBlockY(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.z)) > MathHelper.floor(livingEntity.y)) {
                this.client.world.playSound(d, d2, d3, "ambient.weather.rain", 0.1f, 0.5f);
            } else {
                this.client.world.playSound(d, d2, d3, "ambient.weather.rain", 0.2f, 1.0f);
            }
        }
    }

    protected void renderSnow(float tickDelta) {
        float f;
        int n;
        int n2;
        int n3;
        Biome biome;
        int n4;
        int n5;
        float f2 = this.client.world.getRainGradient(tickDelta);
        if (f2 <= 0.0f) {
            return;
        }
        LivingEntity livingEntity = this.client.camera;
        World world = this.client.world;
        int n6 = MathHelper.floor(livingEntity.x);
        int n7 = MathHelper.floor(livingEntity.y);
        int n8 = MathHelper.floor(livingEntity.z);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glDisable(2884);
        GL11.glNormal3f(0.0f, 1.0f, 0.0f);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        GL11.glAlphaFunc(516, 0.01f);
        GL11.glBindTexture(3553, this.client.textureManager.getTextureId("/environment/snow.png"));
        double d = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * (double)tickDelta;
        double d2 = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * (double)tickDelta;
        double d3 = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * (double)tickDelta;
        int n9 = MathHelper.floor(d2);
        int n10 = 5;
        if (this.client.options.fancyGraphics) {
            n10 = 10;
        }
        Biome[] biomeArray = world.getBiomeSource().getBiomesInArea(n6 - n10, n8 - n10, n10 * 2 + 1, n10 * 2 + 1);
        int n11 = 0;
        for (n5 = n6 - n10; n5 <= n6 + n10; ++n5) {
            for (n4 = n8 - n10; n4 <= n8 + n10; ++n4) {
                if (!(biome = biomeArray[n11++]).canSnow()) continue;
                n3 = world.getTopSolidBlockY(n5, n4);
                if (n3 < 0) {
                    n3 = 0;
                }
                if ((n2 = n3) < n9) {
                    n2 = n9;
                }
                n = n7 - n10;
                int n12 = n7 + n10;
                if (n < n3) {
                    n = n3;
                }
                if (n12 < n3) {
                    n12 = n3;
                }
                f = 1.0f;
                if (n == n12) continue;
                this.random.setSeed(n5 * n5 * 3121 + n5 * 45238971 + n4 * n4 * 418711 + n4 * 13761);
                float f3 = (float)this.ticks + tickDelta;
                float f4 = ((float)(this.ticks & 0x1FF) + tickDelta) / 512.0f;
                float f5 = this.random.nextFloat() + f3 * 0.01f * (float)this.random.nextGaussian();
                float f6 = this.random.nextFloat() + f3 * (float)this.random.nextGaussian() * 0.001f;
                double d4 = (double)((float)n5 + 0.5f) - livingEntity.x;
                double d5 = (double)((float)n4 + 0.5f) - livingEntity.z;
                float f7 = MathHelper.sqrt(d4 * d4 + d5 * d5) / (float)n10;
                tessellator.startQuads();
                float f8 = world.getLightBrightness(n5, n2, n4);
                GL11.glColor4f(f8, f8, f8, ((1.0f - f7 * f7) * 0.3f + 0.5f) * f2);
                tessellator.translate(-d * 1.0, -d2 * 1.0, -d3 * 1.0);
                tessellator.vertex(n5 + 0, n, (double)n4 + 0.5, 0.0f * f + f5, (float)n * f / 4.0f + f4 * f + f6);
                tessellator.vertex(n5 + 1, n, (double)n4 + 0.5, 1.0f * f + f5, (float)n * f / 4.0f + f4 * f + f6);
                tessellator.vertex(n5 + 1, n12, (double)n4 + 0.5, 1.0f * f + f5, (float)n12 * f / 4.0f + f4 * f + f6);
                tessellator.vertex(n5 + 0, n12, (double)n4 + 0.5, 0.0f * f + f5, (float)n12 * f / 4.0f + f4 * f + f6);
                tessellator.vertex((double)n5 + 0.5, n, n4 + 0, 0.0f * f + f5, (float)n * f / 4.0f + f4 * f + f6);
                tessellator.vertex((double)n5 + 0.5, n, n4 + 1, 1.0f * f + f5, (float)n * f / 4.0f + f4 * f + f6);
                tessellator.vertex((double)n5 + 0.5, n12, n4 + 1, 1.0f * f + f5, (float)n12 * f / 4.0f + f4 * f + f6);
                tessellator.vertex((double)n5 + 0.5, n12, n4 + 0, 0.0f * f + f5, (float)n12 * f / 4.0f + f4 * f + f6);
                tessellator.translate(0.0, 0.0, 0.0);
                tessellator.draw();
            }
        }
        GL11.glBindTexture(3553, this.client.textureManager.getTextureId("/environment/rain.png"));
        if (this.client.options.fancyGraphics) {
            n10 = 10;
        }
        n11 = 0;
        for (n5 = n6 - n10; n5 <= n6 + n10; ++n5) {
            for (n4 = n8 - n10; n4 <= n8 + n10; ++n4) {
                if (!(biome = biomeArray[n11++]).canRain()) continue;
                n3 = world.getTopSolidBlockY(n5, n4);
                n2 = n7 - n10;
                n = n7 + n10;
                if (n2 < n3) {
                    n2 = n3;
                }
                if (n < n3) {
                    n = n3;
                }
                float f9 = 1.0f;
                if (n2 == n) continue;
                this.random.setSeed(n5 * n5 * 3121 + n5 * 45238971 + n4 * n4 * 418711 + n4 * 13761);
                f = ((float)(this.ticks + n5 * n5 * 3121 + n5 * 45238971 + n4 * n4 * 418711 + n4 * 13761 & 0x1F) + tickDelta) / 32.0f * (3.0f + this.random.nextFloat());
                double d6 = (double)((float)n5 + 0.5f) - livingEntity.x;
                double d7 = (double)((float)n4 + 0.5f) - livingEntity.z;
                float f10 = MathHelper.sqrt(d6 * d6 + d7 * d7) / (float)n10;
                tessellator.startQuads();
                float f11 = world.getLightBrightness(n5, 128, n4) * 0.85f + 0.15f;
                GL11.glColor4f(f11, f11, f11, ((1.0f - f10 * f10) * 0.5f + 0.5f) * f2);
                tessellator.translate(-d * 1.0, -d2 * 1.0, -d3 * 1.0);
                tessellator.vertex(n5 + 0, n2, (double)n4 + 0.5, 0.0f * f9, (float)n2 * f9 / 4.0f + f * f9);
                tessellator.vertex(n5 + 1, n2, (double)n4 + 0.5, 1.0f * f9, (float)n2 * f9 / 4.0f + f * f9);
                tessellator.vertex(n5 + 1, n, (double)n4 + 0.5, 1.0f * f9, (float)n * f9 / 4.0f + f * f9);
                tessellator.vertex(n5 + 0, n, (double)n4 + 0.5, 0.0f * f9, (float)n * f9 / 4.0f + f * f9);
                tessellator.vertex((double)n5 + 0.5, n2, n4 + 0, 0.0f * f9, (float)n2 * f9 / 4.0f + f * f9);
                tessellator.vertex((double)n5 + 0.5, n2, n4 + 1, 1.0f * f9, (float)n2 * f9 / 4.0f + f * f9);
                tessellator.vertex((double)n5 + 0.5, n, n4 + 1, 1.0f * f9, (float)n * f9 / 4.0f + f * f9);
                tessellator.vertex((double)n5 + 0.5, n, n4 + 0, 0.0f * f9, (float)n * f9 / 4.0f + f * f9);
                tessellator.translate(0.0, 0.0, 0.0);
                tessellator.draw();
            }
        }
        GL11.glEnable(2884);
        GL11.glDisable(3042);
        GL11.glAlphaFunc(516, 0.1f);
    }

    public void setupHudRender() {
        ScreenScaler screenScaler = new ScreenScaler(this.client.options, this.client.displayWidth, this.client.displayHeight);
        GL11.glClear(256);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glOrtho(0.0, screenScaler.rawScaledWidth, screenScaler.rawScaledHeight, 0.0, 1000.0, 3000.0);
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        GL11.glTranslatef(0.0f, 0.0f, -2000.0f);
    }

    private void updateSkyAndFogColors(float tickDelta) {
        float f;
        float f2;
        World world = this.client.world;
        LivingEntity livingEntity = this.client.camera;
        float f3 = 1.0f / (float)(4 - this.client.options.viewDistance);
        f3 = 1.0f - (float)Math.pow(f3, 0.25);
        Vec3d vec3d = world.getSkyColor(this.client.camera, tickDelta);
        float f4 = (float)vec3d.x;
        float f5 = (float)vec3d.y;
        float f6 = (float)vec3d.z;
        Vec3d vec3d2 = world.getFogColor(tickDelta);
        this.fogRed = (float)vec3d2.x;
        this.fogGreen = (float)vec3d2.y;
        this.fogBlue = (float)vec3d2.z;
        this.fogRed += (f4 - this.fogRed) * f3;
        this.fogGreen += (f5 - this.fogGreen) * f3;
        this.fogBlue += (f6 - this.fogBlue) * f3;
        float f7 = world.getRainGradient(tickDelta);
        if (f7 > 0.0f) {
            f2 = 1.0f - f7 * 0.5f;
            f = 1.0f - f7 * 0.4f;
            this.fogRed *= f2;
            this.fogGreen *= f2;
            this.fogBlue *= f;
        }
        if ((f2 = world.getThunderGradient(tickDelta)) > 0.0f) {
            f = 1.0f - f2 * 0.5f;
            this.fogRed *= f;
            this.fogGreen *= f;
            this.fogBlue *= f;
        }
        if (this.renderFog) {
            Vec3d vec3d3 = world.getCloudColor(tickDelta);
            this.fogRed = (float)vec3d3.x;
            this.fogGreen = (float)vec3d3.y;
            this.fogBlue = (float)vec3d3.z;
        } else if (livingEntity.isInFluid(Material.WATER)) {
            this.fogRed = 0.02f;
            this.fogGreen = 0.02f;
            this.fogBlue = 0.2f;
        } else if (livingEntity.isInFluid(Material.LAVA)) {
            this.fogRed = 0.6f;
            this.fogGreen = 0.1f;
            this.fogBlue = 0.0f;
        }
        float f8 = this.lastViewBob + (this.viewBob - this.lastViewBob) * tickDelta;
        this.fogRed *= f8;
        this.fogGreen *= f8;
        this.fogBlue *= f8;
        if (this.client.options.anaglyph3d) {
            float f9 = (this.fogRed * 30.0f + this.fogGreen * 59.0f + this.fogBlue * 11.0f) / 100.0f;
            float f10 = (this.fogRed * 30.0f + this.fogGreen * 70.0f) / 100.0f;
            float f11 = (this.fogRed * 30.0f + this.fogBlue * 70.0f) / 100.0f;
            this.fogRed = f9;
            this.fogGreen = f10;
            this.fogBlue = f11;
        }
        GL11.glClearColor(this.fogRed, this.fogGreen, this.fogBlue, 0.0f);
    }

    private void applyFog(int mode, float tickDelta) {
        LivingEntity livingEntity = this.client.camera;
        GL11.glFog(2918, this.updateFogColorBuffer(this.fogRed, this.fogGreen, this.fogBlue, 1.0f));
        GL11.glNormal3f(0.0f, -1.0f, 0.0f);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (this.renderFog) {
            GL11.glFogi(2917, 2048);
            GL11.glFogf(2914, 0.1f);
            float f = 1.0f;
            float f2 = 1.0f;
            float f3 = 1.0f;
            if (this.client.options.anaglyph3d) {
                float f4 = (f * 30.0f + f2 * 59.0f + f3 * 11.0f) / 100.0f;
                float f5 = (f * 30.0f + f2 * 70.0f) / 100.0f;
                float f6 = (f * 30.0f + f3 * 70.0f) / 100.0f;
                f = f4;
                f2 = f5;
                f3 = f6;
            }
        } else if (livingEntity.isInFluid(Material.WATER)) {
            GL11.glFogi(2917, 2048);
            GL11.glFogf(2914, 0.1f);
            float f = 0.4f;
            float f7 = 0.4f;
            float f8 = 0.9f;
            if (this.client.options.anaglyph3d) {
                float f9 = (f * 30.0f + f7 * 59.0f + f8 * 11.0f) / 100.0f;
                float f10 = (f * 30.0f + f7 * 70.0f) / 100.0f;
                float f11 = (f * 30.0f + f8 * 70.0f) / 100.0f;
                f = f9;
                f7 = f10;
                f8 = f11;
            }
        } else if (livingEntity.isInFluid(Material.LAVA)) {
            GL11.glFogi(2917, 2048);
            GL11.glFogf(2914, 2.0f);
            float f = 0.4f;
            float f12 = 0.3f;
            float f13 = 0.3f;
            if (this.client.options.anaglyph3d) {
                float f14 = (f * 30.0f + f12 * 59.0f + f13 * 11.0f) / 100.0f;
                float f15 = (f * 30.0f + f12 * 70.0f) / 100.0f;
                float f16 = (f * 30.0f + f13 * 70.0f) / 100.0f;
                f = f14;
                f12 = f15;
                f13 = f16;
            }
        } else {
            GL11.glFogi(2917, 9729);
            GL11.glFogf(2915, this.viewDistance * 0.25f);
            GL11.glFogf(2916, this.viewDistance);
            if (mode < 0) {
                GL11.glFogf(2915, 0.0f);
                GL11.glFogf(2916, this.viewDistance * 0.8f);
            }
            if (GLContext.getCapabilities().GL_NV_fog_distance) {
                GL11.glFogi(34138, 34139);
            }
            if (this.client.world.dimension.isNether) {
                GL11.glFogf(2915, 0.0f);
            }
        }
        GL11.glEnable(2903);
        GL11.glColorMaterial(1028, 4608);
    }

    private FloatBuffer updateFogColorBuffer(float red, float green, float blue, float alpha) {
        this.fogColorBuffer.clear();
        this.fogColorBuffer.put(red).put(green).put(blue).put(alpha);
        this.fogColorBuffer.flip();
        return this.fogColorBuffer;
    }
}

