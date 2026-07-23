/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.AbstractLightningEntity;
import net.minecraft.entity.Entity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class LightningEntity
extends AbstractLightningEntity {
    private int ambientTick;
    public long seed = 0L;
    private int remainingActions;

    public LightningEntity(World world, double x, double y, double z) {
        super(world);
        this.setPositionAndAnglesKeepPrevAngles(x, y, z, 0.0f, 0.0f);
        this.ambientTick = 2;
        this.seed = this.random.nextLong();
        this.remainingActions = this.random.nextInt(3) + 1;
        if (world.difficulty >= 2 && world.isRegionLoaded(MathHelper.floor(x), MathHelper.floor(y), MathHelper.floor(z), 10)) {
            int n;
            int n2;
            int n3 = MathHelper.floor(x);
            if (world.getBlockId(n3, n2 = MathHelper.floor(y), n = MathHelper.floor(z)) == 0 && Block.FIRE.canPlaceAt(world, n3, n2, n)) {
                world.setBlock(n3, n2, n, Block.FIRE.id);
            }
            for (n3 = 0; n3 < 4; ++n3) {
                int n4;
                n2 = MathHelper.floor(x) + this.random.nextInt(3) - 1;
                if (world.getBlockId(n2, n = MathHelper.floor(y) + this.random.nextInt(3) - 1, n4 = MathHelper.floor(z) + this.random.nextInt(3) - 1) != 0 || !Block.FIRE.canPlaceAt(world, n2, n, n4)) continue;
                world.setBlock(n2, n, n4, Block.FIRE.id);
            }
        }
    }

    public void tick() {
        super.tick();
        if (this.ambientTick == 2) {
            this.world.playSound(this.x, this.y, this.z, "ambient.weather.thunder", 10000.0f, 0.8f + this.random.nextFloat() * 0.2f);
            this.world.playSound(this.x, this.y, this.z, "random.explode", 2.0f, 0.5f + this.random.nextFloat() * 0.2f);
        }
        --this.ambientTick;
        if (this.ambientTick < 0) {
            if (this.remainingActions == 0) {
                this.markDead();
            } else if (this.ambientTick < -this.random.nextInt(10)) {
                int n;
                int n2;
                int n3;
                --this.remainingActions;
                this.ambientTick = 1;
                this.seed = this.random.nextLong();
                if (this.world.isRegionLoaded(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z), 10) && this.world.getBlockId(n3 = MathHelper.floor(this.x), n2 = MathHelper.floor(this.y), n = MathHelper.floor(this.z)) == 0 && Block.FIRE.canPlaceAt(this.world, n3, n2, n)) {
                    this.world.setBlock(n3, n2, n, Block.FIRE.id);
                }
            }
        }
        if (this.ambientTick >= 0) {
            double d = 3.0;
            List list = this.world.getEntities(this, Box.createCached(this.x - d, this.y - d, this.z - d, this.x + d, this.y + 6.0 + d, this.z + d));
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                entity.onStruckByLightning(this);
            }
            this.world.lightningTicksLeft = 2;
        }
    }

    protected void initDataTracker() {
    }

    protected void readNbt(NbtCompound nbt) {
    }

    protected void writeNbt(NbtCompound nbt) {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(Vec3d pos) {
        return this.ambientTick >= 0;
    }
}

