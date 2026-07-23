/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import net.minecraft.block.BedBlock;
import net.minecraft.block.material.Material;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.SpawnGroup;
import net.minecraft.entity.ai.pathing.Path;
import net.minecraft.entity.ai.pathing.PathNode;
import net.minecraft.entity.ai.pathing.PathNodeNavigator;
import net.minecraft.entity.mob.SkeletonEntity;
import net.minecraft.entity.mob.SpiderEntity;
import net.minecraft.entity.mob.ZombieEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.World;
import net.minecraft.world.biome.Biome;
import net.minecraft.world.biome.EntitySpawnGroup;

public final class NaturalSpawner {
    private static Set mobSpawningChunks = new HashSet();
    protected static final Class[] MONSTER_TYPE = new Class[]{SpiderEntity.class, ZombieEntity.class, SkeletonEntity.class};

    protected static BlockPos getRandomPosInChunk(World world, int chunkX, int chunkZ) {
        int n = chunkX + world.random.nextInt(16);
        int n2 = world.random.nextInt(128);
        int n3 = chunkZ + world.random.nextInt(16);
        return new BlockPos(n, n2, n3);
    }

    public static final int tick(World world, boolean spawnAnimals, boolean spawnMonsters) {
        Object object;
        int n;
        if (!spawnAnimals && !spawnMonsters) {
            return 0;
        }
        mobSpawningChunks.clear();
        for (n = 0; n < world.players.size(); ++n) {
            object = (PlayerEntity)world.players.get(n);
            int n2 = MathHelper.floor(((PlayerEntity)object).x / 16.0);
            int n3 = MathHelper.floor(((PlayerEntity)object).z / 16.0);
            int n4 = 8;
            for (int i = -n4; i <= n4; ++i) {
                for (int j = -n4; j <= n4; ++j) {
                    mobSpawningChunks.add(new ChunkPos(i + n2, j + n3));
                }
            }
        }
        n = 0;
        object = world.getSpawnPos();
        for (SpawnGroup spawnGroup : SpawnGroup.values()) {
            if (spawnGroup.isPeaceful() && !spawnMonsters || !spawnGroup.isPeaceful() && !spawnAnimals || world.countEntities(spawnGroup.getCreatureClass()) > spawnGroup.getCapacity() * mobSpawningChunks.size() / 256) continue;
            block6: for (ChunkPos chunkPos : mobSpawningChunks) {
                EntitySpawnGroup entitySpawnGroup2;
                Biome biome = world.getBiomeSource().getBiome(chunkPos);
                List list = biome.getSpawnableEntities(spawnGroup);
                if (list == null || list.isEmpty()) continue;
                int n5 = 0;
                for (EntitySpawnGroup entitySpawnGroup2 : list) {
                    n5 += entitySpawnGroup2.amount;
                }
                int n6 = world.random.nextInt(n5);
                entitySpawnGroup2 = (EntitySpawnGroup)list.get(0);
                for (EntitySpawnGroup entitySpawnGroup3 : list) {
                    if ((n6 -= entitySpawnGroup3.amount) >= 0) continue;
                    entitySpawnGroup2 = entitySpawnGroup3;
                    break;
                }
                BlockPos blockPos = NaturalSpawner.getRandomPosInChunk(world, chunkPos.x * 16, chunkPos.z * 16);
                int n7 = blockPos.x;
                int n8 = blockPos.y;
                int n9 = blockPos.z;
                if (world.shouldSuffocate(n7, n8, n9) || world.getMaterial(n7, n8, n9) != spawnGroup.getSpawnMaterial()) continue;
                int n10 = 0;
                for (int i = 0; i < 3; ++i) {
                    int n11 = n7;
                    int n12 = n8;
                    int n13 = n9;
                    int n14 = 6;
                    for (int j = 0; j < 4; ++j) {
                        LivingEntity livingEntity;
                        float f;
                        float f2;
                        float f3;
                        float f4;
                        float f5;
                        float f6;
                        float f7;
                        if (!NaturalSpawner.isValidSpawnPos(spawnGroup, world, n11 += world.random.nextInt(n14) - world.random.nextInt(n14), n12 += world.random.nextInt(1) - world.random.nextInt(1), n13 += world.random.nextInt(n14) - world.random.nextInt(n14)) || world.getClosestPlayer(f7 = (float)n11 + 0.5f, f6 = (float)n12, f5 = (float)n13 + 0.5f, 24.0) != null || (f4 = (f3 = f7 - (float)((Vec3i)object).x) * f3 + (f2 = f6 - (float)((Vec3i)object).y) * f2 + (f = f5 - (float)((Vec3i)object).z) * f) < 576.0f) continue;
                        try {
                            livingEntity = (LivingEntity)entitySpawnGroup2.clazz.getConstructor(World.class).newInstance(world);
                        }
                        catch (Exception exception) {
                            exception.printStackTrace();
                            return n;
                        }
                        livingEntity.setPositionAndAnglesKeepPrevAngles(f7, f6, f5, world.random.nextFloat() * 360.0f, 0.0f);
                        if (livingEntity.canSpawn()) {
                            world.spawnEntity(livingEntity);
                            NaturalSpawner.postSpawnEntity(livingEntity, world, f7, f6, f5);
                            if (++n10 >= livingEntity.getLimitPerChunk()) continue block6;
                        }
                        n += n10;
                    }
                }
            }
        }
        return n;
    }

    private static boolean isValidSpawnPos(SpawnGroup spawnGroup, World world, int x, int y, int z) {
        if (spawnGroup.getSpawnMaterial() == Material.WATER) {
            return world.getMaterial(x, y, z).isFluid() && !world.shouldSuffocate(x, y + 1, z);
        }
        return world.shouldSuffocate(x, y - 1, z) && !world.shouldSuffocate(x, y, z) && !world.getMaterial(x, y, z).isFluid() && !world.shouldSuffocate(x, y + 1, z);
    }

    private static void postSpawnEntity(LivingEntity entity, World world, float x, float y, float z) {
        if (entity instanceof SpiderEntity && world.random.nextInt(100) == 0) {
            SkeletonEntity skeletonEntity = new SkeletonEntity(world);
            skeletonEntity.setPositionAndAnglesKeepPrevAngles(x, y, z, entity.yaw, 0.0f);
            world.spawnEntity(skeletonEntity);
            skeletonEntity.setVehicle(entity);
        } else if (entity instanceof SheepEntity) {
            ((SheepEntity)entity).setColor(SheepEntity.generateDefaultColor(world.random));
        }
    }

    public static boolean spawnMonstersAndWakePlayers(World world, List players) {
        boolean bl = false;
        PathNodeNavigator pathNodeNavigator = new PathNodeNavigator(world);
        for (PlayerEntity playerEntity : players) {
            Class[] classArray = MONSTER_TYPE;
            if (classArray == null || classArray.length == 0) continue;
            boolean bl2 = false;
            for (int i = 0; i < 20 && !bl2; ++i) {
                Path path;
                LivingEntity livingEntity;
                int n;
                int n2 = MathHelper.floor(playerEntity.x) + world.random.nextInt(32) - world.random.nextInt(32);
                int n3 = MathHelper.floor(playerEntity.z) + world.random.nextInt(32) - world.random.nextInt(32);
                int n4 = MathHelper.floor(playerEntity.y) + world.random.nextInt(16) - world.random.nextInt(16);
                if (n4 < 1) {
                    n4 = 1;
                } else if (n4 > 128) {
                    n4 = 128;
                }
                int n5 = world.random.nextInt(classArray.length);
                for (n = n4; n > 2 && !world.shouldSuffocate(n2, n - 1, n3); --n) {
                }
                while (!NaturalSpawner.isValidSpawnPos(SpawnGroup.MONSTER, world, n2, n, n3) && n < n4 + 16 && n < 128) {
                    ++n;
                }
                if (n >= n4 + 16 || n >= 128) {
                    n = n4;
                    continue;
                }
                float f = (float)n2 + 0.5f;
                float f2 = n;
                float f3 = (float)n3 + 0.5f;
                try {
                    livingEntity = (LivingEntity)classArray[n5].getConstructor(World.class).newInstance(world);
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                    return bl;
                }
                livingEntity.setPositionAndAnglesKeepPrevAngles(f, f2, f3, world.random.nextFloat() * 360.0f, 0.0f);
                if (!livingEntity.canSpawn() || (path = pathNodeNavigator.findPath(livingEntity, playerEntity, 32.0f)) == null || path.length <= 1) continue;
                PathNode pathNode = path.getEnd();
                if (!(Math.abs((double)pathNode.x - playerEntity.x) < 1.5) || !(Math.abs((double)pathNode.z - playerEntity.z) < 1.5) || !(Math.abs((double)pathNode.y - playerEntity.y) < 1.5)) continue;
                Vec3i vec3i = BedBlock.findWakeUpPosition(world, MathHelper.floor(playerEntity.x), MathHelper.floor(playerEntity.y), MathHelper.floor(playerEntity.z), 1);
                if (vec3i == null) {
                    vec3i = new Vec3i(n2, n + 1, n3);
                }
                livingEntity.setPositionAndAnglesKeepPrevAngles((float)vec3i.x + 0.5f, vec3i.y, (float)vec3i.z + 0.5f, 0.0f, 0.0f);
                world.spawnEntity(livingEntity);
                NaturalSpawner.postSpawnEntity(livingEntity, world, (float)vec3i.x + 0.5f, vec3i.y, (float)vec3i.z + 0.5f);
                playerEntity.wakeUp(true, false, false);
                livingEntity.makeSound();
                bl = true;
                bl2 = true;
            }
        }
        return bl;
    }
}

