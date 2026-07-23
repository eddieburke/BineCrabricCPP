/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import java.util.HashMap;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.mob.CreeperEntity;
import net.minecraft.entity.mob.GhastEntity;
import net.minecraft.entity.mob.GiantEntity;
import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.entity.mob.PigZombieEntity;
import net.minecraft.entity.mob.SkeletonEntity;
import net.minecraft.entity.mob.SlimeEntity;
import net.minecraft.entity.mob.SpiderEntity;
import net.minecraft.entity.mob.ZombieEntity;
import net.minecraft.entity.passive.ChickenEntity;
import net.minecraft.entity.passive.CowEntity;
import net.minecraft.entity.passive.PigEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.passive.SquidEntity;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class EntityRegistry {
    private static Map idToClass = new HashMap();
    private static Map classToId = new HashMap();
    private static Map rawIdToClass = new HashMap();
    private static Map classToRawId = new HashMap();

    private static void register(Class entityClass, String id, int rawId) {
        idToClass.put(id, entityClass);
        classToId.put(entityClass, id);
        rawIdToClass.put(rawId, entityClass);
        classToRawId.put(entityClass, rawId);
    }

    public static Entity create(String id, World world) {
        Entity entity = null;
        try {
            Class clazz = (Class)idToClass.get(id);
            if (clazz != null) {
                entity = (Entity)clazz.getConstructor(World.class).newInstance(world);
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        return entity;
    }

    public static Entity getEntityFromNbt(NbtCompound nbt, World world) {
        Entity entity = null;
        try {
            Class clazz = (Class)idToClass.get(nbt.getString("id"));
            if (clazz != null) {
                entity = (Entity)clazz.getConstructor(World.class).newInstance(world);
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        if (entity != null) {
            entity.read(nbt);
        } else {
            System.out.println("Skipping Entity with id " + nbt.getString("id"));
        }
        return entity;
    }

    @Environment(value=EnvType.CLIENT)
    public static Entity create(int rawId, World world) {
        Entity entity = null;
        try {
            Class clazz = (Class)rawIdToClass.get(rawId);
            if (clazz != null) {
                entity = (Entity)clazz.getConstructor(World.class).newInstance(world);
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        if (entity == null) {
            System.out.println("Skipping Entity with id " + rawId);
        }
        return entity;
    }

    public static int getRawId(Entity entity) {
        return (Integer)classToRawId.get(entity.getClass());
    }

    public static String getId(Entity entity) {
        return (String)classToId.get(entity.getClass());
    }

    static {
        EntityRegistry.register(ArrowEntity.class, "Arrow", 10);
        EntityRegistry.register(SnowballEntity.class, "Snowball", 11);
        EntityRegistry.register(ItemEntity.class, "Item", 1);
        EntityRegistry.register(PaintingEntity.class, "Painting", 9);
        EntityRegistry.register(LivingEntity.class, "Mob", 48);
        EntityRegistry.register(MonsterEntity.class, "Monster", 49);
        EntityRegistry.register(CreeperEntity.class, "Creeper", 50);
        EntityRegistry.register(SkeletonEntity.class, "Skeleton", 51);
        EntityRegistry.register(SpiderEntity.class, "Spider", 52);
        EntityRegistry.register(GiantEntity.class, "Giant", 53);
        EntityRegistry.register(ZombieEntity.class, "Zombie", 54);
        EntityRegistry.register(SlimeEntity.class, "Slime", 55);
        EntityRegistry.register(GhastEntity.class, "Ghast", 56);
        EntityRegistry.register(PigZombieEntity.class, "PigZombie", 57);
        EntityRegistry.register(PigEntity.class, "Pig", 90);
        EntityRegistry.register(SheepEntity.class, "Sheep", 91);
        EntityRegistry.register(CowEntity.class, "Cow", 92);
        EntityRegistry.register(ChickenEntity.class, "Chicken", 93);
        EntityRegistry.register(SquidEntity.class, "Squid", 94);
        EntityRegistry.register(WolfEntity.class, "Wolf", 95);
        EntityRegistry.register(TntEntity.class, "PrimedTnt", 20);
        EntityRegistry.register(FallingBlockEntity.class, "FallingSand", 21);
        EntityRegistry.register(MinecartEntity.class, "Minecart", 40);
        EntityRegistry.register(BoatEntity.class, "Boat", 41);
    }
}

