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
import net.minecraft.block.PistonBlock;
import net.minecraft.block.PistonConstants;
import net.minecraft.block.material.Material;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class PistonHeadBlock
extends Block {
    private int pistonHeadSprite = -1;

    public PistonHeadBlock(int id, int textureId) {
        super(id, textureId, Material.PISTON);
        this.setSoundGroup(STONE_SOUND_GROUP);
        this.setHardness(0.5f);
    }

    @Environment(value=EnvType.CLIENT)
    public void setSprite(int sprite) {
        this.pistonHeadSprite = sprite;
    }

    @Environment(value=EnvType.CLIENT)
    public void clearSprite() {
        this.pistonHeadSprite = -1;
    }

    public void onBreak(World world, int x, int y, int z) {
        super.onBreak(world, x, y, z);
        int n = world.getBlockMeta(x, y, z);
        int n2 = PistonConstants.TEXTURE_SIDES[PistonHeadBlock.getFacing(n)];
        int n3 = world.getBlockId(x += PistonConstants.HEAD_OFFSET_X[n2], y += PistonConstants.HEAD_OFFSET_Y[n2], z += PistonConstants.HEAD_OFFSET_Z[n2]);
        if ((n3 == Block.PISTON.id || n3 == Block.STICKY_PISTON.id) && PistonBlock.isExtended(n = world.getBlockMeta(x, y, z))) {
            Block.BLOCKS[n3].dropStacks(world, x, y, z, n);
            world.setBlock(x, y, z, 0);
        }
    }

    public int getTexture(int side, int meta) {
        int n = PistonHeadBlock.getFacing(meta);
        if (side == n) {
            if (this.pistonHeadSprite >= 0) {
                return this.pistonHeadSprite;
            }
            if ((meta & 8) != 0) {
                return this.textureId - 1;
            }
            return this.textureId;
        }
        if (side == PistonConstants.TEXTURE_SIDES[n]) {
            return 107;
        }
        return 108;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 17;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return false;
    }

    public boolean canPlaceAt(World world, int x, int y, int z, int side) {
        return false;
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public void addIntersectingBoundingBox(World world, int x, int y, int z, Box box, ArrayList boxes) {
        int n = world.getBlockMeta(x, y, z);
        switch (PistonHeadBlock.getFacing(n)) {
            case 0: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.375f, 0.25f, 0.375f, 0.625f, 1.0f, 0.625f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                break;
            }
            case 1: {
                this.setBoundingBox(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.375f, 0.0f, 0.375f, 0.625f, 0.75f, 0.625f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                break;
            }
            case 2: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.25f, 0.375f, 0.25f, 0.75f, 0.625f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                break;
            }
            case 3: {
                this.setBoundingBox(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.25f, 0.375f, 0.0f, 0.75f, 0.625f, 0.75f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                break;
            }
            case 4: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.375f, 0.25f, 0.25f, 0.625f, 0.75f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                break;
            }
            case 5: {
                this.setBoundingBox(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
                this.setBoundingBox(0.0f, 0.375f, 0.25f, 0.75f, 0.625f, 0.75f);
                super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
            }
        }
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        switch (PistonHeadBlock.getFacing(n)) {
            case 0: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
                break;
            }
            case 1: {
                this.setBoundingBox(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
                break;
            }
            case 2: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
                break;
            }
            case 3: {
                this.setBoundingBox(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
                break;
            }
            case 4: {
                this.setBoundingBox(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
                break;
            }
            case 5: {
                this.setBoundingBox(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        int n = PistonHeadBlock.getFacing(world.getBlockMeta(x, y, z));
        int n2 = world.getBlockId(x - PistonConstants.HEAD_OFFSET_X[n], y - PistonConstants.HEAD_OFFSET_Y[n], z - PistonConstants.HEAD_OFFSET_Z[n]);
        if (n2 != Block.PISTON.id && n2 != Block.STICKY_PISTON.id) {
            world.setBlock(x, y, z, 0);
        } else {
            Block.BLOCKS[n2].neighborUpdate(world, x - PistonConstants.HEAD_OFFSET_X[n], y - PistonConstants.HEAD_OFFSET_Y[n], z - PistonConstants.HEAD_OFFSET_Z[n], id);
        }
    }

    public static int getFacing(int meta) {
        return meta & 7;
    }
}

