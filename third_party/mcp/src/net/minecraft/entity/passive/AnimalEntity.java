/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.EnvironmentInterface
 *  net.fabricmc.api.EnvironmentInterfaces
 */
package net.minecraft.entity.passive;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.EnvironmentInterface;
import net.fabricmc.api.EnvironmentInterfaces;
import net.minecraft.block.Block;
import net.minecraft.entity.MobEntity;
import net.minecraft.entity.SpawnableEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@EnvironmentInterfaces(value={@EnvironmentInterface(value=EnvType.SERVER, itf=SpawnableEntity.class)})
public abstract class AnimalEntity
extends MobEntity
implements SpawnableEntity {
    public AnimalEntity(World arg) {
        super(arg);
    }

    protected float getPathfindingFavor(int x, int y, int z) {
        if (this.world.getBlockId(x, y - 1, z) == Block.GRASS_BLOCK.id) {
            return 10.0f;
        }
        return this.world.getLightBrightness(x, y, z) - 0.5f;
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    public boolean canSpawn() {
        int n;
        int n2;
        int n3 = MathHelper.floor(this.x);
        return this.world.getBlockId(n3, (n2 = MathHelper.floor(this.boundingBox.minY)) - 1, n = MathHelper.floor(this.z)) == Block.GRASS_BLOCK.id && this.world.getBrightness(n3, n2, n) > 8 && super.canSpawn();
    }

    public int getMinAmbientSoundDelay() {
        return 120;
    }
}

