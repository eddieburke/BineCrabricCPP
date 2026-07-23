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
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class FireBlock
extends Block {
    private int[] burnChances = new int[256];
    private int[] spreadChances = new int[256];

    protected FireBlock(int id, int textureId) {
        super(id, textureId, Material.FIRE);
        this.setTickRandomly(true);
    }

    public void init() {
        this.registerFlammableBlock(Block.PLANKS.id, 5, 20);
        this.registerFlammableBlock(Block.FENCE.id, 5, 20);
        this.registerFlammableBlock(Block.WOODEN_STAIRS.id, 5, 20);
        this.registerFlammableBlock(Block.LOG.id, 5, 5);
        this.registerFlammableBlock(Block.LEAVES.id, 30, 60);
        this.registerFlammableBlock(Block.BOOKSHELF.id, 30, 20);
        this.registerFlammableBlock(Block.TNT.id, 15, 100);
        this.registerFlammableBlock(Block.GRASS.id, 60, 100);
        this.registerFlammableBlock(Block.WOOL.id, 30, 60);
    }

    private void registerFlammableBlock(int block, int burnChance, int spreadChance) {
        this.burnChances[block] = burnChance;
        this.spreadChances[block] = spreadChance;
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

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 3;
    }

    public int getDroppedItemCount(Random random) {
        return 0;
    }

    public int getTickRate() {
        return 40;
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        boolean bl;
        boolean bl2 = bl = world.getBlockId(x, y - 1, z) == Block.NETHERRACK.id;
        if (!this.canPlaceAt(world, x, y, z)) {
            world.setBlock(x, y, z, 0);
        }
        if (!bl && world.isRaining() && (world.isRaining(x, y, z) || world.isRaining(x - 1, y, z) || world.isRaining(x + 1, y, z) || world.isRaining(x, y, z - 1) || world.isRaining(x, y, z + 1))) {
            world.setBlock(x, y, z, 0);
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        if (n < 15) {
            world.setBlockMetaWithoutNotifyingNeighbors(x, y, z, n + random.nextInt(3) / 2);
        }
        world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
        if (!bl && !this.areBlocksAroundFlammable(world, x, y, z)) {
            if (!world.shouldSuffocate(x, y - 1, z) || n > 3) {
                world.setBlock(x, y, z, 0);
            }
            return;
        }
        if (!bl && !this.isFlammable(world, x, y - 1, z) && n == 15 && random.nextInt(4) == 0) {
            world.setBlock(x, y, z, 0);
            return;
        }
        this.trySpreadingFire(world, x + 1, y, z, 300, random, n);
        this.trySpreadingFire(world, x - 1, y, z, 300, random, n);
        this.trySpreadingFire(world, x, y - 1, z, 250, random, n);
        this.trySpreadingFire(world, x, y + 1, z, 250, random, n);
        this.trySpreadingFire(world, x, y, z - 1, 300, random, n);
        this.trySpreadingFire(world, x, y, z + 1, 300, random, n);
        for (int i = x - 1; i <= x + 1; ++i) {
            for (int j = z - 1; j <= z + 1; ++j) {
                for (int k = y - 1; k <= y + 4; ++k) {
                    int n2;
                    int n3;
                    if (i == x && k == y && j == z) continue;
                    int n4 = 100;
                    if (k > y + 1) {
                        n4 += (k - (y + 1)) * 100;
                    }
                    if ((n3 = this.getBurnChance(world, i, k, j)) <= 0 || (n2 = (n3 + 40) / (n + 30)) <= 0 || random.nextInt(n4) > n2 || world.isRaining() && world.isRaining(i, k, j) || world.isRaining(i - 1, k, z) || world.isRaining(i + 1, k, j) || world.isRaining(i, k, j - 1) || world.isRaining(i, k, j + 1)) continue;
                    int n5 = n + random.nextInt(5) / 4;
                    if (n5 > 15) {
                        n5 = 15;
                    }
                    world.setBlock(i, k, j, this.id, n5);
                }
            }
        }
    }

    private void trySpreadingFire(World world, int x, int y, int z, int spreadFactor, Random random, int currentAge) {
        int n = this.spreadChances[world.getBlockId(x, y, z)];
        if (random.nextInt(spreadFactor) < n) {
            boolean bl;
            boolean bl2 = bl = world.getBlockId(x, y, z) == Block.TNT.id;
            if (random.nextInt(currentAge + 10) < 5 && !world.isRaining(x, y, z)) {
                int n2 = currentAge + random.nextInt(5) / 4;
                if (n2 > 15) {
                    n2 = 15;
                }
                world.setBlock(x, y, z, this.id, n2);
            } else {
                world.setBlock(x, y, z, 0);
            }
            if (bl) {
                Block.TNT.onMetadataChange(world, x, y, z, 1);
            }
        }
    }

    private boolean areBlocksAroundFlammable(World world, int x, int y, int z) {
        if (this.isFlammable(world, x + 1, y, z)) {
            return true;
        }
        if (this.isFlammable(world, x - 1, y, z)) {
            return true;
        }
        if (this.isFlammable(world, x, y - 1, z)) {
            return true;
        }
        if (this.isFlammable(world, x, y + 1, z)) {
            return true;
        }
        if (this.isFlammable(world, x, y, z - 1)) {
            return true;
        }
        return this.isFlammable(world, x, y, z + 1);
    }

    private int getBurnChance(World world, int x, int y, int z) {
        int n = 0;
        if (!world.isAir(x, y, z)) {
            return 0;
        }
        n = this.getBurnChance(world, x + 1, y, z, n);
        n = this.getBurnChance(world, x - 1, y, z, n);
        n = this.getBurnChance(world, x, y - 1, z, n);
        n = this.getBurnChance(world, x, y + 1, z, n);
        n = this.getBurnChance(world, x, y, z - 1, n);
        n = this.getBurnChance(world, x, y, z + 1, n);
        return n;
    }

    public boolean hasCollision() {
        return false;
    }

    public boolean isFlammable(BlockView blockView, int x, int y, int z) {
        return this.burnChances[blockView.getBlockId(x, y, z)] > 0;
    }

    public int getBurnChance(World world, int x, int y, int z, int currentChance) {
        int n = this.burnChances[world.getBlockId(x, y, z)];
        if (n > currentChance) {
            return n;
        }
        return currentChance;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return world.shouldSuffocate(x, y - 1, z) || this.areBlocksAroundFlammable(world, x, y, z);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (!world.shouldSuffocate(x, y - 1, z) && !this.areBlocksAroundFlammable(world, x, y, z)) {
            world.setBlock(x, y, z, 0);
            return;
        }
    }

    public void onPlaced(World world, int x, int y, int z) {
        if (world.getBlockId(x, y - 1, z) == Block.OBSIDIAN.id && Block.NETHER_PORTAL.create(world, x, y, z)) {
            return;
        }
        if (!world.shouldSuffocate(x, y - 1, z) && !this.areBlocksAroundFlammable(world, x, y, z)) {
            world.setBlock(x, y, z, 0);
            return;
        }
        world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        block12: {
            float f;
            float f2;
            float f3;
            int n;
            block11: {
                if (random.nextInt(24) == 0) {
                    world.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "fire.fire", 1.0f + random.nextFloat(), random.nextFloat() * 0.7f + 0.3f);
                }
                if (!world.shouldSuffocate(x, y - 1, z) && !Block.FIRE.isFlammable(world, x, y - 1, z)) break block11;
                for (int i = 0; i < 3; ++i) {
                    float f4 = (float)x + random.nextFloat();
                    float f5 = (float)y + random.nextFloat() * 0.5f + 0.5f;
                    float f6 = (float)z + random.nextFloat();
                    world.addParticle("largesmoke", f4, f5, f6, 0.0, 0.0, 0.0);
                }
                break block12;
            }
            if (Block.FIRE.isFlammable(world, x - 1, y, z)) {
                for (n = 0; n < 2; ++n) {
                    f3 = (float)x + random.nextFloat() * 0.1f;
                    f2 = (float)y + random.nextFloat();
                    f = (float)z + random.nextFloat();
                    world.addParticle("largesmoke", f3, f2, f, 0.0, 0.0, 0.0);
                }
            }
            if (Block.FIRE.isFlammable(world, x + 1, y, z)) {
                for (n = 0; n < 2; ++n) {
                    f3 = (float)(x + 1) - random.nextFloat() * 0.1f;
                    f2 = (float)y + random.nextFloat();
                    f = (float)z + random.nextFloat();
                    world.addParticle("largesmoke", f3, f2, f, 0.0, 0.0, 0.0);
                }
            }
            if (Block.FIRE.isFlammable(world, x, y, z - 1)) {
                for (n = 0; n < 2; ++n) {
                    f3 = (float)x + random.nextFloat();
                    f2 = (float)y + random.nextFloat();
                    f = (float)z + random.nextFloat() * 0.1f;
                    world.addParticle("largesmoke", f3, f2, f, 0.0, 0.0, 0.0);
                }
            }
            if (Block.FIRE.isFlammable(world, x, y, z + 1)) {
                for (n = 0; n < 2; ++n) {
                    f3 = (float)x + random.nextFloat();
                    f2 = (float)y + random.nextFloat();
                    f = (float)(z + 1) - random.nextFloat() * 0.1f;
                    world.addParticle("largesmoke", f3, f2, f, 0.0, 0.0, 0.0);
                }
            }
            if (!Block.FIRE.isFlammable(world, x, y + 1, z)) break block12;
            for (n = 0; n < 2; ++n) {
                f3 = (float)x + random.nextFloat();
                f2 = (float)(y + 1) - random.nextFloat() * 0.1f;
                f = (float)z + random.nextFloat();
                world.addParticle("largesmoke", f3, f2, f, 0.0, 0.0, 0.0);
            }
        }
    }
}

