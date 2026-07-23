/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.block.Block;
import net.minecraft.block.entity.ChestBlockEntity;
import net.minecraft.block.entity.MobSpawnerBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;
import net.minecraft.world.gen.feature.Feature;

public class DungeonFeature
extends Feature {
    public boolean generate(World world, Random random, int x, int y, int z) {
        int n;
        int n2;
        int n3;
        int n4 = 3;
        int n5 = random.nextInt(2) + 2;
        int n6 = random.nextInt(2) + 2;
        int n7 = 0;
        for (n3 = x - n5 - 1; n3 <= x + n5 + 1; ++n3) {
            for (n2 = y - 1; n2 <= y + n4 + 1; ++n2) {
                for (n = z - n6 - 1; n <= z + n6 + 1; ++n) {
                    Material material = world.getMaterial(n3, n2, n);
                    if (n2 == y - 1 && !material.isSolid()) {
                        return false;
                    }
                    if (n2 == y + n4 + 1 && !material.isSolid()) {
                        return false;
                    }
                    if (n3 != x - n5 - 1 && n3 != x + n5 + 1 && n != z - n6 - 1 && n != z + n6 + 1 || n2 != y || !world.isAir(n3, n2, n) || !world.isAir(n3, n2 + 1, n)) continue;
                    ++n7;
                }
            }
        }
        if (n7 < 1 || n7 > 5) {
            return false;
        }
        for (n3 = x - n5 - 1; n3 <= x + n5 + 1; ++n3) {
            for (n2 = y + n4; n2 >= y - 1; --n2) {
                for (n = z - n6 - 1; n <= z + n6 + 1; ++n) {
                    if (n3 == x - n5 - 1 || n2 == y - 1 || n == z - n6 - 1 || n3 == x + n5 + 1 || n2 == y + n4 + 1 || n == z + n6 + 1) {
                        if (n2 >= 0 && !world.getMaterial(n3, n2 - 1, n).isSolid()) {
                            world.setBlock(n3, n2, n, 0);
                            continue;
                        }
                        if (!world.getMaterial(n3, n2, n).isSolid()) continue;
                        if (n2 == y - 1 && random.nextInt(4) != 0) {
                            world.setBlock(n3, n2, n, Block.MOSSY_COBBLESTONE.id);
                            continue;
                        }
                        world.setBlock(n3, n2, n, Block.COBBLESTONE.id);
                        continue;
                    }
                    world.setBlock(n3, n2, n, 0);
                }
            }
        }
        block6: for (n3 = 0; n3 < 2; ++n3) {
            for (n2 = 0; n2 < 3; ++n2) {
                int n8;
                int n9;
                n = x + random.nextInt(n5 * 2 + 1) - n5;
                if (!world.isAir(n, n9 = y, n8 = z + random.nextInt(n6 * 2 + 1) - n6)) continue;
                int n10 = 0;
                if (world.getMaterial(n - 1, n9, n8).isSolid()) {
                    ++n10;
                }
                if (world.getMaterial(n + 1, n9, n8).isSolid()) {
                    ++n10;
                }
                if (world.getMaterial(n, n9, n8 - 1).isSolid()) {
                    ++n10;
                }
                if (world.getMaterial(n, n9, n8 + 1).isSolid()) {
                    ++n10;
                }
                if (n10 != 1) continue;
                world.setBlock(n, n9, n8, Block.CHEST.id);
                ChestBlockEntity chestBlockEntity = (ChestBlockEntity)world.getBlockEntity(n, n9, n8);
                for (int i = 0; i < 8; ++i) {
                    ItemStack itemStack = this.getRandomChestItem(random);
                    if (itemStack == null) continue;
                    chestBlockEntity.setStack(random.nextInt(chestBlockEntity.size()), itemStack);
                }
                continue block6;
            }
        }
        world.setBlock(x, y, z, Block.SPAWNER.id);
        MobSpawnerBlockEntity mobSpawnerBlockEntity = (MobSpawnerBlockEntity)world.getBlockEntity(x, y, z);
        mobSpawnerBlockEntity.setSpawnedEntityId(this.getRandomEntity(random));
        return true;
    }

    private ItemStack getRandomChestItem(Random random) {
        int n = random.nextInt(11);
        if (n == 0) {
            return new ItemStack(Item.SADDLE);
        }
        if (n == 1) {
            return new ItemStack(Item.IRON_INGOT, random.nextInt(4) + 1);
        }
        if (n == 2) {
            return new ItemStack(Item.BREAD);
        }
        if (n == 3) {
            return new ItemStack(Item.WHEAT, random.nextInt(4) + 1);
        }
        if (n == 4) {
            return new ItemStack(Item.GUNPOWDER, random.nextInt(4) + 1);
        }
        if (n == 5) {
            return new ItemStack(Item.STRING, random.nextInt(4) + 1);
        }
        if (n == 6) {
            return new ItemStack(Item.BUCKET);
        }
        if (n == 7 && random.nextInt(100) == 0) {
            return new ItemStack(Item.GOLDEN_APPLE);
        }
        if (n == 8 && random.nextInt(2) == 0) {
            return new ItemStack(Item.REDSTONE, random.nextInt(4) + 1);
        }
        if (n == 9 && random.nextInt(10) == 0) {
            return new ItemStack(Item.ITEMS[Item.RECORD_THIRTEEN.id + random.nextInt(2)]);
        }
        if (n == 10) {
            return new ItemStack(Item.DYE, 1, 3);
        }
        return null;
    }

    private String getRandomEntity(Random random) {
        int n = random.nextInt(4);
        if (n == 0) {
            return "Skeleton";
        }
        if (n == 1) {
            return "Zombie";
        }
        if (n == 2) {
            return "Zombie";
        }
        if (n == 3) {
            return "Spider";
        }
        return "";
    }
}

