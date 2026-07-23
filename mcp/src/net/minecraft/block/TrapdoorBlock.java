/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class TrapdoorBlock
extends Block {
    protected TrapdoorBlock(int i, Material arg) {
        super(i, arg);
        this.textureId = 84;
        if (arg == Material.METAL) {
            ++this.textureId;
        }
        float f = 0.5f;
        float f2 = 1.0f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f2, 0.5f + f);
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        this.updateBoundingBox(world, x, y, z);
        return super.getBoundingBox(world, x, y, z);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        this.updateBoundingBox(world, x, y, z);
        return super.getCollisionShape(world, x, y, z);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        this.updateBoundingBox(blockView.getBlockMeta(x, y, z));
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
        float f = 0.1875f;
        this.setBoundingBox(0.0f, 0.5f - f / 2.0f, 0.0f, 1.0f, 0.5f + f / 2.0f, 1.0f);
    }

    public void updateBoundingBox(int meta) {
        float f = 0.1875f;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, f, 1.0f);
        if (TrapdoorBlock.isOpen(meta)) {
            if ((meta & 3) == 0) {
                this.setBoundingBox(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
            }
            if ((meta & 3) == 1) {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
            }
            if ((meta & 3) == 2) {
                this.setBoundingBox(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            }
            if ((meta & 3) == 3) {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
            }
        }
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.onUse(world, x, y, z, player);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (this.material == Material.METAL) {
            return true;
        }
        int n = world.getBlockMeta(x, y, z);
        world.setBlockMeta(x, y, z, n ^ 4);
        world.worldEvent(player, 1003, x, y, z, 0);
        return true;
    }

    public void setOpen(World world, int x, int y, int z, boolean open) {
        boolean bl;
        int n = world.getBlockMeta(x, y, z);
        boolean bl2 = bl = (n & 4) > 0;
        if (bl == open) {
            return;
        }
        world.setBlockMeta(x, y, z, n ^ 4);
        world.worldEvent(null, 1003, x, y, z, 0);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        int n2 = x;
        int n3 = z;
        if ((n & 3) == 0) {
            ++n3;
        }
        if ((n & 3) == 1) {
            --n3;
        }
        if ((n & 3) == 2) {
            ++n2;
        }
        if ((n & 3) == 3) {
            --n2;
        }
        if (!world.shouldSuffocate(n2, y, n3)) {
            world.setBlock(x, y, z, 0);
            this.dropStacks(world, x, y, z, n);
        }
        if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower()) {
            boolean bl = world.isEmittingRedstonePower(x, y, z);
            this.setOpen(world, x, y, z, bl);
        }
    }

    public HitResult raycast(World world, int x, int y, int z, Vec3d startPos, Vec3d endPos) {
        this.updateBoundingBox(world, x, y, z);
        return super.raycast(world, x, y, z, startPos, endPos);
    }

    public void onPlaced(World world, int x, int y, int z, int direction) {
        int n = 0;
        if (direction == 2) {
            n = 0;
        }
        if (direction == 3) {
            n = 1;
        }
        if (direction == 4) {
            n = 2;
        }
        if (direction == 5) {
            n = 3;
        }
        world.setBlockMeta(x, y, z, n);
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
        if (side == 0) {
            return false;
        }
        if (side == 1) {
            return false;
        }
        if (side == 2) {
            ++z;
        }
        if (side == 3) {
            --z;
        }
        if (side == 4) {
            ++x;
        }
        if (side == 5) {
            --x;
        }
        return world.shouldSuffocate(x, y, z);
    }

    public static boolean isOpen(int meta) {
        return (meta & 4) != 0;
    }
}

