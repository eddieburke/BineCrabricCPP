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
import net.minecraft.block.TransparentBlock;
import net.minecraft.block.material.Material;
import net.minecraft.client.color.world.FoliageColors;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.stat.Stats;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class LeavesBlock
extends TransparentBlock {
    private int spriteIndex;
    int[] decayRegion;

    protected LeavesBlock(int id, int textureId) {
        super(id, textureId, Material.LEAVES, false);
        this.spriteIndex = textureId;
        this.setTickRandomly(true);
    }

    @Environment(value=EnvType.CLIENT)
    public int getColor(int meta) {
        if ((meta & 1) == 1) {
            return FoliageColors.getSpruceColor();
        }
        if ((meta & 2) == 2) {
            return FoliageColors.getBirchColor();
        }
        return FoliageColors.getDefaultColor();
    }

    @Environment(value=EnvType.CLIENT)
    public int getColorMultiplier(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        if ((n & 1) == 1) {
            return FoliageColors.getSpruceColor();
        }
        if ((n & 2) == 2) {
            return FoliageColors.getBirchColor();
        }
        blockView.getBiomeSource().getBiomesInArea(x, z, 1, 1);
        double d = blockView.getBiomeSource().temperatureMap[0];
        double d2 = blockView.getBiomeSource().downfallMap[0];
        return FoliageColors.getColor(d, d2);
    }

    public void onBreak(World world, int x, int y, int z) {
        int n = 1;
        int n2 = n + 1;
        if (world.isRegionLoaded(x - n2, y - n2, z - n2, x + n2, y + n2, z + n2)) {
            for (int i = -n; i <= n; ++i) {
                for (int j = -n; j <= n; ++j) {
                    for (int k = -n; k <= n; ++k) {
                        int n3 = world.getBlockId(x + i, y + j, z + k);
                        if (n3 != Block.LEAVES.id) continue;
                        int n4 = world.getBlockMeta(x + i, y + j, z + k);
                        world.setBlockMetaWithoutNotifyingNeighbors(x + i, y + j, z + k, n4 | 8);
                    }
                }
            }
        }
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) != 0) {
            int n2;
            int n3 = 4;
            int n4 = n3 + 1;
            int n5 = 32;
            int n6 = n5 * n5;
            int n7 = n5 / 2;
            if (this.decayRegion == null) {
                this.decayRegion = new int[n5 * n5 * n5];
            }
            if (world.isRegionLoaded(x - n4, y - n4, z - n4, x + n4, y + n4, z + n4)) {
                int n8;
                int n9;
                int n10;
                for (n2 = -n3; n2 <= n3; ++n2) {
                    for (n10 = -n3; n10 <= n3; ++n10) {
                        for (n9 = -n3; n9 <= n3; ++n9) {
                            n8 = world.getBlockId(x + n2, y + n10, z + n9);
                            this.decayRegion[(n2 + n7) * n6 + (n10 + n7) * n5 + (n9 + n7)] = n8 == Block.LOG.id ? 0 : (n8 == Block.LEAVES.id ? -2 : -1);
                        }
                    }
                }
                for (n2 = 1; n2 <= 4; ++n2) {
                    for (n10 = -n3; n10 <= n3; ++n10) {
                        for (n9 = -n3; n9 <= n3; ++n9) {
                            for (n8 = -n3; n8 <= n3; ++n8) {
                                if (this.decayRegion[(n10 + n7) * n6 + (n9 + n7) * n5 + (n8 + n7)] != n2 - 1) continue;
                                if (this.decayRegion[(n10 + n7 - 1) * n6 + (n9 + n7) * n5 + (n8 + n7)] == -2) {
                                    this.decayRegion[(n10 + n7 - 1) * n6 + (n9 + n7) * n5 + (n8 + n7)] = n2;
                                }
                                if (this.decayRegion[(n10 + n7 + 1) * n6 + (n9 + n7) * n5 + (n8 + n7)] == -2) {
                                    this.decayRegion[(n10 + n7 + 1) * n6 + (n9 + n7) * n5 + (n8 + n7)] = n2;
                                }
                                if (this.decayRegion[(n10 + n7) * n6 + (n9 + n7 - 1) * n5 + (n8 + n7)] == -2) {
                                    this.decayRegion[(n10 + n7) * n6 + (n9 + n7 - 1) * n5 + (n8 + n7)] = n2;
                                }
                                if (this.decayRegion[(n10 + n7) * n6 + (n9 + n7 + 1) * n5 + (n8 + n7)] == -2) {
                                    this.decayRegion[(n10 + n7) * n6 + (n9 + n7 + 1) * n5 + (n8 + n7)] = n2;
                                }
                                if (this.decayRegion[(n10 + n7) * n6 + (n9 + n7) * n5 + (n8 + n7 - 1)] == -2) {
                                    this.decayRegion[(n10 + n7) * n6 + (n9 + n7) * n5 + (n8 + n7 - 1)] = n2;
                                }
                                if (this.decayRegion[(n10 + n7) * n6 + (n9 + n7) * n5 + (n8 + n7 + 1)] != -2) continue;
                                this.decayRegion[(n10 + n7) * n6 + (n9 + n7) * n5 + (n8 + n7 + 1)] = n2;
                            }
                        }
                    }
                }
            }
            if ((n2 = this.decayRegion[n7 * n6 + n7 * n5 + n7]) >= 0) {
                world.setBlockMetaWithoutNotifyingNeighbors(x, y, z, n & 0xFFFFFFF7);
            } else {
                this.breakLeaves(world, x, y, z);
            }
        }
    }

    private void breakLeaves(World world, int x, int y, int z) {
        this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
        world.setBlock(x, y, z, 0);
    }

    public int getDroppedItemCount(Random random) {
        return random.nextInt(20) == 0 ? 1 : 0;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.SAPLING.id;
    }

    public void afterBreak(World world, PlayerEntity playerEntity, int x, int y, int z, int meta) {
        if (!world.isRemote && playerEntity.getHand() != null && playerEntity.getHand().itemId == Item.SHEARS.id) {
            playerEntity.increaseStat(Stats.MINE_BLOCK[this.id], 1);
            this.dropStack(world, x, y, z, new ItemStack(Block.LEAVES.id, 1, meta & 3));
        } else {
            super.afterBreak(world, playerEntity, x, y, z, meta);
        }
    }

    protected int getDroppedItemMeta(int blockMeta) {
        return blockMeta & 3;
    }

    public boolean isOpaque() {
        return !this.renderSides;
    }

    public int getTexture(int side, int meta) {
        if ((meta & 3) == 1) {
            return this.textureId + 80;
        }
        return this.textureId;
    }

    @Environment(value=EnvType.CLIENT)
    public void setFancyGraphics(boolean bl) {
        this.renderSides = bl;
        this.textureId = this.spriteIndex + (bl ? 0 : 1);
    }

    public void onSteppedOn(World world, int x, int y, int z, Entity entity) {
        super.onSteppedOn(world, x, y, z, entity);
    }
}

