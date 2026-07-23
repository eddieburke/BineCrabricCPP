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
import net.minecraft.block.TranslucentBlock;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class NetherPortalBlock
extends TranslucentBlock {
    public NetherPortalBlock(int id, int textureId) {
        super(id, textureId, Material.NETHER_PORTAL, false);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        if (blockView.getBlockId(x - 1, y, z) == this.id || blockView.getBlockId(x + 1, y, z) == this.id) {
            float f = 0.5f;
            float f2 = 0.125f;
            this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f2, 0.5f + f, 1.0f, 0.5f + f2);
        } else {
            float f = 0.125f;
            float f3 = 0.5f;
            this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f3, 0.5f + f, 1.0f, 0.5f + f3);
        }
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean create(World world, int x, int y, int z) {
        int n;
        int n2;
        int n3 = 0;
        int n4 = 0;
        if (world.getBlockId(x - 1, y, z) == Block.OBSIDIAN.id || world.getBlockId(x + 1, y, z) == Block.OBSIDIAN.id) {
            n3 = 1;
        }
        if (world.getBlockId(x, y, z - 1) == Block.OBSIDIAN.id || world.getBlockId(x, y, z + 1) == Block.OBSIDIAN.id) {
            n4 = 1;
        }
        if (n3 == n4) {
            return false;
        }
        if (world.getBlockId(x - n3, y, z - n4) == 0) {
            x -= n3;
            z -= n4;
        }
        for (n2 = -1; n2 <= 2; ++n2) {
            for (n = -1; n <= 3; ++n) {
                boolean bl;
                boolean bl2 = bl = n2 == -1 || n2 == 2 || n == -1 || n == 3;
                if (!(n2 != -1 && n2 != 2 || n != -1 && n != 3)) continue;
                int n5 = world.getBlockId(x + n3 * n2, y + n, z + n4 * n2);
                if (!(bl ? n5 != Block.OBSIDIAN.id : n5 != 0 && n5 != Block.FIRE.id)) continue;
                return false;
            }
        }
        world.pauseTicking = true;
        for (n2 = 0; n2 < 2; ++n2) {
            for (n = 0; n < 3; ++n) {
                world.setBlock(x + n3 * n2, y + n, z + n4 * n2, Block.NETHER_PORTAL.id);
            }
        }
        world.pauseTicking = false;
        return true;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        boolean bl;
        int n;
        int n2 = 0;
        int n3 = 1;
        if (world.getBlockId(x - 1, y, z) == this.id || world.getBlockId(x + 1, y, z) == this.id) {
            n2 = 1;
            n3 = 0;
        }
        int n4 = y;
        while (world.getBlockId(x, n4 - 1, z) == this.id) {
            --n4;
        }
        if (world.getBlockId(x, n4 - 1, z) != Block.OBSIDIAN.id) {
            world.setBlock(x, y, z, 0);
            return;
        }
        for (n = 1; n < 4 && world.getBlockId(x, n4 + n, z) == this.id; ++n) {
        }
        if (n != 3 || world.getBlockId(x, n4 + n, z) != Block.OBSIDIAN.id) {
            world.setBlock(x, y, z, 0);
            return;
        }
        boolean bl2 = world.getBlockId(x - 1, y, z) == this.id || world.getBlockId(x + 1, y, z) == this.id;
        boolean bl3 = bl = world.getBlockId(x, y, z - 1) == this.id || world.getBlockId(x, y, z + 1) == this.id;
        if (bl2 && bl) {
            world.setBlock(x, y, z, 0);
            return;
        }
        if (!(world.getBlockId(x + n2, y, z + n3) == Block.OBSIDIAN.id && world.getBlockId(x - n2, y, z - n3) == this.id || world.getBlockId(x - n2, y, z - n3) == Block.OBSIDIAN.id && world.getBlockId(x + n2, y, z + n3) == this.id)) {
            world.setBlock(x, y, z, 0);
            return;
        }
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        boolean bl;
        if (blockView.getBlockId(x, y, z) == this.id) {
            return false;
        }
        boolean bl2 = blockView.getBlockId(x - 1, y, z) == this.id && blockView.getBlockId(x - 2, y, z) != this.id;
        boolean bl3 = blockView.getBlockId(x + 1, y, z) == this.id && blockView.getBlockId(x + 2, y, z) != this.id;
        boolean bl4 = blockView.getBlockId(x, y, z - 1) == this.id && blockView.getBlockId(x, y, z - 2) != this.id;
        boolean bl5 = blockView.getBlockId(x, y, z + 1) == this.id && blockView.getBlockId(x, y, z + 2) != this.id;
        boolean bl6 = bl2 || bl3;
        boolean bl7 = bl = bl4 || bl5;
        if (bl6 && side == 4) {
            return true;
        }
        if (bl6 && side == 5) {
            return true;
        }
        if (bl && side == 2) {
            return true;
        }
        return bl && side == 3;
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return 1;
    }

    public void onEntityCollision(World world, int x, int y, int z, Entity entity) {
        if (entity.vehicle == null && entity.passenger == null) {
            entity.tickPortalCooldown();
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        if (random.nextInt(100) == 0) {
            world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "portal.portal", 1.0f, random.nextFloat() * 0.4f + 0.8f);
        }
        for (int i = 0; i < 4; ++i) {
            double d = (float)x + random.nextFloat();
            double d2 = (float)y + random.nextFloat();
            double d3 = (float)z + random.nextFloat();
            double d4 = 0.0;
            double d5 = 0.0;
            double d6 = 0.0;
            int n = random.nextInt(2) * 2 - 1;
            d4 = ((double)random.nextFloat() - 0.5) * 0.5;
            d5 = ((double)random.nextFloat() - 0.5) * 0.5;
            d6 = ((double)random.nextFloat() - 0.5) * 0.5;
            if (world.getBlockId(x - 1, y, z) == this.id || world.getBlockId(x + 1, y, z) == this.id) {
                d3 = (double)z + 0.5 + 0.25 * (double)n;
                d6 = random.nextFloat() * 2.0f * (float)n;
            } else {
                d = (double)x + 0.5 + 0.25 * (double)n;
                d4 = random.nextFloat() * 2.0f * (float)n;
            }
            world.addParticle("portal", d, d2, d3, d4, d5, d6);
        }
    }
}

