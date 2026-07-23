/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.ArrayList;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class StairsBlock
extends Block {
    private Block baseBlock;

    protected StairsBlock(int id, Block block) {
        super(id, block.textureId, block.material);
        this.baseBlock = block;
        this.setHardness(block.hardness);
        this.setResistance(block.resistance / 3.0f);
        this.setSoundGroup(block.soundGroup);
        this.setOpacity(255);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return super.getCollisionShape(world, x, y, z);
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 10;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        return super.isSideVisible(blockView, x, y, z, side);
    }

    public void addIntersectingBoundingBox(World world, int x, int y, int z, Box box, ArrayList boxes) {
        int n = world.getBlockMeta(x, y, z);
        if (n == 0) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
            this.setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
        } else if (n == 1) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
            this.setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
        } else if (n == 2) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
            this.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
        } else if (n == 3) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
            this.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
            super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
        }
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        this.baseBlock.randomDisplayTick(world, x, y, z, random);
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.baseBlock.onBlockBreakStart(world, x, y, z, player);
    }

    public void onMetadataChange(World world, int x, int y, int z, int meta) {
        this.baseBlock.onMetadataChange(world, x, y, z, meta);
    }

    @Environment(value=EnvType.CLIENT)
    public float getLuminance(BlockView blockView, int x, int y, int z) {
        return this.baseBlock.getLuminance(blockView, x, y, z);
    }

    public float getBlastResistance(Entity entity) {
        return this.baseBlock.getBlastResistance(entity);
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return this.baseBlock.getRenderLayer();
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return this.baseBlock.getDroppedItemId(blockMeta, random);
    }

    public int getDroppedItemCount(Random random) {
        return this.baseBlock.getDroppedItemCount(random);
    }

    public int getTexture(int side, int meta) {
        return this.baseBlock.getTexture(side, meta);
    }

    public int getTexture(int side) {
        return this.baseBlock.getTexture(side);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(BlockView blockView, int x, int y, int z, int side) {
        return this.baseBlock.getTextureId(blockView, x, y, z, side);
    }

    public int getTickRate() {
        return this.baseBlock.getTickRate();
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        return this.baseBlock.getBoundingBox(world, x, y, z);
    }

    public void applyVelocity(World world, int x, int y, int z, Entity entity, Vec3d velocity) {
        this.baseBlock.applyVelocity(world, x, y, z, entity, velocity);
    }

    public boolean hasCollision() {
        return this.baseBlock.hasCollision();
    }

    public boolean hasCollision(int meta, boolean allowLiquids) {
        return this.baseBlock.hasCollision(meta, allowLiquids);
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return this.baseBlock.canPlaceAt(world, x, y, z);
    }

    public void onPlaced(World world, int x, int y, int z) {
        this.neighborUpdate(world, x, y, z, 0);
        this.baseBlock.onPlaced(world, x, y, z);
    }

    public void onBreak(World world, int x, int y, int z) {
        this.baseBlock.onBreak(world, x, y, z);
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        this.baseBlock.dropStacks(world, x, y, z, meta, luck);
    }

    public void onSteppedOn(World world, int x, int y, int z, Entity entity) {
        this.baseBlock.onSteppedOn(world, x, y, z, entity);
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        this.baseBlock.onTick(world, x, y, z, random);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        return this.baseBlock.onUse(world, x, y, z, player);
    }

    public void onDestroyedByExplosion(World world, int x, int y, int z) {
        this.baseBlock.onDestroyedByExplosion(world, x, y, z);
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
        int n = MathHelper.floor((double)(placer.yaw * 4.0f / 360.0f) + 0.5) & 3;
        if (n == 0) {
            world.setBlockMeta(x, y, z, 2);
        }
        if (n == 1) {
            world.setBlockMeta(x, y, z, 1);
        }
        if (n == 2) {
            world.setBlockMeta(x, y, z, 3);
        }
        if (n == 3) {
            world.setBlockMeta(x, y, z, 0);
        }
    }
}

