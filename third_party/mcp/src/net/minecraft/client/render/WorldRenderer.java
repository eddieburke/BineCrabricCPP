/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.client.Minecraft;
import net.minecraft.client.particle.ExplosionParticle;
import net.minecraft.client.particle.FireSmokeParticle;
import net.minecraft.client.particle.FlameParticle;
import net.minecraft.client.particle.FootstepParticle;
import net.minecraft.client.particle.HeartParticle;
import net.minecraft.client.particle.ItemParticle;
import net.minecraft.client.particle.LavaEmberParticle;
import net.minecraft.client.particle.NoteParticle;
import net.minecraft.client.particle.PortalParticle;
import net.minecraft.client.particle.RedDustParticle;
import net.minecraft.client.particle.SnowParticle;
import net.minecraft.client.particle.WaterBubbleParticle;
import net.minecraft.client.particle.WaterSplashParticle;
import net.minecraft.client.render.Culler;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.block.entity.BlockEntityRenderDispatcher;
import net.minecraft.client.render.chunk.ChunkBuilder;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.render.world.ChunkRenderer;
import net.minecraft.client.render.world.DirtyChunkSorter;
import net.minecraft.client.render.world.DistanceChunkSorter;
import net.minecraft.client.texture.SkinImageProcessor;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.MusicDiscItem;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.hit.HitResultType;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;
import net.minecraft.world.event.listener.GameEventListener;
import org.lwjgl.opengl.ARBOcclusionQuery;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class WorldRenderer
implements GameEventListener {
    public List globalBlockEntities = new ArrayList();
    private World world;
    private TextureManager textureManager;
    private LinkedHashSet<ChunkBuilder> dirtyChunks = new LinkedHashSet<ChunkBuilder>();
    private ChunkBuilder[] sortedChunks;
    private ChunkBuilder[] chunks;
    private int chunkCountX;
    private int chunkCountY;
    private int chunkCountZ;
    private int chunkGlList;
    private Minecraft client;
    private BlockRenderManager blockRenderManager;
    private IntBuffer occlusionBuffer;
    private boolean occlusion = false;
    private int ticks = 0;
    private int starsGlList;
    private int lightSkyGlList;
    private int darkSkyGlList;
    private int minChunkX;
    private int minChunkY;
    private int minChunkZ;
    private int maxChunkX;
    private int maxChunkY;
    private int maxChunkZ;
    private int lastViewDistance = -1;
    private int entityRenderCooldown = 2;
    private int entityCount;
    private int renderedEntityCount;
    private int culledEntityCount;
    int[] occlusionQueryResults = new int[50000];
    IntBuffer occlusionQueryBuffer = GlAllocationUtils.allocateIntBuffer(64);
    private int chunkCount;
    private int invisibleChunkCount;
    private int occludedChunkCount;
    private int compiledChunkCount;
    private int emptyChunkCount;
    private int chunkRenderIndex;
    private List chunksInCurrentLayer = new ArrayList();
    private ChunkRenderer[] chunkRenderers = new ChunkRenderer[]{new ChunkRenderer(), new ChunkRenderer(), new ChunkRenderer(), new ChunkRenderer()};
    int displayListId = 0;
    int chunkDisplayListId = GlAllocationUtils.generateDisplayLists(1);
    double prevCameraX = -9999.0;
    double prevCameraY = -9999.0;
    double prevCameraZ = -9999.0;
    private DistanceChunkSorter distanceChunkSorter = null;
    private DirtyChunkSorter dirtyChunkSorterCached = null;
    public float miningProgress;
    int cullStep = 0;

    public WorldRenderer(Minecraft minecraft, TextureManager textureManager) {
        int n;
        int n2;
        this.client = minecraft;
        this.textureManager = textureManager;
        int n3 = 64;
        this.chunkGlList = GlAllocationUtils.generateDisplayLists(n3 * n3 * n3 * 3);
        this.occlusion = minecraft.getOpenGlCapabilities().glArbOcclusionQuery();
        if (this.occlusion) {
            this.occlusionQueryBuffer.clear();
            this.occlusionBuffer = GlAllocationUtils.allocateIntBuffer(n3 * n3 * n3);
            this.occlusionBuffer.clear();
            this.occlusionBuffer.position(0);
            this.occlusionBuffer.limit(n3 * n3 * n3);
            ARBOcclusionQuery.glGenQueriesARB(this.occlusionBuffer);
        }
        this.starsGlList = GlAllocationUtils.generateDisplayLists(3);
        GL11.glPushMatrix();
        GL11.glNewList(this.starsGlList, 4864);
        this.renderStars();
        GL11.glEndList();
        GL11.glPopMatrix();
        Tessellator tessellator = Tessellator.INSTANCE;
        this.lightSkyGlList = this.starsGlList + 1;
        GL11.glNewList(this.lightSkyGlList, 4864);
        int n4 = 64;
        int n5 = 256 / n4 + 2;
        float f = 16.0f;
        for (n2 = -n4 * n5; n2 <= n4 * n5; n2 += n4) {
            for (n = -n4 * n5; n <= n4 * n5; n += n4) {
                tessellator.startQuads();
                tessellator.vertex(n2 + 0, f, n + 0);
                tessellator.vertex(n2 + n4, f, n + 0);
                tessellator.vertex(n2 + n4, f, n + n4);
                tessellator.vertex(n2 + 0, f, n + n4);
                tessellator.draw();
            }
        }
        GL11.glEndList();
        this.darkSkyGlList = this.starsGlList + 2;
        GL11.glNewList(this.darkSkyGlList, 4864);
        f = -16.0f;
        tessellator.startQuads();
        for (n2 = -n4 * n5; n2 <= n4 * n5; n2 += n4) {
            for (n = -n4 * n5; n <= n4 * n5; n += n4) {
                tessellator.vertex(n2 + n4, f, n + 0);
                tessellator.vertex(n2 + 0, f, n + 0);
                tessellator.vertex(n2 + 0, f, n + n4);
                tessellator.vertex(n2 + n4, f, n + n4);
            }
        }
        tessellator.draw();
        GL11.glEndList();
    }

    private void renderStars() {
        Random random = new Random(10842L);
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        for (int i = 0; i < 1500; ++i) {
            double d = random.nextFloat() * 2.0f - 1.0f;
            double d2 = random.nextFloat() * 2.0f - 1.0f;
            double d3 = random.nextFloat() * 2.0f - 1.0f;
            double d4 = 0.25f + random.nextFloat() * 0.25f;
            double d5 = d * d + d2 * d2 + d3 * d3;
            if (!(d5 < 1.0) || !(d5 > 0.01)) continue;
            d5 = 1.0 / Math.sqrt(d5);
            double d6 = (d *= d5) * 100.0;
            double d7 = (d2 *= d5) * 100.0;
            double d8 = (d3 *= d5) * 100.0;
            double d9 = Math.atan2(d, d3);
            double d10 = Math.sin(d9);
            double d11 = Math.cos(d9);
            double d12 = Math.atan2(Math.sqrt(d * d + d3 * d3), d2);
            double d13 = Math.sin(d12);
            double d14 = Math.cos(d12);
            double d15 = random.nextDouble() * Math.PI * 2.0;
            double d16 = Math.sin(d15);
            double d17 = Math.cos(d15);
            for (int j = 0; j < 4; ++j) {
                double d18;
                double d19 = 0.0;
                double d20 = (double)((j & 2) - 1) * d4;
                double d21 = (double)((j + 1 & 2) - 1) * d4;
                double d22 = d19;
                double d23 = d20 * d17 - d21 * d16;
                double d24 = d18 = d21 * d17 + d20 * d16;
                double d25 = d23 * d13 + d22 * d14;
                double d26 = d22 * d13 - d23 * d14;
                double d27 = d26 * d10 - d24 * d11;
                double d28 = d25;
                double d29 = d24 * d10 + d26 * d11;
                tessellator.vertex(d6 + d27, d7 + d28, d8 + d29);
            }
        }
        tessellator.draw();
    }

    public void setWorld(World world) {
        if (this.world != null) {
            this.world.removeEventListener(this);
        }
        this.prevCameraX = -9999.0;
        this.prevCameraY = -9999.0;
        this.prevCameraZ = -9999.0;
        EntityRenderDispatcher.INSTANCE.setWorld(world);
        this.world = world;
        this.blockRenderManager = new BlockRenderManager(world);
        if (world != null) {
            world.addEventListener(this);
            this.reload();
        }
    }

    public void reload() {
        LivingEntity livingEntity;
        int n;
        int n2;
        Block.LEAVES.setFancyGraphics(this.client.options.fancyGraphics);
        this.lastViewDistance = this.client.options.viewDistance;
        if (this.chunks != null) {
            for (n2 = 0; n2 < this.chunks.length; ++n2) {
                this.chunks[n2].close();
            }
        }
        if ((n2 = 64 << 3 - this.lastViewDistance) > 400) {
            n2 = 400;
        }
        this.chunkCountX = n2 / 16 + 1;
        this.chunkCountY = 8;
        this.chunkCountZ = n2 / 16 + 1;
        this.chunks = new ChunkBuilder[this.chunkCountX * this.chunkCountY * this.chunkCountZ];
        this.sortedChunks = new ChunkBuilder[this.chunkCountX * this.chunkCountY * this.chunkCountZ];
        int n3 = 0;
        int n4 = 0;
        this.minChunkX = 0;
        this.minChunkY = 0;
        this.minChunkZ = 0;
        this.maxChunkX = this.chunkCountX;
        this.maxChunkY = this.chunkCountY;
        this.maxChunkZ = this.chunkCountZ;
        for (n = 0; n < this.dirtyChunks.size(); ++n) {
            ((ChunkBuilder)this.dirtyChunks.get((int)n)).dirty = false;
        }
        this.dirtyChunks.clear();
        this.globalBlockEntities.clear();
        for (n = 0; n < this.chunkCountX; ++n) {
            for (int i = 0; i < this.chunkCountY; ++i) {
                for (int j = 0; j < this.chunkCountZ; ++j) {
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n] = new ChunkBuilder(this.world, this.globalBlockEntities, n * 16, i * 16, j * 16, 16, this.chunkGlList + n3);
                    if (this.occlusion) {
                        this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].occlusionQueryId = this.occlusionBuffer.get(n4);
                    }
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].occlusionQueryReady = false;
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].unoccluded = true;
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].inFrustum = true;
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].id = n4++;
                    this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n].invalidate();
                    this.sortedChunks[(j * this.chunkCountY + i) * this.chunkCountX + n] = this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n];
                    this.dirtyChunks.add(this.chunks[(j * this.chunkCountY + i) * this.chunkCountX + n]);
                    n3 += 3;
                }
            }
        }
        if (this.world != null && (livingEntity = this.client.camera) != null) {
            this.sortChunks(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.y), MathHelper.floor(livingEntity.z));
            if (this.distanceChunkSorter == null) {
                this.distanceChunkSorter = new DistanceChunkSorter(livingEntity);
            } else {
                this.distanceChunkSorter.update(livingEntity);
            }
            Arrays.sort(this.sortedChunks, this.distanceChunkSorter);
        }
        this.entityRenderCooldown = 2;
    }

    public void renderEntities(Vec3d cameraPos, Culler culler, float tickDelta) {
        Entity entity;
        int n;
        if (this.entityRenderCooldown > 0) {
            --this.entityRenderCooldown;
            return;
        }
        BlockEntityRenderDispatcher.INSTANCE.prepare(this.world, this.textureManager, this.client.textRenderer, this.client.camera, tickDelta);
        EntityRenderDispatcher.INSTANCE.init(this.world, this.textureManager, this.client.textRenderer, this.client.camera, this.client.options, tickDelta);
        this.entityCount = 0;
        this.renderedEntityCount = 0;
        this.culledEntityCount = 0;
        LivingEntity livingEntity = this.client.camera;
        EntityRenderDispatcher.offsetX = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * (double)tickDelta;
        EntityRenderDispatcher.offsetY = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * (double)tickDelta;
        EntityRenderDispatcher.offsetZ = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * (double)tickDelta;
        BlockEntityRenderDispatcher.offsetX = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * (double)tickDelta;
        BlockEntityRenderDispatcher.offsetY = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * (double)tickDelta;
        BlockEntityRenderDispatcher.offsetZ = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * (double)tickDelta;
        List list = this.world.getEntities();
        this.entityCount = list.size();
        for (n = 0; n < this.world.globalEntities.size(); ++n) {
            entity = (Entity)this.world.globalEntities.get(n);
            ++this.renderedEntityCount;
            if (!entity.shouldRender(cameraPos)) continue;
            EntityRenderDispatcher.INSTANCE.render(entity, tickDelta);
        }
        for (n = 0; n < list.size(); ++n) {
            entity = (Entity)list.get(n);
            if (!entity.shouldRender(cameraPos) || !entity.ignoreFrustumCull && !culler.isVisible(entity.boundingBox) || entity == this.client.camera && !this.client.options.thirdPerson && !this.client.camera.isSleeping()) continue;
            int n2 = MathHelper.floor(entity.y);
            if (n2 < 0) {
                n2 = 0;
            }
            if (n2 >= 128) {
                n2 = 127;
            }
            if (!this.world.isPosLoaded(MathHelper.floor(entity.x), n2, MathHelper.floor(entity.z))) continue;
            ++this.renderedEntityCount;
            EntityRenderDispatcher.INSTANCE.render(entity, tickDelta);
        }
        for (n = 0; n < this.globalBlockEntities.size(); ++n) {
            BlockEntityRenderDispatcher.INSTANCE.render((BlockEntity)this.globalBlockEntities.get(n), tickDelta);
        }
    }

    public String getChunkDebugInfo() {
        return "C: " + this.compiledChunkCount + "/" + this.chunkCount + ". F: " + this.invisibleChunkCount + ", O: " + this.occludedChunkCount + ", E: " + this.emptyChunkCount;
    }

    public String getEntityDebugInfo() {
        return "E: " + this.renderedEntityCount + "/" + this.entityCount + ". B: " + this.culledEntityCount + ", I: " + (this.entityCount - this.culledEntityCount - this.renderedEntityCount);
    }

    private void sortChunks(int cameraX, int cameraY, int cameraZ) {
        cameraX -= 8;
        cameraY -= 8;
        cameraZ -= 8;
        this.minChunkX = Integer.MAX_VALUE;
        this.minChunkY = Integer.MAX_VALUE;
        this.minChunkZ = Integer.MAX_VALUE;
        this.maxChunkX = Integer.MIN_VALUE;
        this.maxChunkY = Integer.MIN_VALUE;
        this.maxChunkZ = Integer.MIN_VALUE;
        int n = this.chunkCountX * 16;
        int n2 = n / 2;
        for (int i = 0; i < this.chunkCountX; ++i) {
            int n3 = i * 16;
            int n4 = n3 + n2 - cameraX;
            if (n4 < 0) {
                n4 -= n - 1;
            }
            if ((n3 -= (n4 /= n) * n) < this.minChunkX) {
                this.minChunkX = n3;
            }
            if (n3 > this.maxChunkX) {
                this.maxChunkX = n3;
            }
            for (int j = 0; j < this.chunkCountZ; ++j) {
                int n5 = j * 16;
                int n6 = n5 + n2 - cameraZ;
                if (n6 < 0) {
                    n6 -= n - 1;
                }
                if ((n5 -= (n6 /= n) * n) < this.minChunkZ) {
                    this.minChunkZ = n5;
                }
                if (n5 > this.maxChunkZ) {
                    this.maxChunkZ = n5;
                }
                for (int k = 0; k < this.chunkCountY; ++k) {
                    int n7 = k * 16;
                    if (n7 < this.minChunkY) {
                        this.minChunkY = n7;
                    }
                    if (n7 > this.maxChunkY) {
                        this.maxChunkY = n7;
                    }
                    ChunkBuilder chunkBuilder = this.chunks[(j * this.chunkCountY + k) * this.chunkCountX + i];
                    boolean bl = chunkBuilder.dirty;
                    chunkBuilder.setPosition(n3, n7, n5);
                    if (bl || !chunkBuilder.dirty) continue;
                    this.dirtyChunks.add(chunkBuilder);
                }
            }
        }
    }

    public int render(LivingEntity camera, int layer, double tickDelta) {
        for (int i = 0; i < 10; ++i) {
            this.chunkRenderIndex = (this.chunkRenderIndex + 1) % this.chunks.length;
            ChunkBuilder chunkBuilder = this.chunks[this.chunkRenderIndex];
            if (!chunkBuilder.dirty || this.dirtyChunks.contains(chunkBuilder)) continue;
            this.dirtyChunks.add(chunkBuilder);
        }
        if (this.client.options.viewDistance != this.lastViewDistance) {
            this.reload();
        }
        if (layer == 0) {
            this.chunkCount = 0;
            this.invisibleChunkCount = 0;
            this.occludedChunkCount = 0;
            this.compiledChunkCount = 0;
            this.emptyChunkCount = 0;
        }
        double d = camera.lastTickX + (camera.x - camera.lastTickX) * tickDelta;
        double d2 = camera.lastTickY + (camera.y - camera.lastTickY) * tickDelta;
        double d3 = camera.lastTickZ + (camera.z - camera.lastTickZ) * tickDelta;
        double d4 = camera.x - this.prevCameraX;
        double d5 = camera.y - this.prevCameraY;
        double d6 = camera.z - this.prevCameraZ;
        if (d4 * d4 + d5 * d5 + d6 * d6 > 64.0) {
            this.prevCameraX = camera.x;
            this.prevCameraY = camera.y;
            this.prevCameraZ = camera.z;
            this.sortChunks(MathHelper.floor(camera.x), MathHelper.floor(camera.y), MathHelper.floor(camera.z));
            if (this.distanceChunkSorter == null) {
                this.distanceChunkSorter = new DistanceChunkSorter(camera);
            } else {
                this.distanceChunkSorter.update(camera);
            }
            Arrays.sort(this.sortedChunks, this.distanceChunkSorter);
        }
        Lighting.turnOff();
        int n = 0;
        if (this.occlusion && this.client.options.advancedOpengl && !this.client.options.anaglyph3d && layer == 0) {
            int n2 = 0;
            int n3 = 16;
            this.checkOcclusionQueries(n2, n3);
            for (int i = n2; i < n3; ++i) {
                this.sortedChunks[i].unoccluded = true;
            }
            n += this.renderChunks(n2, n3, layer, tickDelta);
            do {
                n2 = n3;
                if ((n3 *= 2) > this.sortedChunks.length) {
                    n3 = this.sortedChunks.length;
                }
                GL11.glDisable(3553);
                GL11.glDisable(2896);
                GL11.glDisable(3008);
                GL11.glDisable(2912);
                GL11.glColorMask(false, false, false, false);
                GL11.glDepthMask(false);
                this.checkOcclusionQueries(n2, n3);
                GL11.glPushMatrix();
                float f = 0.0f;
                float f2 = 0.0f;
                float f3 = 0.0f;
                for (int i = n2; i < n3; ++i) {
                    float f4;
                    int n4;
                    if (this.sortedChunks[i].hasNoGeometry()) {
                        this.sortedChunks[i].inFrustum = false;
                        continue;
                    }
                    if (!this.sortedChunks[i].inFrustum) {
                        this.sortedChunks[i].unoccluded = true;
                    }
                    if (!this.sortedChunks[i].inFrustum || this.sortedChunks[i].occlusionQueryReady || this.ticks % (n4 = (int)(1.0f + (f4 = MathHelper.sqrt(this.sortedChunks[i].squaredDistanceTo(camera))) / 128.0f)) != i % n4) continue;
                    ChunkBuilder chunkBuilder = this.sortedChunks[i];
                    float f5 = (float)((double)chunkBuilder.cameraOffsetX - d);
                    float f6 = (float)((double)chunkBuilder.cameraOffsetY - d2);
                    float f7 = (float)((double)chunkBuilder.cameraOffsetZ - d3);
                    float f8 = f5 - f;
                    float f9 = f6 - f2;
                    float f10 = f7 - f3;
                    if (f8 != 0.0f || f9 != 0.0f || f10 != 0.0f) {
                        GL11.glTranslatef(f8, f9, f10);
                        f += f8;
                        f2 += f9;
                        f3 += f10;
                    }
                    ARBOcclusionQuery.glBeginQueryARB(35092, this.sortedChunks[i].occlusionQueryId);
                    this.sortedChunks[i].renderOcclusionBox();
                    ARBOcclusionQuery.glEndQueryARB(35092);
                    this.sortedChunks[i].occlusionQueryReady = true;
                }
                GL11.glPopMatrix();
                if (this.client.options.anaglyph3d) {
                    if (GameRenderer.activeEye == 0) {
                        GL11.glColorMask(false, true, true, true);
                    } else {
                        GL11.glColorMask(true, false, false, true);
                    }
                } else {
                    GL11.glColorMask(true, true, true, true);
                }
                GL11.glDepthMask(true);
                GL11.glEnable(3553);
                GL11.glEnable(2896);
                GL11.glEnable(3008);
                GL11.glEnable(2912);
                n += this.renderChunks(n2, n3, layer, tickDelta);
            } while (n3 < this.sortedChunks.length);
        } else {
            n += this.renderChunks(0, this.sortedChunks.length, layer, tickDelta);
        }
        return n;
    }

    private void checkOcclusionQueries(int from, int to) {
        for (int i = from; i < to; ++i) {
            if (!this.sortedChunks[i].occlusionQueryReady) continue;
            this.occlusionQueryBuffer.clear();
            ARBOcclusionQuery.glGetQueryObjectuARB(this.sortedChunks[i].occlusionQueryId, 34919, this.occlusionQueryBuffer);
            if (this.occlusionQueryBuffer.get(0) == 0) continue;
            this.sortedChunks[i].occlusionQueryReady = false;
            this.occlusionQueryBuffer.clear();
            ARBOcclusionQuery.glGetQueryObjectuARB(this.sortedChunks[i].occlusionQueryId, 34918, this.occlusionQueryBuffer);
            this.sortedChunks[i].unoccluded = this.occlusionQueryBuffer.get(0) != 0;
        }
    }

    private int renderChunks(int from, int to, int layer, double tickDelta) {
        int n;
        this.chunksInCurrentLayer.clear();
        int n2 = 0;
        for (int i = from; i < to; ++i) {
            int n3;
            if (layer == 0) {
                ++this.chunkCount;
                if (this.sortedChunks[i].renderLayerEmpty[layer]) {
                    ++this.emptyChunkCount;
                } else if (!this.sortedChunks[i].inFrustum) {
                    ++this.invisibleChunkCount;
                } else if (this.occlusion && !this.sortedChunks[i].unoccluded) {
                    ++this.occludedChunkCount;
                } else {
                    ++this.compiledChunkCount;
                }
            }
            if (this.sortedChunks[i].renderLayerEmpty[layer] || !this.sortedChunks[i].inFrustum || this.occlusion && !this.sortedChunks[i].unoccluded || (n3 = this.sortedChunks[i].getRenderListId(layer)) < 0) continue;
            this.chunksInCurrentLayer.add(this.sortedChunks[i]);
            ++n2;
        }
        LivingEntity livingEntity = this.client.camera;
        double d = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * tickDelta;
        double d2 = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * tickDelta;
        double d3 = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * tickDelta;
        int n4 = 0;
        for (n = 0; n < this.chunkRenderers.length; ++n) {
            this.chunkRenderers[n].clear();
        }
        for (n = 0; n < this.chunksInCurrentLayer.size(); ++n) {
            ChunkBuilder chunkBuilder = (ChunkBuilder)this.chunksInCurrentLayer.get(n);
            int n5 = -1;
            for (int i = 0; i < n4; ++i) {
                if (!this.chunkRenderers[i].isAt(chunkBuilder.cameraOffsetX, chunkBuilder.cameraOffsetY, chunkBuilder.cameraOffsetZ)) continue;
                n5 = i;
            }
            if (n5 < 0) {
                n5 = n4++;
                this.chunkRenderers[n5].init(chunkBuilder.cameraOffsetX, chunkBuilder.cameraOffsetY, chunkBuilder.cameraOffsetZ, d, d2, d3);
            }
            this.chunkRenderers[n5].addGlList(chunkBuilder.getRenderListId(layer));
        }
        this.renderLastChunks(layer, tickDelta);
        return n2;
    }

    public void renderLastChunks(int layer, double tickDelta) {
        for (int i = 0; i < this.chunkRenderers.length; ++i) {
            this.chunkRenderers[i].render();
        }
    }

    public void tick() {
        ++this.ticks;
    }

    public void renderSky(float tickDelta) {
        float f;
        float f2;
        float f3;
        float f4;
        if (this.client.world.dimension.isNether) {
            return;
        }
        GL11.glDisable(3553);
        Vec3d vec3d = this.world.getSkyColor(this.client.camera, tickDelta);
        float f5 = (float)vec3d.x;
        float f6 = (float)vec3d.y;
        float f7 = (float)vec3d.z;
        if (this.client.options.anaglyph3d) {
            float f8 = (f5 * 30.0f + f6 * 59.0f + f7 * 11.0f) / 100.0f;
            float f9 = (f5 * 30.0f + f6 * 70.0f) / 100.0f;
            f4 = (f5 * 30.0f + f7 * 70.0f) / 100.0f;
            f5 = f8;
            f6 = f9;
            f7 = f4;
        }
        GL11.glColor3f(f5, f6, f7);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glDepthMask(false);
        GL11.glEnable(2912);
        GL11.glColor3f(f5, f6, f7);
        GL11.glCallList(this.lightSkyGlList);
        GL11.glDisable(2912);
        GL11.glDisable(3008);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        Lighting.turnOff();
        float[] fArray = this.world.dimension.getBackgroundColor(this.world.getTime(tickDelta), tickDelta);
        if (fArray != null) {
            float f10;
            GL11.glDisable(3553);
            GL11.glShadeModel(7425);
            GL11.glPushMatrix();
            GL11.glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            f4 = this.world.getTime(tickDelta);
            GL11.glRotatef(f4 > 0.5f ? 180.0f : 0.0f, 0.0f, 0.0f, 1.0f);
            f3 = fArray[0];
            f2 = fArray[1];
            f = fArray[2];
            if (this.client.options.anaglyph3d) {
                float f11 = (f3 * 30.0f + f2 * 59.0f + f * 11.0f) / 100.0f;
                float f12 = (f3 * 30.0f + f2 * 70.0f) / 100.0f;
                f10 = (f3 * 30.0f + f * 70.0f) / 100.0f;
                f3 = f11;
                f2 = f12;
                f = f10;
            }
            tessellator.start(6);
            tessellator.color(f3, f2, f, fArray[3]);
            tessellator.vertex(0.0, 100.0, 0.0);
            int n = 16;
            tessellator.color(fArray[0], fArray[1], fArray[2], 0.0f);
            for (int i = 0; i <= n; ++i) {
                f10 = (float)i * (float)Math.PI * 2.0f / (float)n;
                float f13 = MathHelper.sin(f10);
                float f14 = MathHelper.cos(f10);
                tessellator.vertex(f13 * 120.0f, f14 * 120.0f, -f14 * 40.0f * fArray[3]);
            }
            tessellator.draw();
            GL11.glPopMatrix();
            GL11.glShadeModel(7424);
        }
        GL11.glEnable(3553);
        GL11.glBlendFunc(770, 1);
        GL11.glPushMatrix();
        float f15 = 1.0f - this.world.getRainGradient(tickDelta);
        f4 = 0.0f;
        f3 = 0.0f;
        f2 = 0.0f;
        GL11.glColor4f(1.0f, 1.0f, 1.0f, f15);
        GL11.glTranslatef(f4, f3, f2);
        GL11.glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
        GL11.glRotatef(this.world.getTime(tickDelta) * 360.0f, 1.0f, 0.0f, 0.0f);
        f = 30.0f;
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/terrain/sun.png"));
        tessellator.startQuads();
        tessellator.vertex(-f, 100.0, -f, 0.0, 0.0);
        tessellator.vertex(f, 100.0, -f, 1.0, 0.0);
        tessellator.vertex(f, 100.0, f, 1.0, 1.0);
        tessellator.vertex(-f, 100.0, f, 0.0, 1.0);
        tessellator.draw();
        f = 20.0f;
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/terrain/moon.png"));
        tessellator.startQuads();
        tessellator.vertex(-f, -100.0, f, 1.0, 1.0);
        tessellator.vertex(f, -100.0, f, 0.0, 1.0);
        tessellator.vertex(f, -100.0, -f, 0.0, 0.0);
        tessellator.vertex(-f, -100.0, -f, 1.0, 0.0);
        tessellator.draw();
        GL11.glDisable(3553);
        float f16 = this.world.calculateSkyLightIntensity(tickDelta) * f15;
        if (f16 > 0.0f) {
            GL11.glColor4f(f16, f16, f16, f16);
            GL11.glCallList(this.starsGlList);
        }
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
        GL11.glEnable(3008);
        GL11.glEnable(2912);
        GL11.glPopMatrix();
        if (this.world.dimension.hasGround()) {
            GL11.glColor3f(f5 * 0.2f + 0.04f, f6 * 0.2f + 0.04f, f7 * 0.6f + 0.1f);
        } else {
            GL11.glColor3f(f5, f6, f7);
        }
        GL11.glDisable(3553);
        GL11.glCallList(this.darkSkyGlList);
        GL11.glEnable(3553);
        GL11.glDepthMask(true);
    }

    public void renderClouds(float tickDelta) {
        float f;
        if (this.client.world.dimension.isNether) {
            return;
        }
        if (this.client.options.fancyGraphics) {
            this.renderFancyClouds(tickDelta);
            return;
        }
        GL11.glDisable(2884);
        float f2 = (float)(this.client.camera.lastTickY + (this.client.camera.y - this.client.camera.lastTickY) * (double)tickDelta);
        int n = 32;
        int n2 = 256 / n;
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/environment/clouds.png"));
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        Vec3d vec3d = this.world.getCloudColor(tickDelta);
        float f3 = (float)vec3d.x;
        float f4 = (float)vec3d.y;
        float f5 = (float)vec3d.z;
        if (this.client.options.anaglyph3d) {
            f = (f3 * 30.0f + f4 * 59.0f + f5 * 11.0f) / 100.0f;
            float f6 = (f3 * 30.0f + f4 * 70.0f) / 100.0f;
            float f7 = (f3 * 30.0f + f5 * 70.0f) / 100.0f;
            f3 = f;
            f4 = f6;
            f5 = f7;
        }
        f = 4.8828125E-4f;
        double d = this.client.camera.prevX + (this.client.camera.x - this.client.camera.prevX) * (double)tickDelta + (double)(((float)this.ticks + tickDelta) * 0.03f);
        double d2 = this.client.camera.prevZ + (this.client.camera.z - this.client.camera.prevZ) * (double)tickDelta;
        int n3 = MathHelper.floor(d / 2048.0);
        int n4 = MathHelper.floor(d2 / 2048.0);
        float f8 = this.world.dimension.getCloudHeight() - f2 + 0.33f;
        float f9 = (float)((d -= (double)(n3 * 2048)) * (double)f);
        float f10 = (float)((d2 -= (double)(n4 * 2048)) * (double)f);
        tessellator.startQuads();
        tessellator.color(f3, f4, f5, 0.8f);
        for (int i = -n * n2; i < n * n2; i += n) {
            for (int j = -n * n2; j < n * n2; j += n) {
                tessellator.vertex(i + 0, f8, j + n, (float)(i + 0) * f + f9, (float)(j + n) * f + f10);
                tessellator.vertex(i + n, f8, j + n, (float)(i + n) * f + f9, (float)(j + n) * f + f10);
                tessellator.vertex(i + n, f8, j + 0, (float)(i + n) * f + f9, (float)(j + 0) * f + f10);
                tessellator.vertex(i + 0, f8, j + 0, (float)(i + 0) * f + f9, (float)(j + 0) * f + f10);
            }
        }
        tessellator.draw();
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
        GL11.glEnable(2884);
    }

    public boolean hasFog(double x, double y, double z, float tickDelta) {
        return false;
    }

    public void renderFancyClouds(float tickDelta) {
        float f;
        float f2;
        float f3;
        GL11.glDisable(2884);
        float f4 = (float)(this.client.camera.lastTickY + (this.client.camera.y - this.client.camera.lastTickY) * (double)tickDelta);
        Tessellator tessellator = Tessellator.INSTANCE;
        float f5 = 12.0f;
        float f6 = 4.0f;
        double d = (this.client.camera.prevX + (this.client.camera.x - this.client.camera.prevX) * (double)tickDelta + (double)(((float)this.ticks + tickDelta) * 0.03f)) / (double)f5;
        double d2 = (this.client.camera.prevZ + (this.client.camera.z - this.client.camera.prevZ) * (double)tickDelta) / (double)f5 + (double)0.33f;
        float f7 = this.world.dimension.getCloudHeight() - f4 + 0.33f;
        int n = MathHelper.floor(d / 2048.0);
        int n2 = MathHelper.floor(d2 / 2048.0);
        d -= (double)(n * 2048);
        d2 -= (double)(n2 * 2048);
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/environment/clouds.png"));
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        Vec3d vec3d = this.world.getCloudColor(tickDelta);
        float f8 = (float)vec3d.x;
        float f9 = (float)vec3d.y;
        float f10 = (float)vec3d.z;
        if (this.client.options.anaglyph3d) {
            f3 = (f8 * 30.0f + f9 * 59.0f + f10 * 11.0f) / 100.0f;
            f2 = (f8 * 30.0f + f9 * 70.0f) / 100.0f;
            f = (f8 * 30.0f + f10 * 70.0f) / 100.0f;
            f8 = f3;
            f9 = f2;
            f10 = f;
        }
        f3 = (float)(d * 0.0);
        f2 = (float)(d2 * 0.0);
        f = 0.00390625f;
        f3 = (float)MathHelper.floor(d) * f;
        f2 = (float)MathHelper.floor(d2) * f;
        float f11 = (float)(d - (double)MathHelper.floor(d));
        float f12 = (float)(d2 - (double)MathHelper.floor(d2));
        int n3 = 8;
        int n4 = 3;
        float f13 = 9.765625E-4f;
        GL11.glScalef(f5, 1.0f, f5);
        for (int i = 0; i < 2; ++i) {
            if (i == 0) {
                GL11.glColorMask(false, false, false, false);
            } else if (this.client.options.anaglyph3d) {
                if (GameRenderer.activeEye == 0) {
                    GL11.glColorMask(false, true, true, true);
                } else {
                    GL11.glColorMask(true, false, false, true);
                }
            } else {
                GL11.glColorMask(true, true, true, true);
            }
            for (int j = -n4 + 1; j <= n4; ++j) {
                for (int k = -n4 + 1; k <= n4; ++k) {
                    int n5;
                    tessellator.startQuads();
                    float f14 = j * n3;
                    float f15 = k * n3;
                    float f16 = f14 - f11;
                    float f17 = f15 - f12;
                    if (f7 > -f6 - 1.0f) {
                        tessellator.color(f8 * 0.7f, f9 * 0.7f, f10 * 0.7f, 0.8f);
                        tessellator.normal(0.0f, -1.0f, 0.0f);
                        tessellator.vertex(f16 + 0.0f, f7 + 0.0f, f17 + (float)n3, (f14 + 0.0f) * f + f3, (f15 + (float)n3) * f + f2);
                        tessellator.vertex(f16 + (float)n3, f7 + 0.0f, f17 + (float)n3, (f14 + (float)n3) * f + f3, (f15 + (float)n3) * f + f2);
                        tessellator.vertex(f16 + (float)n3, f7 + 0.0f, f17 + 0.0f, (f14 + (float)n3) * f + f3, (f15 + 0.0f) * f + f2);
                        tessellator.vertex(f16 + 0.0f, f7 + 0.0f, f17 + 0.0f, (f14 + 0.0f) * f + f3, (f15 + 0.0f) * f + f2);
                    }
                    if (f7 <= f6 + 1.0f) {
                        tessellator.color(f8, f9, f10, 0.8f);
                        tessellator.normal(0.0f, 1.0f, 0.0f);
                        tessellator.vertex(f16 + 0.0f, f7 + f6 - f13, f17 + (float)n3, (f14 + 0.0f) * f + f3, (f15 + (float)n3) * f + f2);
                        tessellator.vertex(f16 + (float)n3, f7 + f6 - f13, f17 + (float)n3, (f14 + (float)n3) * f + f3, (f15 + (float)n3) * f + f2);
                        tessellator.vertex(f16 + (float)n3, f7 + f6 - f13, f17 + 0.0f, (f14 + (float)n3) * f + f3, (f15 + 0.0f) * f + f2);
                        tessellator.vertex(f16 + 0.0f, f7 + f6 - f13, f17 + 0.0f, (f14 + 0.0f) * f + f3, (f15 + 0.0f) * f + f2);
                    }
                    tessellator.color(f8 * 0.9f, f9 * 0.9f, f10 * 0.9f, 0.8f);
                    if (j > -1) {
                        tessellator.normal(-1.0f, 0.0f, 0.0f);
                        for (n5 = 0; n5 < n3; ++n5) {
                            tessellator.vertex(f16 + (float)n5 + 0.0f, f7 + 0.0f, f17 + (float)n3, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + (float)n3) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 0.0f, f7 + f6, f17 + (float)n3, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + (float)n3) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 0.0f, f7 + f6, f17 + 0.0f, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + 0.0f) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 0.0f, f7 + 0.0f, f17 + 0.0f, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + 0.0f) * f + f2);
                        }
                    }
                    if (j <= 1) {
                        tessellator.normal(1.0f, 0.0f, 0.0f);
                        for (n5 = 0; n5 < n3; ++n5) {
                            tessellator.vertex(f16 + (float)n5 + 1.0f - f13, f7 + 0.0f, f17 + (float)n3, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + (float)n3) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 1.0f - f13, f7 + f6, f17 + (float)n3, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + (float)n3) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 1.0f - f13, f7 + f6, f17 + 0.0f, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + 0.0f) * f + f2);
                            tessellator.vertex(f16 + (float)n5 + 1.0f - f13, f7 + 0.0f, f17 + 0.0f, (f14 + (float)n5 + 0.5f) * f + f3, (f15 + 0.0f) * f + f2);
                        }
                    }
                    tessellator.color(f8 * 0.8f, f9 * 0.8f, f10 * 0.8f, 0.8f);
                    if (k > -1) {
                        tessellator.normal(0.0f, 0.0f, -1.0f);
                        for (n5 = 0; n5 < n3; ++n5) {
                            tessellator.vertex(f16 + 0.0f, f7 + f6, f17 + (float)n5 + 0.0f, (f14 + 0.0f) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + (float)n3, f7 + f6, f17 + (float)n5 + 0.0f, (f14 + (float)n3) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + (float)n3, f7 + 0.0f, f17 + (float)n5 + 0.0f, (f14 + (float)n3) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + 0.0f, f7 + 0.0f, f17 + (float)n5 + 0.0f, (f14 + 0.0f) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                        }
                    }
                    if (k <= 1) {
                        tessellator.normal(0.0f, 0.0f, 1.0f);
                        for (n5 = 0; n5 < n3; ++n5) {
                            tessellator.vertex(f16 + 0.0f, f7 + f6, f17 + (float)n5 + 1.0f - f13, (f14 + 0.0f) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + (float)n3, f7 + f6, f17 + (float)n5 + 1.0f - f13, (f14 + (float)n3) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + (float)n3, f7 + 0.0f, f17 + (float)n5 + 1.0f - f13, (f14 + (float)n3) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                            tessellator.vertex(f16 + 0.0f, f7 + 0.0f, f17 + (float)n5 + 1.0f - f13, (f14 + 0.0f) * f + f3, (f15 + (float)n5 + 0.5f) * f + f2);
                        }
                    }
                    tessellator.draw();
                }
            }
        }
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        GL11.glDisable(3042);
        GL11.glEnable(2884);
    }

    public boolean compileChunks(LivingEntity camera, boolean force) {
        int n6 = 2;
        if (this.dirtyChunkSorterCached == null) {
            this.dirtyChunkSorterCached = new DirtyChunkSorter(camera);
        } else {
            this.dirtyChunkSorterCached.update(camera);
        }
        DirtyChunkSorter dirtyChunkSorter = this.dirtyChunkSorterCached;
        ChunkBuilder[] chunkBuilderArray = new ChunkBuilder[n6];
        ArrayList<ChunkBuilder> toRebuild = null;
        int n7 = this.dirtyChunks.size();
        int n8 = 0;
        for (ChunkBuilder chunkBuilder : this.dirtyChunks) {
            if (!force) {
                if (chunkBuilder.squaredDistanceTo(camera) > 256.0f) {
                    int n9;
                    for (n9 = 0; n9 < n6 && (chunkBuilderArray[n9] == null || dirtyChunkSorter.compare(chunkBuilderArray[n9], chunkBuilder) <= 0); ++n9) {
                    }
                    if (--n9 <= 0) continue;
                    int n2 = n9;
                    while (--n2 != 0) {
                        chunkBuilderArray[n2 - 1] = chunkBuilderArray[n2];
                    }
                    chunkBuilderArray[n9] = chunkBuilder;
                    continue;
                }
            } else if (!chunkBuilder.inFrustum) continue;
            if (toRebuild == null) {
                toRebuild = new ArrayList<ChunkBuilder>();
            }
            ++n8;
            toRebuild.add(chunkBuilder);
        }
        if (toRebuild != null) {
            if (toRebuild.size() > 1) {
                Collections.sort(toRebuild, dirtyChunkSorter);
            }
            // cap per-frame near-chunk rebuilds to avoid multi-frame spikes when entering new areas
            int rebuildLimit = force ? toRebuild.size() : Math.min(toRebuild.size(), 4);
            List rebuiltThisFrame = toRebuild.subList(toRebuild.size() - rebuildLimit, toRebuild.size());
            this.dirtyChunks.removeAll(rebuiltThisFrame);
            for (int i = rebuiltThisFrame.size() - 1; i >= 0; --i) {
                ChunkBuilder cb = (ChunkBuilder) rebuiltThisFrame.get(i);
                cb.rebuild();
                cb.dirty = false;
            }
        }
        int n3 = 0;
        for (int n = n6 - 1; n >= 0; --n) {
            ChunkBuilder chunkBuilder3 = chunkBuilderArray[n];
            if (chunkBuilder3 == null) continue;
            if (!chunkBuilder3.inFrustum && n != n6 - 1) {
                chunkBuilderArray[n] = null;
                chunkBuilderArray[0] = null;
                break;
            }
            this.dirtyChunks.remove(chunkBuilderArray[n]);
            chunkBuilderArray[n].rebuild();
            chunkBuilderArray[n].dirty = false;
            ++n3;
        }
        return n7 == n8 + n3;
    }

    public void renderMiningProgress(PlayerEntity entity, HitResult hit, int i, ItemStack handStack, float tickDelta) {
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glEnable(3042);
        GL11.glEnable(3008);
        GL11.glBlendFunc(770, 1);
        GL11.glColor4f(1.0f, 1.0f, 1.0f, (MathHelper.sin((float)System.currentTimeMillis() / 100.0f) * 0.2f + 0.4f) * 0.5f);
        if (i == 0) {
            if (this.miningProgress > 0.0f) {
                GL11.glBlendFunc(774, 768);
                int n = this.textureManager.getTextureId("/terrain.png");
                GL11.glBindTexture(3553, n);
                GL11.glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                GL11.glPushMatrix();
                int n2 = this.world.getBlockId(hit.blockX, hit.blockY, hit.blockZ);
                Block block = n2 > 0 ? Block.BLOCKS[n2] : null;
                GL11.glDisable(3008);
                GL11.glPolygonOffset(-3.0f, -3.0f);
                GL11.glEnable(32823);
                double d = entity.lastTickX + (entity.x - entity.lastTickX) * (double)tickDelta;
                double d2 = entity.lastTickY + (entity.y - entity.lastTickY) * (double)tickDelta;
                double d3 = entity.lastTickZ + (entity.z - entity.lastTickZ) * (double)tickDelta;
                if (block == null) {
                    block = Block.STONE;
                }
                GL11.glEnable(3008);
                tessellator.startQuads();
                tessellator.translate(-d, -d2, -d3);
                tessellator.disableColor();
                this.blockRenderManager.renderWithTexture(block, hit.blockX, hit.blockY, hit.blockZ, 240 + (int)(this.miningProgress * 10.0f));
                tessellator.draw();
                tessellator.translate(0.0, 0.0, 0.0);
                GL11.glDisable(3008);
                GL11.glPolygonOffset(0.0f, 0.0f);
                GL11.glDisable(32823);
                GL11.glEnable(3008);
                GL11.glDepthMask(true);
                GL11.glPopMatrix();
            }
        } else if (handStack != null) {
            GL11.glBlendFunc(770, 771);
            float f = MathHelper.sin((float)System.currentTimeMillis() / 100.0f) * 0.2f + 0.8f;
            GL11.glColor4f(f, f, f, MathHelper.sin((float)System.currentTimeMillis() / 200.0f) * 0.2f + 0.5f);
            int n = this.textureManager.getTextureId("/terrain.png");
            GL11.glBindTexture(3553, n);
            int n3 = hit.blockX;
            int n4 = hit.blockY;
            int n5 = hit.blockZ;
            if (hit.side == 0) {
                --n4;
            }
            if (hit.side == 1) {
                ++n4;
            }
            if (hit.side == 2) {
                --n5;
            }
            if (hit.side == 3) {
                ++n5;
            }
            if (hit.side == 4) {
                --n3;
            }
            if (hit.side == 5) {
                ++n3;
            }
        }
        GL11.glDisable(3042);
        GL11.glDisable(3008);
    }

    public void renderBlockOutline(PlayerEntity player, HitResult hitResult, int i, ItemStack handStack, float tickDelta) {
        if (i == 0 && hitResult.type == HitResultType.BLOCK) {
            GL11.glEnable(3042);
            GL11.glBlendFunc(770, 771);
            GL11.glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
            GL11.glLineWidth(2.0f);
            GL11.glDisable(3553);
            GL11.glDepthMask(false);
            float f = 0.002f;
            int n = this.world.getBlockId(hitResult.blockX, hitResult.blockY, hitResult.blockZ);
            if (n > 0) {
                Block.BLOCKS[n].updateBoundingBox(this.world, hitResult.blockX, hitResult.blockY, hitResult.blockZ);
                double d = player.lastTickX + (player.x - player.lastTickX) * (double)tickDelta;
                double d2 = player.lastTickY + (player.y - player.lastTickY) * (double)tickDelta;
                double d3 = player.lastTickZ + (player.z - player.lastTickZ) * (double)tickDelta;
                this.renderOutline(Block.BLOCKS[n].getBoundingBox(this.world, hitResult.blockX, hitResult.blockY, hitResult.blockZ).expand(f, f, f).offset(-d, -d2, -d3));
            }
            GL11.glDepthMask(true);
            GL11.glEnable(3553);
            GL11.glDisable(3042);
        }
    }

    private void renderOutline(Box box) {
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.start(GL11.GL_LINE_STRIP);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.draw();
        tessellator.start(GL11.GL_LINE_STRIP);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.draw();
        tessellator.start(GL11.GL_LINES);
        tessellator.vertex(box.minX, box.minY, box.minZ);
        tessellator.vertex(box.minX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.minZ);
        tessellator.vertex(box.maxX, box.maxY, box.minZ);
        tessellator.vertex(box.maxX, box.minY, box.maxZ);
        tessellator.vertex(box.maxX, box.maxY, box.maxZ);
        tessellator.vertex(box.minX, box.minY, box.maxZ);
        tessellator.vertex(box.minX, box.maxY, box.maxZ);
        tessellator.draw();
    }

    public void markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        int n = MathHelper.floorDiv(minX, 16);
        int n2 = MathHelper.floorDiv(minY, 16);
        int n3 = MathHelper.floorDiv(minZ, 16);
        int n4 = MathHelper.floorDiv(maxX, 16);
        int n5 = MathHelper.floorDiv(maxY, 16);
        int n6 = MathHelper.floorDiv(maxZ, 16);
        for (int i = n; i <= n4; ++i) {
            int n7 = i % this.chunkCountX;
            if (n7 < 0) {
                n7 += this.chunkCountX;
            }
            for (int j = n2; j <= n5; ++j) {
                int n8 = j % this.chunkCountY;
                if (n8 < 0) {
                    n8 += this.chunkCountY;
                }
                for (int k = n3; k <= n6; ++k) {
                    int n9 = k % this.chunkCountZ;
                    if (n9 < 0) {
                        n9 += this.chunkCountZ;
                    }
                    int n10 = (n9 * this.chunkCountY + n8) * this.chunkCountX + n7;
                    ChunkBuilder chunkBuilder = this.chunks[n10];
                    if (chunkBuilder.dirty) continue;
                    this.dirtyChunks.add(chunkBuilder);
                    chunkBuilder.invalidate();
                }
            }
        }
    }

    public void blockUpdate(int x, int y, int z) {
        this.markDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
    }

    public void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
        this.markDirty(minX - 1, minY - 1, minZ - 1, maxX + 1, maxY + 1, maxZ + 1);
    }

    public void cullChunks(Culler culler, float tickDelta) {
        for (int i = 0; i < this.chunks.length; ++i) {
            if (this.chunks[i].hasNoGeometry() || this.chunks[i].inFrustum && (i + this.cullStep & 0xF) != 0) continue;
            this.chunks[i].updateFrustum(culler);
        }
        ++this.cullStep;
    }

    public void playStreaming(String stream, int x, int y, int z) {
        if (stream != null) {
            this.client.inGameHud.setRecordPlayingOverlay("C418 - " + stream);
        }
        this.client.soundManager.playStreaming(stream, x, y, z, 1.0f, 1.0f);
    }

    public void playSound(String sound, double x, double y, double z, float volume, float pitch) {
        float f = 16.0f;
        if (volume > 1.0f) {
            f *= volume;
        }
        if (this.client.camera.getSquaredDistance(x, y, z) < (double)(f * f)) {
            this.client.soundManager.playSound(sound, (float)x, (float)y, (float)z, volume, pitch);
        }
    }

    public void addParticle(String particle, double x, double y, double z, double velocityX, double velocityY, double velocityZ) {
        if (this.client == null || this.client.camera == null || this.client.particleManager == null) {
            return;
        }
        double d = this.client.camera.x - x;
        double d2 = this.client.camera.y - y;
        double d3 = this.client.camera.z - z;
        double d4 = 16.0;
        if (d * d + d2 * d2 + d3 * d3 > d4 * d4) {
            return;
        }
        if (particle.equals("bubble")) {
            this.client.particleManager.addParticle(new WaterBubbleParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("smoke")) {
            this.client.particleManager.addParticle(new FireSmokeParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("note")) {
            this.client.particleManager.addParticle(new NoteParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("portal")) {
            this.client.particleManager.addParticle(new PortalParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("explode")) {
            this.client.particleManager.addParticle(new ExplosionParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("flame")) {
            this.client.particleManager.addParticle(new FlameParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("lava")) {
            this.client.particleManager.addParticle(new LavaEmberParticle(this.world, x, y, z));
        } else if (particle.equals("footstep")) {
            this.client.particleManager.addParticle(new FootstepParticle(this.textureManager, this.world, x, y, z));
        } else if (particle.equals("splash")) {
            this.client.particleManager.addParticle(new WaterSplashParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("largesmoke")) {
            this.client.particleManager.addParticle(new FireSmokeParticle(this.world, x, y, z, velocityX, velocityY, velocityZ, 2.5f));
        } else if (particle.equals("reddust")) {
            this.client.particleManager.addParticle(new RedDustParticle(this.world, x, y, z, (float)velocityX, (float)velocityY, (float)velocityZ));
        } else if (particle.equals("snowballpoof")) {
            this.client.particleManager.addParticle(new ItemParticle(this.world, x, y, z, Item.SNOWBALL));
        } else if (particle.equals("snowshovel")) {
            this.client.particleManager.addParticle(new SnowParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        } else if (particle.equals("slime")) {
            this.client.particleManager.addParticle(new ItemParticle(this.world, x, y, z, Item.SLIMEBALL));
        } else if (particle.equals("heart")) {
            this.client.particleManager.addParticle(new HeartParticle(this.world, x, y, z, velocityX, velocityY, velocityZ));
        }
    }

    public void notifyEntityAdded(Entity entity) {
        entity.updateCapeUrl();
        if (entity.skinUrl != null) {
            this.textureManager.downloadImage(entity.skinUrl, new SkinImageProcessor());
        }
        if (entity.capeUrl != null) {
            this.textureManager.downloadImage(entity.capeUrl, new SkinImageProcessor());
        }
    }

    public void notifyEntityRemoved(Entity entity) {
        if (entity.skinUrl != null) {
            this.textureManager.releaseImage(entity.skinUrl);
        }
        if (entity.capeUrl != null) {
            this.textureManager.releaseImage(entity.capeUrl);
        }
    }

    public void notifyAmbientDarknessChanged() {
        for (int i = 0; i < this.chunks.length; ++i) {
            if (!this.chunks[i].hasSkyLight || this.chunks[i].dirty) continue;
            this.dirtyChunks.add(this.chunks[i]);
            this.chunks[i].invalidate();
        }
    }

    public void updateBlockEntity(int x, int y, int z, BlockEntity blockEntity) {
    }

    public void releaseGlLists() {
        GlAllocationUtils.deleteDisplayLists(this.chunkGlList);
    }

    public void worldEvent(PlayerEntity player, int event, int x, int y, int z, int data) {
        Random random = this.world.random;
        switch (event) {
            case 1001: {
                this.world.playSound(x, y, z, "random.click", 1.0f, 1.2f);
                break;
            }
            case 1000: {
                this.world.playSound(x, y, z, "random.click", 1.0f, 1.0f);
                break;
            }
            case 1002: {
                this.world.playSound(x, y, z, "random.bow", 1.0f, 1.2f);
                break;
            }
            case 2000: {
                int n = data % 3 - 1;
                int n2 = data / 3 % 3 - 1;
                double d = (double)x + (double)n * 0.6 + 0.5;
                double d2 = (double)y + 0.5;
                double d3 = (double)z + (double)n2 * 0.6 + 0.5;
                for (int i = 0; i < 10; ++i) {
                    double d4 = random.nextDouble() * 0.2 + 0.01;
                    double d5 = d + (double)n * 0.01 + (random.nextDouble() - 0.5) * (double)n2 * 0.5;
                    double d6 = d2 + (random.nextDouble() - 0.5) * 0.5;
                    double d7 = d3 + (double)n2 * 0.01 + (random.nextDouble() - 0.5) * (double)n * 0.5;
                    double d8 = (double)n * d4 + random.nextGaussian() * 0.01;
                    double d9 = -0.03 + random.nextGaussian() * 0.01;
                    double d10 = (double)n2 * d4 + random.nextGaussian() * 0.01;
                    this.addParticle("smoke", d5, d6, d7, d8, d9, d10);
                }
                break;
            }
            case 2001: {
                int n = data & 0xFF;
                if (n > 0) {
                    Block block = Block.BLOCKS[n];
                    this.client.soundManager.playSound(block.soundGroup.getBreakSound(), (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, (block.soundGroup.getVolume() + 1.0f) / 2.0f, block.soundGroup.getPitch() * 0.8f);
                }
                this.client.particleManager.addBlockBreakParticles(x, y, z, data & 0xFF, data >> 8 & 0xFF);
                break;
            }
            case 1003: {
                if (Math.random() < 0.5) {
                    this.world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "random.door_open", 1.0f, this.world.random.nextFloat() * 0.1f + 0.9f);
                    break;
                }
                this.world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "random.door_close", 1.0f, this.world.random.nextFloat() * 0.1f + 0.9f);
                break;
            }
            case 1004: {
                this.world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "random.fizz", 0.5f, 2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f);
                break;
            }
            case 1005: {
                if (Item.ITEMS[data] instanceof MusicDiscItem) {
                    this.world.playStreaming(((MusicDiscItem)Item.ITEMS[data]).sound, x, y, z);
                    break;
                }
                this.world.playStreaming(null, x, y, z);
            }
        }
    }
}

