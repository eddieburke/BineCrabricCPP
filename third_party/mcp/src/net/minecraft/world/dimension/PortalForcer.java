/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.dimension;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class PortalForcer {
    private Random random = new Random();

    public void moveToPortal(World world, Entity entity) {
        if (this.teleportToValidPortal(world, entity)) {
            return;
        }
        this.createPortal(world, entity);
        this.teleportToValidPortal(world, entity);
    }

    public boolean teleportToValidPortal(World world, Entity entity) {
        double d;
        int n;
        int n2 = 128;
        double d2 = -1.0;
        int n3 = 0;
        int n4 = 0;
        int n5 = 0;
        int n6 = MathHelper.floor(entity.x);
        int n7 = MathHelper.floor(entity.z);
        for (n = n6 - n2; n <= n6 + n2; ++n) {
            double d3 = (double)n + 0.5 - entity.x;
            for (int i = n7 - n2; i <= n7 + n2; ++i) {
                double d4 = (double)i + 0.5 - entity.z;
                for (int j = 127; j >= 0; --j) {
                    if (world.getBlockId(n, j, i) != Block.NETHER_PORTAL.id) continue;
                    while (world.getBlockId(n, j - 1, i) == Block.NETHER_PORTAL.id) {
                        --j;
                    }
                    d = (double)j + 0.5 - entity.y;
                    double d5 = d3 * d3 + d * d + d4 * d4;
                    if (!(d2 < 0.0) && !(d5 < d2)) continue;
                    d2 = d5;
                    n3 = n;
                    n4 = j;
                    n5 = i;
                }
            }
        }
        if (d2 >= 0.0) {
            n = n3;
            int n8 = n4;
            int n9 = n5;
            double d6 = (double)n + 0.5;
            double d7 = (double)n8 + 0.5;
            d = (double)n9 + 0.5;
            if (world.getBlockId(n - 1, n8, n9) == Block.NETHER_PORTAL.id) {
                d6 -= 0.5;
            }
            if (world.getBlockId(n + 1, n8, n9) == Block.NETHER_PORTAL.id) {
                d6 += 0.5;
            }
            if (world.getBlockId(n, n8, n9 - 1) == Block.NETHER_PORTAL.id) {
                d -= 0.5;
            }
            if (world.getBlockId(n, n8, n9 + 1) == Block.NETHER_PORTAL.id) {
                d += 0.5;
            }
            entity.setPositionAndAnglesKeepPrevAngles(d6, d7, d, entity.yaw, 0.0f);
            entity.velocityZ = 0.0;
            entity.velocityY = 0.0;
            entity.velocityX = 0.0;
            return true;
        }
        return false;
    }

    public boolean createPortal(World world, Entity entity) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        int n7;
        int n8;
        int n9;
        double d;
        int n10;
        double d2;
        int n11;
        int n12 = 16;
        double d3 = -1.0;
        int n13 = MathHelper.floor(entity.x);
        int n14 = MathHelper.floor(entity.y);
        int n15 = MathHelper.floor(entity.z);
        int n16 = n13;
        int n17 = n14;
        int n18 = n15;
        int n19 = 0;
        int n20 = this.random.nextInt(4);
        for (n11 = n13 - n12; n11 <= n13 + n12; ++n11) {
            d2 = (double)n11 + 0.5 - entity.x;
            for (n10 = n15 - n12; n10 <= n15 + n12; ++n10) {
                d = (double)n10 + 0.5 - entity.z;
                block2: for (n9 = 127; n9 >= 0; --n9) {
                    if (!world.isAir(n11, n9, n10)) continue;
                    while (n9 > 0 && world.isAir(n11, n9 - 1, n10)) {
                        --n9;
                    }
                    for (n8 = n20; n8 < n20 + 4; ++n8) {
                        n7 = n8 % 2;
                        n6 = 1 - n7;
                        if (n8 % 4 >= 2) {
                            n7 = -n7;
                            n6 = -n6;
                        }
                        for (n5 = 0; n5 < 3; ++n5) {
                            for (n4 = 0; n4 < 4; ++n4) {
                                for (n3 = -1; n3 < 4; ++n3) {
                                    n2 = n11 + (n4 - 1) * n7 + n5 * n6;
                                    n = n9 + n3;
                                    int n21 = n10 + (n4 - 1) * n6 - n5 * n7;
                                    if (n3 < 0 && !world.getMaterial(n2, n, n21).isSolid() || n3 >= 0 && !world.isAir(n2, n, n21)) continue block2;
                                }
                            }
                        }
                        double d4 = (double)n9 + 0.5 - entity.y;
                        double d5 = d2 * d2 + d4 * d4 + d * d;
                        if (!(d3 < 0.0) && !(d5 < d3)) continue;
                        d3 = d5;
                        n16 = n11;
                        n17 = n9;
                        n18 = n10;
                        n19 = n8 % 4;
                    }
                }
            }
        }
        if (d3 < 0.0) {
            for (n11 = n13 - n12; n11 <= n13 + n12; ++n11) {
                d2 = (double)n11 + 0.5 - entity.x;
                for (n10 = n15 - n12; n10 <= n15 + n12; ++n10) {
                    d = (double)n10 + 0.5 - entity.z;
                    block10: for (n9 = 127; n9 >= 0; --n9) {
                        if (!world.isAir(n11, n9, n10)) continue;
                        while (world.isAir(n11, n9 - 1, n10)) {
                            --n9;
                        }
                        for (n8 = n20; n8 < n20 + 2; ++n8) {
                            n7 = n8 % 2;
                            n6 = 1 - n7;
                            for (int i = 0; i < 4; ++i) {
                                for (n4 = -1; n4 < 4; ++n4) {
                                    int n22 = n11 + (i - 1) * n7;
                                    n2 = n9 + n4;
                                    n = n10 + (i - 1) * n6;
                                    if (n4 < 0 && !world.getMaterial(n22, n2, n).isSolid() || n4 >= 0 && !world.isAir(n22, n2, n)) continue block10;
                                }
                            }
                            double d6 = (double)n9 + 0.5 - entity.y;
                            double d7 = d2 * d2 + d6 * d6 + d * d;
                            if (!(d3 < 0.0) && !(d7 < d3)) continue;
                            d3 = d7;
                            n16 = n11;
                            n17 = n9;
                            n18 = n10;
                            n19 = n8 % 2;
                        }
                    }
                }
            }
        }
        n11 = n19;
        int n23 = n16;
        int n24 = n17;
        n10 = n18;
        int n25 = n11 % 2;
        int n26 = 1 - n25;
        if (n11 % 4 >= 2) {
            n25 = -n25;
            n26 = -n26;
        }
        if (d3 < 0.0) {
            if (n17 < 70) {
                n17 = 70;
            }
            if (n17 > 118) {
                n17 = 118;
            }
            n24 = n17;
            for (n9 = -1; n9 <= 1; ++n9) {
                for (n8 = 1; n8 < 3; ++n8) {
                    for (n7 = -1; n7 < 3; ++n7) {
                        n6 = n23 + (n8 - 1) * n25 + n9 * n26;
                        n5 = n24 + n7;
                        n4 = n10 + (n8 - 1) * n26 - n9 * n25;
                        n3 = n7 < 0 ? 1 : 0;
                        world.setBlock(n6, n5, n4, n3 != 0 ? Block.OBSIDIAN.id : 0);
                    }
                }
            }
        }
        for (n9 = 0; n9 < 4; ++n9) {
            world.pauseTicking = true;
            for (n8 = 0; n8 < 4; ++n8) {
                for (n7 = -1; n7 < 4; ++n7) {
                    n6 = n23 + (n8 - 1) * n25;
                    n5 = n24 + n7;
                    n4 = n10 + (n8 - 1) * n26;
                    n3 = n8 == 0 || n8 == 3 || n7 == -1 || n7 == 3 ? 1 : 0;
                    world.setBlock(n6, n5, n4, n3 != 0 ? Block.OBSIDIAN.id : Block.NETHER_PORTAL.id);
                }
            }
            world.pauseTicking = false;
            for (n8 = 0; n8 < 4; ++n8) {
                for (n7 = -1; n7 < 4; ++n7) {
                    n6 = n23 + (n8 - 1) * n25;
                    n5 = n24 + n7;
                    n4 = n10 + (n8 - 1) * n26;
                    world.notifyNeighbors(n6, n5, n4, world.getBlockId(n6, n5, n4));
                }
            }
        }
        return true;
    }
}

