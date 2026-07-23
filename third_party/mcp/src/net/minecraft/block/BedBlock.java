/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.SleepAttemptResult;
import net.minecraft.item.Item;
import net.minecraft.util.math.Facings;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class BedBlock
extends Block {
    public static final int[][] BED_OFFSETS = new int[][]{{0, 1}, {-1, 0}, {0, -1}, {1, 0}};

    public BedBlock(int id) {
        super(id, 134, Material.WOOL);
        this.setDefaultShape();
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        SleepAttemptResult sleepAttemptResult;
        if (world.isRemote) {
            return true;
        }
        int n = world.getBlockMeta(x, y, z);
        if (!BedBlock.isHeadOfBed(n)) {
            int n2 = BedBlock.getDirection(n);
            if (world.getBlockId(x += BED_OFFSETS[n2][0], y, z += BED_OFFSETS[n2][1]) != this.id) {
                return true;
            }
            n = world.getBlockMeta(x, y, z);
        }
        if (!world.dimension.hasWorldSpawn()) {
            double d = (double)x + 0.5;
            double d2 = (double)y + 0.5;
            double d3 = (double)z + 0.5;
            world.setBlock(x, y, z, 0);
            int n3 = BedBlock.getDirection(n);
            if (world.getBlockId(x += BED_OFFSETS[n3][0], y, z += BED_OFFSETS[n3][1]) == this.id) {
                world.setBlock(x, y, z, 0);
                d = (d + (double)x + 0.5) / 2.0;
                d2 = (d2 + (double)y + 0.5) / 2.0;
                d3 = (d3 + (double)z + 0.5) / 2.0;
            }
            world.createExplosion(null, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 5.0f, true);
            return true;
        }
        if (BedBlock.isOccupied(n)) {
            PlayerEntity playerEntity = null;
            for (PlayerEntity playerEntity2 : world.players) {
                if (!playerEntity2.isSleeping()) continue;
                Vec3i vec3i = playerEntity2.sleepingPos;
                if (vec3i.x != x || vec3i.y != y || vec3i.z != z) continue;
                playerEntity = playerEntity2;
            }
            if (playerEntity == null) {
                BedBlock.updateState(world, x, y, z, false);
            } else {
                player.sendMessage("tile.bed.occupied");
                return true;
            }
        }
        if ((sleepAttemptResult = player.trySleep(x, y, z)) == SleepAttemptResult.OK) {
            BedBlock.updateState(world, x, y, z, true);
            return true;
        }
        if (sleepAttemptResult == SleepAttemptResult.NOT_POSSIBLE_NOW) {
            player.sendMessage("tile.bed.noSleep");
        }
        return true;
    }

    public int getTexture(int side, int meta) {
        if (side == 0) {
            return Block.PLANKS.textureId;
        }
        int n = BedBlock.getDirection(meta);
        int n2 = Facings.BED_FACINGS[n][side];
        if (BedBlock.isHeadOfBed(meta)) {
            if (n2 == 2) {
                return this.textureId + 2 + 16;
            }
            if (n2 == 5 || n2 == 4) {
                return this.textureId + 1 + 16;
            }
            return this.textureId + 1;
        }
        if (n2 == 3) {
            return this.textureId - 1 + 16;
        }
        if (n2 == 5 || n2 == 4) {
            return this.textureId + 16;
        }
        return this.textureId;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 14;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        this.setDefaultShape();
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = BedBlock.getDirection(n);
        if (BedBlock.isHeadOfBed(n)) {
            if (world.getBlockId(x - BED_OFFSETS[n2][0], y, z - BED_OFFSETS[n2][1]) != this.id) {
                world.setBlock(x, y, z, 0);
            }
        } else if (world.getBlockId(x + BED_OFFSETS[n2][0], y, z + BED_OFFSETS[n2][1]) != this.id) {
            world.setBlock(x, y, z, 0);
            if (!world.isRemote) {
                this.dropStacks(world, x, y, z, n);
            }
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        if (BedBlock.isHeadOfBed(blockMeta)) {
            return 0;
        }
        return Item.BED.id;
    }

    private void setDefaultShape() {
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f);
    }

    public static int getDirection(int meta) {
        return meta & 3;
    }

    public static boolean isHeadOfBed(int meta) {
        return (meta & 8) != 0;
    }

    public static boolean isOccupied(int meta) {
        return (meta & 4) != 0;
    }

    public static void updateState(World world, int x, int y, int z, boolean occupied) {
        int n = world.getBlockMeta(x, y, z);
        n = occupied ? (n |= 4) : (n &= 0xFFFFFFFB);
        world.setBlockMeta(x, y, z, n);
    }

    public static Vec3i findWakeUpPosition(World world, int x, int y, int z, int skip) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = BedBlock.getDirection(n);
        for (int i = 0; i <= 1; ++i) {
            int n3 = x - BED_OFFSETS[n2][0] * i - 1;
            int n4 = z - BED_OFFSETS[n2][1] * i - 1;
            int n5 = n3 + 2;
            int n6 = n4 + 2;
            for (int j = n3; j <= n5; ++j) {
                for (int k = n4; k <= n6; ++k) {
                    if (!world.shouldSuffocate(j, y - 1, k) || !world.isAir(j, y, k) || !world.isAir(j, y + 1, k)) continue;
                    if (skip > 0) {
                        --skip;
                        continue;
                    }
                    return new Vec3i(j, y, k);
                }
            }
        }
        return null;
    }

    public void dropStacks(World world, int x, int y, int z, int meta, float luck) {
        if (!BedBlock.isHeadOfBed(meta)) {
            super.dropStacks(world, x, y, z, meta, luck);
        }
    }

    public int getPistonBehavior() {
        return 1;
    }
}

