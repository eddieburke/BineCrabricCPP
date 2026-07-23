/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.explosion;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class Explosion {
    public boolean fire = false;
    private Random random = new Random();
    private World world;
    public double x;
    public double y;
    public double z;
    public Entity source;
    public float power;
    public Set damagedBlocks = new HashSet();

    public Explosion(World world, Entity source, double x, double y, double z, float power) {
        this.world = world;
        this.source = source;
        this.power = power;
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public void explode() {
        double d;
        double d2;
        double d3;
        int n;
        int n2;
        int n3;
        float f = this.power;
        int n4 = 16;
        for (n3 = 0; n3 < n4; ++n3) {
            for (n2 = 0; n2 < n4; ++n2) {
                for (n = 0; n < n4; ++n) {
                    if (n3 != 0 && n3 != n4 - 1 && n2 != 0 && n2 != n4 - 1 && n != 0 && n != n4 - 1) continue;
                    double d4 = (float)n3 / ((float)n4 - 1.0f) * 2.0f - 1.0f;
                    double d5 = (float)n2 / ((float)n4 - 1.0f) * 2.0f - 1.0f;
                    double d6 = (float)n / ((float)n4 - 1.0f) * 2.0f - 1.0f;
                    double d7 = Math.sqrt(d4 * d4 + d5 * d5 + d6 * d6);
                    d4 /= d7;
                    d5 /= d7;
                    d6 /= d7;
                    d3 = this.x;
                    d2 = this.y;
                    d = this.z;
                    float f2 = 0.3f;
                    for (float f3 = this.power * (0.7f + this.world.random.nextFloat() * 0.6f); f3 > 0.0f; f3 -= f2 * 0.75f) {
                        int n5;
                        int n6;
                        int n7 = MathHelper.floor(d3);
                        int n8 = this.world.getBlockId(n7, n6 = MathHelper.floor(d2), n5 = MathHelper.floor(d));
                        if (n8 > 0) {
                            f3 -= (Block.BLOCKS[n8].getBlastResistance(this.source) + 0.3f) * f2;
                        }
                        if (f3 > 0.0f) {
                            this.damagedBlocks.add(new BlockPos(n7, n6, n5));
                        }
                        d3 += d4 * (double)f2;
                        d2 += d5 * (double)f2;
                        d += d6 * (double)f2;
                    }
                }
            }
        }
        this.power *= 2.0f;
        n3 = MathHelper.floor(this.x - (double)this.power - 1.0);
        n2 = MathHelper.floor(this.x + (double)this.power + 1.0);
        n = MathHelper.floor(this.y - (double)this.power - 1.0);
        int n9 = MathHelper.floor(this.y + (double)this.power + 1.0);
        int n10 = MathHelper.floor(this.z - (double)this.power - 1.0);
        int n11 = MathHelper.floor(this.z + (double)this.power + 1.0);
        List list = this.world.getEntities(this.source, Box.createCached(n3, n, n10, n2, n9, n11));
        Vec3d vec3d = Vec3d.createCached(this.x, this.y, this.z);
        for (int i = 0; i < list.size(); ++i) {
            Entity entity = (Entity)list.get(i);
            double d8 = entity.getDistance(this.x, this.y, this.z) / (double)this.power;
            if (!(d8 <= 1.0)) continue;
            d3 = entity.x - this.x;
            d2 = entity.y - this.y;
            d = entity.z - this.z;
            double d9 = MathHelper.sqrt(d3 * d3 + d2 * d2 + d * d);
            d3 /= d9;
            d2 /= d9;
            d /= d9;
            double d10 = this.world.getVisibilityRatio(vec3d, entity.boundingBox);
            double d11 = (1.0 - d8) * d10;
            entity.damage(this.source, (int)((d11 * d11 + d11) / 2.0 * 8.0 * (double)this.power + 1.0));
            double d12 = d11;
            entity.velocityX += d3 * d12;
            entity.velocityY += d2 * d12;
            entity.velocityZ += d * d12;
        }
        this.power = f;
        ArrayList arrayList = new ArrayList();
        arrayList.addAll(this.damagedBlocks);
        if (this.fire) {
            for (int i = arrayList.size() - 1; i >= 0; --i) {
                BlockPos blockPos = (BlockPos)arrayList.get(i);
                int n12 = blockPos.x;
                int n13 = blockPos.y;
                int n14 = blockPos.z;
                int n15 = this.world.getBlockId(n12, n13, n14);
                int n16 = this.world.getBlockId(n12, n13 - 1, n14);
                if (n15 != 0 || !Block.BLOCKS_OPAQUE[n16] || this.random.nextInt(3) != 0) continue;
                this.world.setBlock(n12, n13, n14, Block.FIRE.id);
            }
        }
    }

    public void playExplosionSound(boolean addParticles) {
        this.world.playSound(this.x, this.y, this.z, "random.explode", 4.0f, (1.0f + (this.world.random.nextFloat() - this.world.random.nextFloat()) * 0.2f) * 0.7f);
        ArrayList arrayList = new ArrayList();
        arrayList.addAll(this.damagedBlocks);
        for (int i = arrayList.size() - 1; i >= 0; --i) {
            BlockPos blockPos = (BlockPos)arrayList.get(i);
            int n = blockPos.x;
            int n2 = blockPos.y;
            int n3 = blockPos.z;
            int n4 = this.world.getBlockId(n, n2, n3);
            if (addParticles) {
                double d = (float)n + this.world.random.nextFloat();
                double d2 = (float)n2 + this.world.random.nextFloat();
                double d3 = (float)n3 + this.world.random.nextFloat();
                double d4 = d - this.x;
                double d5 = d2 - this.y;
                double d6 = d3 - this.z;
                double d7 = MathHelper.sqrt(d4 * d4 + d5 * d5 + d6 * d6);
                d4 /= d7;
                d5 /= d7;
                d6 /= d7;
                double d8 = 0.5 / (d7 / (double)this.power + 0.1);
                this.world.addParticle("explode", (d + this.x * 1.0) / 2.0, (d2 + this.y * 1.0) / 2.0, (d3 + this.z * 1.0) / 2.0, d4 *= (d8 *= (double)(this.world.random.nextFloat() * this.world.random.nextFloat() + 0.3f)), d5 *= d8, d6 *= d8);
                this.world.addParticle("smoke", d, d2, d3, d4, d5, d6);
            }
            if (n4 <= 0) continue;
            Block.BLOCKS[n4].dropStacks(this.world, n, n2, n3, this.world.getBlockMeta(n, n2, n3), 0.3f);
            this.world.setBlock(n, n2, n3, 0);
            Block.BLOCKS[n4].onDestroyedByExplosion(this.world, n, n2, n3);
        }
    }
}

