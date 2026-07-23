/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.EnvironmentInterface
 *  net.fabricmc.api.EnvironmentInterfaces
 */
package net.minecraft.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.EnvironmentInterface;
import net.fabricmc.api.EnvironmentInterfaces;
import net.minecraft.entity.MobEntity;
import net.minecraft.entity.SpawnableEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

@EnvironmentInterfaces(value={@EnvironmentInterface(value=EnvType.SERVER, itf=SpawnableEntity.class)})
public class WaterCreatureEntity
extends MobEntity
implements SpawnableEntity {
    public WaterCreatureEntity(World arg) {
        super(arg);
    }

    public boolean canBreatheInWater() {
        return true;
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    public boolean canSpawn() {
        return this.world.canSpawnEntity(this.boundingBox);
    }

    public int getMinAmbientSoundDelay() {
        return 120;
    }
}

