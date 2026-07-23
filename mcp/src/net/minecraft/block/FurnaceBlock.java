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
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class FurnaceBlock
extends BlockWithEntity {
    private Random random = new Random();
    private final boolean lit;
    private static boolean ignoreBlockRemoval = false;

    protected FurnaceBlock(int id, boolean lit) {
        super(id, Material.STONE);
        this.lit = lit;
        this.textureId = 45;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.FURNACE.id;
    }

    public void onPlaced(World world, int x, int y, int z) {
        super.onPlaced(world, x, y, z);
        this.updateDirection(world, x, y, z);
    }

    private void updateDirection(World world, int x, int y, int z) {
        if (world.isRemote) {
            return;
        }
        int n = world.getBlockId(x, y, z - 1);
        int n2 = world.getBlockId(x, y, z + 1);
        int n3 = world.getBlockId(x - 1, y, z);
        int n4 = world.getBlockId(x + 1, y, z);
        int n5 = 3;
        if (Block.BLOCKS_OPAQUE[n] && !Block.BLOCKS_OPAQUE[n2]) {
            n5 = 3;
        }
        if (Block.BLOCKS_OPAQUE[n2] && !Block.BLOCKS_OPAQUE[n]) {
            n5 = 2;
        }
        if (Block.BLOCKS_OPAQUE[n3] && !Block.BLOCKS_OPAQUE[n4]) {
            n5 = 5;
        }
        if (Block.BLOCKS_OPAQUE[n4] && !Block.BLOCKS_OPAQUE[n3]) {
            n5 = 4;
        }
        world.setBlockMeta(x, y, z, n5);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(BlockView blockView, int x, int y, int z, int side) {
        if (side == 1) {
            return this.textureId + 17;
        }
        if (side == 0) {
            return this.textureId + 17;
        }
        int n = blockView.getBlockMeta(x, y, z);
        if (side != n) {
            return this.textureId;
        }
        if (this.lit) {
            return this.textureId + 16;
        }
        return this.textureId - 1;
    }

    @Environment(value=EnvType.CLIENT)
    public void randomDisplayTick(World world, int x, int y, int z, Random random) {
        if (!this.lit) {
            return;
        }
        int n = world.getBlockMeta(x, y, z);
        float f = (float)x + 0.5f;
        float f2 = (float)y + 0.0f + random.nextFloat() * 6.0f / 16.0f;
        float f3 = (float)z + 0.5f;
        float f4 = 0.52f;
        float f5 = random.nextFloat() * 0.6f - 0.3f;
        if (n == 4) {
            world.addParticle("smoke", f - f4, f2, f3 + f5, 0.0, 0.0, 0.0);
            world.addParticle("flame", f - f4, f2, f3 + f5, 0.0, 0.0, 0.0);
        } else if (n == 5) {
            world.addParticle("smoke", f + f4, f2, f3 + f5, 0.0, 0.0, 0.0);
            world.addParticle("flame", f + f4, f2, f3 + f5, 0.0, 0.0, 0.0);
        } else if (n == 2) {
            world.addParticle("smoke", f + f5, f2, f3 - f4, 0.0, 0.0, 0.0);
            world.addParticle("flame", f + f5, f2, f3 - f4, 0.0, 0.0, 0.0);
        } else if (n == 3) {
            world.addParticle("smoke", f + f5, f2, f3 + f4, 0.0, 0.0, 0.0);
            world.addParticle("flame", f + f5, f2, f3 + f4, 0.0, 0.0, 0.0);
        }
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId + 17;
        }
        if (side == 0) {
            return this.textureId + 17;
        }
        if (side == 3) {
            return this.textureId - 1;
        }
        return this.textureId;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return true;
        }
        FurnaceBlockEntity furnaceBlockEntity = (FurnaceBlockEntity)world.getBlockEntity(x, y, z);
        player.openFurnaceScreen(furnaceBlockEntity);
        return true;
    }

    public static void updateLitState(boolean lit, World world, int x, int y, int z) {
        int n = world.getBlockMeta(x, y, z);
        BlockEntity blockEntity = world.getBlockEntity(x, y, z);
        ignoreBlockRemoval = true;
        if (lit) {
            world.setBlock(x, y, z, Block.LIT_FURNACE.id);
        } else {
            world.setBlock(x, y, z, Block.FURNACE.id);
        }
        ignoreBlockRemoval = false;
        world.setBlockMeta(x, y, z, n);
        blockEntity.cancelRemoval();
        world.setBlockEntity(x, y, z, blockEntity);
    }

    protected BlockEntity createBlockEntity() {
        return new FurnaceBlockEntity();
    }

    public void onPlaced(World world, int x, int y, int z, LivingEntity placer) {
        int n = MathHelper.floor((double)(placer.yaw * 4.0f / 360.0f) + 0.5) & 3;
        if (n == 0) {
            world.setBlockMeta(x, y, z, 2);
        }
        if (n == 1) {
            world.setBlockMeta(x, y, z, 5);
        }
        if (n == 2) {
            world.setBlockMeta(x, y, z, 3);
        }
        if (n == 3) {
            world.setBlockMeta(x, y, z, 4);
        }
    }

    public void onBreak(World world, int x, int y, int z) {
        if (!ignoreBlockRemoval) {
            FurnaceBlockEntity furnaceBlockEntity = (FurnaceBlockEntity)world.getBlockEntity(x, y, z);
            for (int i = 0; i < furnaceBlockEntity.size(); ++i) {
                ItemStack itemStack = furnaceBlockEntity.getStack(i);
                if (itemStack == null) continue;
                float f = this.random.nextFloat() * 0.8f + 0.1f;
                float f2 = this.random.nextFloat() * 0.8f + 0.1f;
                float f3 = this.random.nextFloat() * 0.8f + 0.1f;
                while (itemStack.count > 0) {
                    int n = this.random.nextInt(21) + 10;
                    if (n > itemStack.count) {
                        n = itemStack.count;
                    }
                    itemStack.count -= n;
                    ItemEntity itemEntity = new ItemEntity(world, (float)x + f, (float)y + f2, (float)z + f3, new ItemStack(itemStack.itemId, n, itemStack.getDamage()));
                    float f4 = 0.05f;
                    itemEntity.velocityX = (float)this.random.nextGaussian() * f4;
                    itemEntity.velocityY = (float)this.random.nextGaussian() * f4 + 0.2f;
                    itemEntity.velocityZ = (float)this.random.nextGaussian() * f4;
                    world.spawnEntity(itemEntity);
                }
            }
        }
        super.onBreak(world, x, y, z);
    }
}

