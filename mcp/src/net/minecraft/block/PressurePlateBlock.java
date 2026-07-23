/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.PressurePlateActivationRule;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class PressurePlateBlock
extends Block {
    private PressurePlateActivationRule activationRule;

    protected PressurePlateBlock(int id, int textureId, PressurePlateActivationRule activationRule, Material material) {
        super(id, textureId, material);
        this.activationRule = activationRule;
        this.setTickRandomly(true);
        float f = 0.0625f;
        this.setBoundingBox(f, 0.0f, f, 1.0f - f, 0.03125f, 1.0f - f);
    }

    public int getTickRate() {
        return 20;
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return world.shouldSuffocate(x, y - 1, z);
    }

    public void onPlaced(World world, int x, int y, int z) {
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        boolean bl = false;
        if (!world.shouldSuffocate(x, y - 1, z)) {
            bl = true;
        }
        if (bl) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        if (world.getBlockMeta(x, y, z) == 0) {
            return;
        }
        this.updatePlateState(world, x, y, z);
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        if (world.isRemote) {
            return;
        }
        if (world.getBlockMeta(x, y, z) == 1) {
            return;
        }
        this.updatePlateState(world, x, y, z);
    }

    private void updatePlateState(World world, int x, int y, int z) {
        boolean bl = world.getBlockMeta(x, y, z) == 1;
        boolean bl2 = false;
        float f = 0.125f;
        List list = null;
        if (this.activationRule == PressurePlateActivationRule.EVERYTHING) {
            list = world.getEntities(null, Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, (double)y + 0.25, (float)(z + 1) - f));
        }
        if (this.activationRule == PressurePlateActivationRule.MOBS) {
            list = world.collectEntitiesByClass(LivingEntity.class, Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, (double)y + 0.25, (float)(z + 1) - f));
        }
        if (this.activationRule == PressurePlateActivationRule.PLAYERS) {
            list = world.collectEntitiesByClass(PlayerEntity.class, Box.createCached((float)x + f, y, (float)z + f, (float)(x + 1) - f, (double)y + 0.25, (float)(z + 1) - f));
        }
        if (list.size() > 0) {
            bl2 = true;
        }
        if (bl2 && !bl) {
            world.setBlockMeta(x, y, z, 1);
            world.notifyNeighbors(x, y, z, this.id);
            world.notifyNeighbors(x, y - 1, z, this.id);
            world.setBlocksDirty(x, y, z, x, y, z);
            world.playSound((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.3f, 0.6f);
        }
        if (!bl2 && bl) {
            world.setBlockMeta(x, y, z, 0);
            world.notifyNeighbors(x, y, z, this.id);
            world.notifyNeighbors(x, y - 1, z, this.id);
            world.setBlocksDirty(x, y, z, x, y, z);
            world.playSound((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.3f, 0.5f);
        }
        if (bl2) {
            world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
        }
    }

    public void onBreak(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        if (n > 0) {
            world.notifyNeighbors(x, y, z, this.id);
            world.notifyNeighbors(x, y - 1, z, this.id);
        }
        super.onBreak(world, x, y, z);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        boolean bl = blockView.getBlockMeta(x, y, z) == 1;
        float f = 0.0625f;
        if (bl) {
            this.setBoundingBox(f, 0.0f, f, 1.0f - f, 0.03125f, 1.0f - f);
        } else {
            this.setBoundingBox(f, 0.0f, f, 1.0f - f, 0.0625f, 1.0f - f);
        }
    }

    public boolean isEmittingRedstonePowerInDirection(BlockView blockView, int x, int y, int z, int direction) {
        return blockView.getBlockMeta(x, y, z) > 0;
    }

    public boolean canTransferPowerInDirection(World world, int x, int y, int z, int direction) {
        if (world.getBlockMeta(x, y, z) == 0) {
            return false;
        }
        return direction == 1;
    }

    public boolean canEmitRedstonePower() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
        float f = 0.5f;
        float f2 = 0.125f;
        float f3 = 0.5f;
        this.setBoundingBox(0.5f - f, 0.5f - f2, 0.5f - f3, 0.5f + f, 0.5f + f2, 0.5f + f3);
    }

    public int getPistonBehavior() {
        return 1;
    }
}

