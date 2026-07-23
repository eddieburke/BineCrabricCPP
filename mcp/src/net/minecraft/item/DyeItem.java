/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.CropBlock;
import net.minecraft.block.SaplingBlock;
import net.minecraft.block.WoolBlock;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

public class DyeItem
extends Item {
    public static final String[] names = new String[]{"black", "red", "green", "brown", "blue", "purple", "cyan", "silver", "gray", "pink", "lime", "yellow", "lightBlue", "magenta", "orange", "white"};
    public static final int[] colors = new int[]{0x1E1B1B, 11743532, 3887386, 5320730, 2437522, 8073150, 2651799, 2651799, 0x434343, 14188952, 4312372, 14602026, 6719955, 12801229, 15435844, 0xF0F0F0};

    public DyeItem(int i) {
        super(i);
        this.setHasSubtypes(true);
        this.setMaxDamage(0);
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId(int damage) {
        int n = damage;
        return this.textureId + n % 8 * 16 + n / 8;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey(ItemStack stack) {
        return super.getTranslationKey() + "." + names[stack.getDamage()];
    }

    public boolean useOnBlock(ItemStack stack, PlayerEntity user, World world, int x, int y, int z, int side) {
        if (stack.getDamage() == 15) {
            int n = world.getBlockId(x, y, z);
            if (n == Block.SAPLING.id) {
                if (!world.isRemote) {
                    ((SaplingBlock)Block.SAPLING).generate(world, x, y, z, world.random);
                    --stack.count;
                }
                return true;
            }
            if (n == Block.WHEAT.id) {
                if (!world.isRemote) {
                    ((CropBlock)Block.WHEAT).applyFullGrowth(world, x, y, z);
                    --stack.count;
                }
                return true;
            }
            if (n == Block.GRASS_BLOCK.id) {
                if (!world.isRemote) {
                    --stack.count;
                    block0: for (int i = 0; i < 128; ++i) {
                        int n2 = x;
                        int n3 = y + 1;
                        int n4 = z;
                        for (int j = 0; j < i / 16; ++j) {
                            if (world.getBlockId(n2 += random.nextInt(3) - 1, (n3 += (random.nextInt(3) - 1) * random.nextInt(3) / 2) - 1, n4 += random.nextInt(3) - 1) != Block.GRASS_BLOCK.id || world.shouldSuffocate(n2, n3, n4)) continue block0;
                        }
                        if (world.getBlockId(n2, n3, n4) != 0) continue;
                        if (random.nextInt(10) != 0) {
                            world.setBlock(n2, n3, n4, Block.GRASS.id, 1);
                            continue;
                        }
                        if (random.nextInt(3) != 0) {
                            world.setBlock(n2, n3, n4, Block.DANDELION.id);
                            continue;
                        }
                        world.setBlock(n2, n3, n4, Block.ROSE.id);
                    }
                }
                return true;
            }
        }
        return false;
    }

    public void useOnEntity(ItemStack stack, LivingEntity entity) {
        if (entity instanceof SheepEntity) {
            SheepEntity sheepEntity = (SheepEntity)entity;
            int n = WoolBlock.getBlockMeta(stack.getDamage());
            if (!sheepEntity.isSheared() && sheepEntity.getColor() != n) {
                sheepEntity.setColor(n);
                --stack.count;
            }
        }
    }
}

