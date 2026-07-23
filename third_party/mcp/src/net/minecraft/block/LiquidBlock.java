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
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public abstract class LiquidBlock
extends Block {
    protected LiquidBlock(int i, Material arg) {
        super(i, (arg == Material.LAVA ? 14 : 12) * 16 + 13, arg);
        float f = 0.0f;
        float f2 = 0.0f;
        this.setBoundingBox(0.0f + f2, 0.0f + f, 0.0f + f2, 1.0f + f2, 1.0f + f, 1.0f + f2);
        this.setTickRandomly(true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        return 0xFFFFFF;
    }

    public static float getFluidHeightFromMeta(int meta) {
        if (meta >= 8) {
            meta = 0;
        }
        float f = (float)(meta + 1) / 9.0f;
        return f;
    }

    public int getTexture(int side) {
        if (side == 0 || side == 1) {
            return this.textureId;
        }
        return this.textureId + 1;
    }

    protected int getLiquidState(World world, int x, int y, int z) {
        if (world.getMaterial(x, y, z) != this.material) {
            return -1;
        }
        return world.getBlockMeta(x, y, z);
    }

    protected int getLiquidDepth(BlockView blockView, int x, int y, int z) {
        if (blockView.getMaterial(x, y, z) != this.material) {
            return -1;
        }
        int n = blockView.getBlockMeta(x, y, z);
        if (n >= 8) {
            n = 0;
        }
        return n;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean hasCollision(int meta, boolean allowLiquids) {
        return allowLiquids && meta == 0;
    }

    public boolean isSolidFace(BlockView blockView, int x, int y, int z, int face) {
        Material material = blockView.getMaterial(x, y, z);
        if (material == this.material) {
            return false;
        }
        if (material == Material.ICE) {
            return false;
        }
        if (face == 1) {
            return true;
        }
        return super.isSolidFace(blockView, x, y, z, face);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isSideVisible(BlockView blockView, int x, int y, int z, int side) {
        Material material = blockView.getMaterial(x, y, z);
        if (material == this.material) {
            return false;
        }
        if (material == Material.ICE) {
            return false;
        }
        if (side == 1) {
            return true;
        }
        return super.isSideVisible(blockView, x, y, z, side);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 4;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return 0;
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    private Vec3d getFlow(BlockView world, int x, int y, int z) {
        int n;
        Vec3d vec3d = Vec3d.createCached(0.0, 0.0, 0.0);
        int n2 = this.getLiquidDepth(world, x, y, z);
        for (n = 0; n < 4; ++n) {
            int n3;
            int n4;
            int n5 = x;
            int n6 = y;
            int n7 = z;
            if (n == 0) {
                --n5;
            }
            if (n == 1) {
                --n7;
            }
            if (n == 2) {
                ++n5;
            }
            if (n == 3) {
                ++n7;
            }
            if ((n4 = this.getLiquidDepth(world, n5, n6, n7)) < 0) {
                if (world.getMaterial(n5, n6, n7).blocksMovement() || (n4 = this.getLiquidDepth(world, n5, n6 - 1, n7)) < 0) continue;
                n3 = n4 - (n2 - 8);
                vec3d = vec3d.add((n5 - x) * n3, (n6 - y) * n3, (n7 - z) * n3);
                continue;
            }
            if (n4 < 0) continue;
            n3 = n4 - n2;
            vec3d = vec3d.add((n5 - x) * n3, (n6 - y) * n3, (n7 - z) * n3);
        }
        if (world.getBlockMeta(x, y, z) >= 8) {
            n = 0;
            if (n != 0 || this.isSolidFace(world, x, y, z - 1, 2)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x, y, z + 1, 3)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x - 1, y, z, 4)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x + 1, y, z, 5)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x, y + 1, z - 1, 2)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x, y + 1, z + 1, 3)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x - 1, y + 1, z, 4)) {
                n = 1;
            }
            if (n != 0 || this.isSolidFace(world, x + 1, y + 1, z, 5)) {
                n = 1;
            }
            if (n != 0) {
                vec3d = vec3d.normalize().add(0.0, -6.0, 0.0);
            }
        }
        vec3d = vec3d.normalize();
        return vec3d;
    }

    public void applyVelocity(World world, int x, int y, int z, Entity entity, Vec3d velocity) {
        Vec3d vec3d = this.getFlow(world, x, y, z);
        velocity.x += vec3d.x;
        velocity.y += vec3d.y;
        velocity.z += vec3d.z;
    }

    public int getTickRate() {
        if (this.material == Material.WATER) {
            return 5;
        }
        if (this.material == Material.LAVA) {
            return 30;
        }
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    public float getLuminance(BlockView blockView, int x, int y, int z) {
        float f;
        float f2 = blockView.getLightBrightness(x, y, z);
        return f2 > (f = blockView.getLightBrightness(x, y + 1, z)) ? f2 : f;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        super.onTick(world, x, y, z, random);
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderLayer() {
        return this.material == Material.WATER ? 1 : 0;
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        int n;
        if (this.material == Material.WATER && random.nextInt(64) == 0 && (n = world.getBlockMeta(x, y, z)) > 0 && n < 8) {
            world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "liquid.water", random.nextFloat() * 0.25f + 0.75f, random.nextFloat() * 1.0f + 0.5f);
        }
        if (this.material == Material.LAVA && world.getMaterial(x, y + 1, z) == Material.AIR && !world.isBlockOpaqueCube(x, y + 1, z) && random.nextInt(100) == 0) {
            double d = (float)x + random.nextFloat();
            double d2 = (double)y + this.maxY;
            double d3 = (float)z + random.nextFloat();
            world.addParticle("lava", d, d2, d3, 0.0, 0.0, 0.0);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public static double getFlowingAngle(BlockView view, int x, int y, int z, Material material) {
        Vec3d vec3d = null;
        if (material == Material.WATER) {
            vec3d = ((LiquidBlock)Block.FLOWING_WATER).getFlow(view, x, y, z);
        }
        if (material == Material.LAVA) {
            vec3d = ((LiquidBlock)Block.FLOWING_LAVA).getFlow(view, x, y, z);
        }
        if (vec3d.x == 0.0 && vec3d.z == 0.0) {
            return -1000.0;
        }
        return Math.atan2(vec3d.z, vec3d.x) - 1.5707963267948966;
    }

    public void onPlaced(World world, int x, int y, int z) {
        this.checkBlockCollisions(world, x, y, z);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        this.checkBlockCollisions(world, x, y, z);
    }

    private void checkBlockCollisions(World world, int x, int y, int z) {
        if (world.getBlockId(x, y, z) != this.id) {
            return;
        }
        if (this.material == Material.LAVA) {
            boolean bl = false;
            if (bl || world.getMaterial(x, y, z - 1) == Material.WATER) {
                bl = true;
            }
            if (bl || world.getMaterial(x, y, z + 1) == Material.WATER) {
                bl = true;
            }
            if (bl || world.getMaterial(x - 1, y, z) == Material.WATER) {
                bl = true;
            }
            if (bl || world.getMaterial(x + 1, y, z) == Material.WATER) {
                bl = true;
            }
            if (bl || world.getMaterial(x, y + 1, z) == Material.WATER) {
                bl = true;
            }
            if (bl) {
                int n = world.getBlockMeta(x, y, z);
                if (n == 0) {
                    world.setBlock(x, y, z, Block.OBSIDIAN.id);
                } else if (n <= 4) {
                    world.setBlock(x, y, z, Block.COBBLESTONE.id);
                }
                this.fizz(world, x, y, z);
            }
        }
    }

    protected void fizz(World world, int x, int y, int z) {
        world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "random.fizz", 0.5f, 2.6f + (world.random.nextFloat() - world.random.nextFloat()) * 0.8f);
        for (int i = 0; i < 8; ++i) {
            world.addParticle("largesmoke", (double)x + Math.random(), (double)y + 1.2, (double)z + Math.random(), 0.0, 0.0, 0.0);
        }
    }
}

