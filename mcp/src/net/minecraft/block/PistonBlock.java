/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.ArrayList;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.PistonConstants;
import net.minecraft.block.PistonExtensionBlock;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.PistonBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class PistonBlock
extends Block {
    private boolean sticky;
    private boolean deaf;

    public PistonBlock(int id, int textureId, boolean sticky) {
        super(id, textureId, Material.PISTON);
        this.sticky = sticky;
        this.setSoundGroup(STONE_SOUND_GROUP);
        this.setHardness(0.5f);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTopTexture() {
        if (this.sticky) {
            return 106;
        }
        return 107;
    }

    public int getTexture(int side, int meta) {
        int n = PistonBlock.getFacing(meta);
        if (n > 5) {
            return this.textureId;
        }
        if (side == n) {
            if (PistonBlock.isExtended(meta) || this.minX > 0.0 || this.minY > 0.0 || this.minZ > 0.0 || this.maxX < 1.0 || this.maxY < 1.0 || this.maxZ < 1.0) {
                return 110;
            }
            return this.textureId;
        }
        if (side == PistonConstants.TEXTURE_SIDES[n]) {
            return 109;
        }
        return 108;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 16;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        return false;
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
        int n = PistonBlock.getFacingForPlacement(world, x, y, z, (PlayerEntity)placer);
        world.setBlockMeta(x, y, z, n);
        if (!world.isRemote) {
            this.checkExtended(world, x, y, z);
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (!world.isRemote && !this.deaf) {
            this.checkExtended(world, x, y, z);
        }
    }

    public void onPlaced(World world, int x, int y, int z) {
        if (!world.isRemote && world.getBlockEntity(x, y, z) == null) {
            this.checkExtended(world, x, y, z);
        }
    }

    private void checkExtended(World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = PistonBlock.getFacing(n);
        boolean bl = this.shouldExtend(world, x, y, z, n2);
        if (n == 7) {
            return;
        }
        if (bl && !PistonBlock.isExtended(n)) {
            if (PistonBlock.canExtend(world, x, y, z, n2)) {
                world.setBlockMetaWithoutNotifyingNeighbors(x, y, z, n2 | 8);
                world.playNoteBlockActionAt(x, y, z, 0, n2);
            }
        } else if (!bl && PistonBlock.isExtended(n)) {
            world.setBlockMetaWithoutNotifyingNeighbors(x, y, z, n2);
            world.playNoteBlockActionAt(x, y, z, 1, n2);
        }
    }

    private boolean shouldExtend(World world, int x, int y, int z, int facing) {
        if (facing != 0 && world.isEmittingRedstonePowerInDirection(x, y - 1, z, 0)) {
            return true;
        }
        if (facing != 1 && world.isEmittingRedstonePowerInDirection(x, y + 1, z, 1)) {
            return true;
        }
        if (facing != 2 && world.isEmittingRedstonePowerInDirection(x, y, z - 1, 2)) {
            return true;
        }
        if (facing != 3 && world.isEmittingRedstonePowerInDirection(x, y, z + 1, 3)) {
            return true;
        }
        if (facing != 5 && world.isEmittingRedstonePowerInDirection(x + 1, y, z, 5)) {
            return true;
        }
        if (facing != 4 && world.isEmittingRedstonePowerInDirection(x - 1, y, z, 4)) {
            return true;
        }
        if (world.isEmittingRedstonePowerInDirection(x, y, z, 0)) {
            return true;
        }
        if (world.isEmittingRedstonePowerInDirection(x, y + 2, z, 1)) {
            return true;
        }
        if (world.isEmittingRedstonePowerInDirection(x, y + 1, z - 1, 2)) {
            return true;
        }
        if (world.isEmittingRedstonePowerInDirection(x, y + 1, z + 1, 3)) {
            return true;
        }
        if (world.isEmittingRedstonePowerInDirection(x - 1, y + 1, z, 4)) {
            return true;
        }
        return world.isEmittingRedstonePowerInDirection(x + 1, y + 1, z, 5);
    }

    public void onBlockAction(World world, int x, int y, int z, int data1, int data2) {
        this.deaf = true;
        int n = data2;
        if (data1 == 0) {
            if (this.push(world, x, y, z, n)) {
                world.setBlockMeta(x, y, z, n | 8);
                world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "tile.piston.out", 0.5f, world.random.nextFloat() * 0.25f + 0.6f);
            }
        } else if (data1 == 1) {
            BlockEntity blockEntity = world.getBlockEntity(x + PistonConstants.HEAD_OFFSET_X[n], y + PistonConstants.HEAD_OFFSET_Y[n], z + PistonConstants.HEAD_OFFSET_Z[n]);
            if (blockEntity != null && blockEntity instanceof PistonBlockEntity) {
                ((PistonBlockEntity)blockEntity).finish();
            }
            world.setBlockWithoutNotifyingNeighbors(x, y, z, Block.MOVING_PISTON.id, n);
            world.setBlockEntity(x, y, z, PistonExtensionBlock.createPistonBlockEntity(this.id, n, n, false, true));
            if (this.sticky) {
                PistonBlockEntity pistonBlockEntity;
                BlockEntity blockEntity2;
                int n2 = x + PistonConstants.HEAD_OFFSET_X[n] * 2;
                int n3 = y + PistonConstants.HEAD_OFFSET_Y[n] * 2;
                int n4 = z + PistonConstants.HEAD_OFFSET_Z[n] * 2;
                int n5 = world.getBlockId(n2, n3, n4);
                int n6 = world.getBlockMeta(n2, n3, n4);
                boolean bl = false;
                if (n5 == Block.MOVING_PISTON.id && (blockEntity2 = world.getBlockEntity(n2, n3, n4)) != null && blockEntity2 instanceof PistonBlockEntity && (pistonBlockEntity = (PistonBlockEntity)blockEntity2).getFacing() == n && pistonBlockEntity.isExtending()) {
                    pistonBlockEntity.finish();
                    n5 = pistonBlockEntity.getPushedBlockId();
                    n6 = pistonBlockEntity.getPushedBlockData();
                    bl = true;
                }
                if (!bl && n5 > 0 && PistonBlock.canMoveBlock(n5, world, n2, n3, n4, false) && (Block.BLOCKS[n5].getPistonBehavior() == 0 || n5 == Block.PISTON.id || n5 == Block.STICKY_PISTON.id)) {
                    this.deaf = false;
                    world.setBlock(n2, n3, n4, 0);
                    this.deaf = true;
                    world.setBlockWithoutNotifyingNeighbors(x += PistonConstants.HEAD_OFFSET_X[n], y += PistonConstants.HEAD_OFFSET_Y[n], z += PistonConstants.HEAD_OFFSET_Z[n], Block.MOVING_PISTON.id, n6);
                    world.setBlockEntity(x, y, z, PistonExtensionBlock.createPistonBlockEntity(n5, n6, n, false, false));
                } else if (!bl) {
                    this.deaf = false;
                    world.setBlock(x + PistonConstants.HEAD_OFFSET_X[n], y + PistonConstants.HEAD_OFFSET_Y[n], z + PistonConstants.HEAD_OFFSET_Z[n], 0);
                    this.deaf = true;
                }
            } else {
                this.deaf = false;
                world.setBlock(x + PistonConstants.HEAD_OFFSET_X[n], y + PistonConstants.HEAD_OFFSET_Y[n], z + PistonConstants.HEAD_OFFSET_Z[n], 0);
                this.deaf = true;
            }
            world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "tile.piston.in", 0.5f, world.random.nextFloat() * 0.15f + 0.6f);
        }
        this.deaf = false;
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        if (PistonBlock.isExtended(n)) {
            switch (PistonBlock.getFacing(n)) {
                case 0: {
                    this.setBoundingBox(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 1: {
                    this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
                    break;
                }
                case 2: {
                    this.setBoundingBox(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 3: {
                    this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f);
                    break;
                }
                case 4: {
                    this.setBoundingBox(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 5: {
                    this.setBoundingBox(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);
                }
            }
        } else {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void setupRenderBoundingBox() {
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }

    public void addIntersectingBoundingBox(World world, int x, int y, int z, Box box, ArrayList boxes) {
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        super.addIntersectingBoundingBox(world, x, y, z, box, boxes);
    }

    public boolean isFullCube() {
        return false;
    }

    public static int getFacing(int meta) {
        return meta & 7;
    }

    public static boolean isExtended(int meta) {
        return (meta & 8) != 0;
    }

    private static int getFacingForPlacement(World world, int x, int y, int z, PlayerEntity player) {
        int n;
        if (MathHelper.abs((float)player.x - (float)x) < 2.0f && MathHelper.abs((float)player.z - (float)z) < 2.0f) {
            double d = player.y + 1.82 - (double)player.standingEyeHeight;
            if (d - (double)y > 2.0) {
                return 1;
            }
            if ((double)y - d > 0.0) {
                return 0;
            }
        }
        if ((n = MathHelper.floor((double)(player.yaw * 4.0f / 360.0f) + 0.5) & 3) == 0) {
            return 2;
        }
        if (n == 1) {
            return 5;
        }
        if (n == 2) {
            return 3;
        }
        if (n == 3) {
            return 4;
        }
        return 0;
    }

    private static boolean canMoveBlock(int id, World world, int x, int y, int z, boolean allowBreaking) {
        BlockEntity blockEntity;
        if (id == Block.OBSIDIAN.id) {
            return false;
        }
        if (id == Block.PISTON.id || id == Block.STICKY_PISTON.id) {
            if (PistonBlock.isExtended(world.getBlockMeta(x, y, z))) {
                return false;
            }
        } else {
            if (Block.BLOCKS[id].getHardness() == -1.0f) {
                return false;
            }
            if (Block.BLOCKS[id].getPistonBehavior() == 2) {
                return false;
            }
            if (!allowBreaking && Block.BLOCKS[id].getPistonBehavior() == 1) {
                return false;
            }
        }
        return (blockEntity = world.getBlockEntity(x, y, z)) == null;
    }

    private static boolean canExtend(World world, int x, int y, int z, int dir) {
        int n = x + PistonConstants.HEAD_OFFSET_X[dir];
        int n2 = y + PistonConstants.HEAD_OFFSET_Y[dir];
        int n3 = z + PistonConstants.HEAD_OFFSET_Z[dir];
        for (int i = 0; i < 13; ++i) {
            if (n2 <= 0 || n2 >= 127) {
                return false;
            }
            int n4 = world.getBlockId(n, n2, n3);
            if (n4 == 0) break;
            if (!PistonBlock.canMoveBlock(n4, world, n, n2, n3, true)) {
                return false;
            }
            if (Block.BLOCKS[n4].getPistonBehavior() == 1) break;
            if (i == 12) {
                return false;
            }
            n += PistonConstants.HEAD_OFFSET_X[dir];
            n2 += PistonConstants.HEAD_OFFSET_Y[dir];
            n3 += PistonConstants.HEAD_OFFSET_Z[dir];
        }
        return true;
    }

    private boolean push(World world, int x, int y, int z, int dir) {
        int n;
        int n2;
        int n3 = x + PistonConstants.HEAD_OFFSET_X[dir];
        int n4 = y + PistonConstants.HEAD_OFFSET_Y[dir];
        int n5 = z + PistonConstants.HEAD_OFFSET_Z[dir];
        for (n2 = 0; n2 < 13; ++n2) {
            if (n4 <= 0 || n4 >= 127) {
                return false;
            }
            n = world.getBlockId(n3, n4, n5);
            if (n == 0) break;
            if (!PistonBlock.canMoveBlock(n, world, n3, n4, n5, true)) {
                return false;
            }
            if (Block.BLOCKS[n].getPistonBehavior() == 1) {
                Block.BLOCKS[n].dropStacks(world, n3, n4, n5, world.getBlockMeta(n3, n4, n5));
                world.setBlock(n3, n4, n5, 0);
                break;
            }
            if (n2 == 12) {
                return false;
            }
            n3 += PistonConstants.HEAD_OFFSET_X[dir];
            n4 += PistonConstants.HEAD_OFFSET_Y[dir];
            n5 += PistonConstants.HEAD_OFFSET_Z[dir];
        }
        while (n3 != x || n4 != y || n5 != z) {
            n2 = n3 - PistonConstants.HEAD_OFFSET_X[dir];
            n = n4 - PistonConstants.HEAD_OFFSET_Y[dir];
            int n6 = n5 - PistonConstants.HEAD_OFFSET_Z[dir];
            int n7 = world.getBlockId(n2, n, n6);
            int n8 = world.getBlockMeta(n2, n, n6);
            if (n7 == this.id && n2 == x && n == y && n6 == z) {
                world.setBlockWithoutNotifyingNeighbors(n3, n4, n5, Block.MOVING_PISTON.id, dir | (this.sticky ? 8 : 0));
                world.setBlockEntity(n3, n4, n5, PistonExtensionBlock.createPistonBlockEntity(Block.PISTON_HEAD.id, dir | (this.sticky ? 8 : 0), dir, true, false));
            } else {
                world.setBlockWithoutNotifyingNeighbors(n3, n4, n5, Block.MOVING_PISTON.id, n8);
                world.setBlockEntity(n3, n4, n5, PistonExtensionBlock.createPistonBlockEntity(n7, n8, dir, true, false));
            }
            n3 = n2;
            n4 = n;
            n5 = n6;
        }
        return true;
    }
}

