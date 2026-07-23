/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.chunk;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.client.render.Culler;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.block.entity.BlockEntityRenderDispatcher;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.WorldRegion;
import net.minecraft.world.chunk.Chunk;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ChunkBuilder {
    public World world;
    private int baseRenderList = -1;
    private static Tessellator tessellator = Tessellator.INSTANCE;
    public static int chunkUpdates = 0;
    public int x;
    public int y;
    public int z;
    public int sizeX;
    public int sizeY;
    public int sizeZ;
    public int cameraOffsetX;
    public int cameraOffsetY;
    public int cameraOffsetZ;
    public int renderX;
    public int renderY;
    public int renderZ;
    public boolean inFrustum = false;
    public boolean[] renderLayerEmpty = new boolean[2];
    public int centerX;
    public int centerY;
    public int centerZ;
    public float radius;
    public boolean dirty;
    public Box cullingBox;
    public int id;
    public boolean unoccluded = true;
    public boolean occlusionQueryReady;
    public int occlusionQueryId;
    public boolean hasSkyLight;
    private boolean built = false;
    public List blockEntities = new ArrayList();
    private List currentBlockEntities;

    public ChunkBuilder(World world, List blockEntitiesUpdateList, int x, int y, int z, int size, int baseRenderListId) {
        this.world = world;
        this.currentBlockEntities = blockEntitiesUpdateList;
        this.sizeY = this.sizeZ = size;
        this.sizeX = this.sizeZ;
        this.radius = MathHelper.sqrt(this.sizeX * this.sizeX + this.sizeY * this.sizeY + this.sizeZ * this.sizeZ) / 2.0f;
        this.baseRenderList = baseRenderListId;
        this.x = -999;
        this.setPosition(x, y, z);
        this.dirty = false;
    }

    public void setPosition(int x, int y, int z) {
        if (x == this.x && y == this.y && z == this.z) {
            return;
        }
        this.reset();
        this.x = x;
        this.y = y;
        this.z = z;
        this.centerX = x + this.sizeX / 2;
        this.centerY = y + this.sizeY / 2;
        this.centerZ = z + this.sizeZ / 2;
        this.renderX = x & 0x3FF;
        this.renderY = y;
        this.renderZ = z & 0x3FF;
        this.cameraOffsetX = x - this.renderX;
        this.cameraOffsetY = y - this.renderY;
        this.cameraOffsetZ = z - this.renderZ;
        float f = 6.0f;
        this.cullingBox = Box.create((float)x - f, (float)y - f, (float)z - f, (float)(x + this.sizeX) + f, (float)(y + this.sizeY) + f, (float)(z + this.sizeZ) + f);
        GL11.glNewList(this.baseRenderList + 2, 4864);
        ItemRenderer.renderShapeFlat(Box.createCached((float)this.renderX - f, (float)this.renderY - f, (float)this.renderZ - f, (float)(this.renderX + this.sizeX) + f, (float)(this.renderY + this.sizeY) + f, (float)(this.renderZ + this.sizeZ) + f));
        GL11.glEndList();
        this.invalidate();
    }

    private void translateToRenderPosition() {
        GL11.glTranslatef(this.renderX, this.renderY, this.renderZ);
    }

    public void rebuild() {
        if (!this.dirty) {
            return;
        }
        ++chunkUpdates;
        int n = this.x;
        int n2 = this.y;
        int n3 = this.z;
        int n4 = this.x + this.sizeX;
        int n5 = this.y + this.sizeY;
        int n6 = this.z + this.sizeZ;
        for (int i = 0; i < 2; ++i) {
            this.renderLayerEmpty[i] = true;
        }
        Chunk.hasSkyLight = false;
        HashSet hashSet = new HashSet();
        hashSet.addAll(this.blockEntities);
        this.blockEntities.clear();
        int n7 = 1;
        WorldRegion worldRegion = new WorldRegion(this.world, n - n7, n2 - n7, n3 - n7, n4 + n7, n5 + n7, n6 + n7);
        BlockRenderManager blockRenderManager = new BlockRenderManager(worldRegion);
        for (int i = 0; i < 2; ++i) {
            boolean bl = false;
            boolean bl2 = false;
            boolean bl3 = false;
            for (int j = n2; j < n5; ++j) {
                for (int k = n3; k < n6; ++k) {
                    for (int i2 = n; i2 < n4; ++i2) {
                        Block block;
                        int n8;
                        BlockEntity blockEntity;
                        int n9 = worldRegion.getBlockId(i2, j, k);
                        if (n9 <= 0) continue;
                        if (!bl3) {
                            bl3 = true;
                            GL11.glNewList(this.baseRenderList + i, 4864);
                            GL11.glPushMatrix();
                            this.translateToRenderPosition();
                            float f = 1.000001f;
                            GL11.glTranslatef((float)(-this.sizeZ) / 2.0f, (float)(-this.sizeY) / 2.0f, (float)(-this.sizeZ) / 2.0f);
                            GL11.glScalef(f, f, f);
                            GL11.glTranslatef((float)this.sizeZ / 2.0f, (float)this.sizeY / 2.0f, (float)this.sizeZ / 2.0f);
                            tessellator.startQuads();
                            tessellator.translate((double)(-this.x), (double)(-this.y), (double)(-this.z));
                        }
                        if (i == 0 && Block.BLOCKS_WITH_ENTITY[n9] && BlockEntityRenderDispatcher.INSTANCE.hasRenderer(blockEntity = worldRegion.getBlockEntity(i2, j, k))) {
                            this.blockEntities.add(blockEntity);
                        }
                        if ((n8 = (block = Block.BLOCKS[n9]).getRenderLayer()) != i) {
                            bl = true;
                            continue;
                        }
                        if (n8 != i) continue;
                        bl2 |= blockRenderManager.render(block, i2, j, k);
                    }
                }
            }
            if (bl3) {
                tessellator.draw();
                GL11.glPopMatrix();
                GL11.glEndList();
                tessellator.translate(0.0, 0.0, 0.0);
            } else {
                bl2 = false;
            }
            if (bl2) {
                this.renderLayerEmpty[i] = false;
            }
            if (!bl) break;
        }
        HashSet hashSet2 = new HashSet();
        hashSet2.addAll(this.blockEntities);
        hashSet2.removeAll(hashSet);
        this.currentBlockEntities.addAll(hashSet2);
        hashSet.removeAll(this.blockEntities);
        this.currentBlockEntities.removeAll(hashSet);
        this.hasSkyLight = Chunk.hasSkyLight;
        this.built = true;
    }

    public float squaredDistanceTo(Entity entity) {
        float f = (float)(entity.x - (double)this.centerX);
        float f2 = (float)(entity.y - (double)this.centerY);
        float f3 = (float)(entity.z - (double)this.centerZ);
        return f * f + f2 * f2 + f3 * f3;
    }

    public void reset() {
        for (int i = 0; i < 2; ++i) {
            this.renderLayerEmpty[i] = true;
        }
        this.inFrustum = false;
        this.built = false;
    }

    public void close() {
        this.reset();
        this.world = null;
    }

    public int getRenderListId(int layerId) {
        if (!this.inFrustum) {
            return -1;
        }
        if (!this.renderLayerEmpty[layerId]) {
            return this.baseRenderList + layerId;
        }
        return -1;
    }

    public void updateFrustum(Culler culler) {
        this.inFrustum = culler.isVisible(this.cullingBox);
    }

    public void renderOcclusionBox() {
        GL11.glCallList(this.baseRenderList + 2);
    }

    public boolean hasNoGeometry() {
        if (!this.built) {
            return false;
        }
        return this.renderLayerEmpty[0] && this.renderLayerEmpty[1];
    }

    public void invalidate() {
        this.dirty = true;
    }
}

