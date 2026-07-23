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
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class DispenserBlock
extends BlockWithEntity {
    private Random random = new Random();

    protected DispenserBlock(int id) {
        super(id, Material.STONE);
        this.textureId = 45;
    }

    public int getTickRate() {
        return 4;
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Block.DISPENSER.id;
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
        return this.textureId + 1;
    }

    public int getTexture(int side) {
        if (side == 1) {
            return this.textureId + 17;
        }
        if (side == 0) {
            return this.textureId + 17;
        }
        if (side == 3) {
            return this.textureId + 1;
        }
        return this.textureId;
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return true;
        }
        DispenserBlockEntity dispenserBlockEntity = (DispenserBlockEntity)world.getBlockEntity(x, y, z);
        player.openDispenserScreen(dispenserBlockEntity);
        return true;
    }

    private void dispense(World world, int x, int y, int z, Random random) {
        int n = world.getBlockMeta(x, y, z);
        int n2 = 0;
        int n3 = 0;
        if (n == 3) {
            n3 = 1;
        } else if (n == 2) {
            n3 = -1;
        } else {
            n2 = n == 5 ? 1 : -1;
        }
        DispenserBlockEntity dispenserBlockEntity = (DispenserBlockEntity)world.getBlockEntity(x, y, z);
        ItemStack itemStack = dispenserBlockEntity.getItemToDispense();
        double d = (double)x + (double)n2 * 0.6 + 0.5;
        double d2 = (double)y + 0.5;
        double d3 = (double)z + (double)n3 * 0.6 + 0.5;
        if (itemStack == null) {
            world.worldEvent(1001, x, y, z, 0);
        } else {
            if (itemStack.itemId == Item.ARROW.id) {
                ArrowEntity arrowEntity = new ArrowEntity(world, d, d2, d3);
                arrowEntity.setVelocity(n2, 0.1f, n3, 1.1f, 6.0f);
                arrowEntity.pickupAllowed = true;
                world.spawnEntity(arrowEntity);
                world.worldEvent(1002, x, y, z, 0);
            } else if (itemStack.itemId == Item.EGG.id) {
                EggEntity eggEntity = new EggEntity(world, d, d2, d3);
                eggEntity.setVelocity(n2, 0.1f, n3, 1.1f, 6.0f);
                world.spawnEntity(eggEntity);
                world.worldEvent(1002, x, y, z, 0);
            } else if (itemStack.itemId == Item.SNOWBALL.id) {
                SnowballEntity snowballEntity = new SnowballEntity(world, d, d2, d3);
                snowballEntity.setVelocity(n2, 0.1f, n3, 1.1f, 6.0f);
                world.spawnEntity(snowballEntity);
                world.worldEvent(1002, x, y, z, 0);
            } else {
                ItemEntity itemEntity = new ItemEntity(world, d, d2 - 0.3, d3, itemStack);
                double d4 = random.nextDouble() * 0.1 + 0.2;
                itemEntity.velocityX = (double)n2 * d4;
                itemEntity.velocityY = 0.2f;
                itemEntity.velocityZ = (double)n3 * d4;
                itemEntity.velocityX += random.nextGaussian() * (double)0.0075f * 6.0;
                itemEntity.velocityY += random.nextGaussian() * (double)0.0075f * 6.0;
                itemEntity.velocityZ += random.nextGaussian() * (double)0.0075f * 6.0;
                world.spawnEntity(itemEntity);
                world.worldEvent(1000, x, y, z, 0);
            }
            world.worldEvent(2000, x, y, z, n2 + 1 + (n3 + 1) * 3);
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower()) {
            boolean bl;
            boolean bl2 = bl = world.isEmittingRedstonePower(x, y, z) || world.isEmittingRedstonePower(x, y + 1, z);
            if (bl) {
                world.scheduleBlockUpdate(x, y, z, this.id, this.getTickRate());
            }
        }
    }

    public void onTick(World world, int x, int y, int z, Random random) {
        if (world.isEmittingRedstonePower(x, y, z) || world.isEmittingRedstonePower(x, y + 1, z)) {
            this.dispense(world, x, y, z, random);
        }
    }

    protected BlockEntity createBlockEntity() {
        return new DispenserBlockEntity();
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
        DispenserBlockEntity dispenserBlockEntity = (DispenserBlockEntity)world.getBlockEntity(x, y, z);
        for (int i = 0; i < dispenserBlockEntity.size(); ++i) {
            ItemStack itemStack = dispenserBlockEntity.getStack(i);
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
        super.onBreak(world, x, y, z);
    }
}

