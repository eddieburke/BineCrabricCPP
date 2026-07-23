/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.util.math;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.MathHelper;

public class Vec3d {
    private static List cache = new ArrayList();
    private static int cacheCount = 0;
    public double x;
    public double y;
    public double z;

    public static Vec3d create(double x, double y, double z) {
        return new Vec3d(x, y, z);
    }

    @Environment(value=EnvType.CLIENT)
    public static void clearCache() {
        cache.clear();
        cacheCount = 0;
    }

    public static void resetCacheCount() {
        cacheCount = 0;
    }

    public static Vec3d createCached(double x, double y, double z) {
        if (cacheCount >= cache.size()) {
            cache.add(Vec3d.create(0.0, 0.0, 0.0));
        }
        return ((Vec3d)cache.get(cacheCount++)).set(x, y, z);
    }

    private Vec3d(double x, double y, double z) {
        if (x == -0.0) {
            x = 0.0;
        }
        if (y == -0.0) {
            y = 0.0;
        }
        if (z == -0.0) {
            z = 0.0;
        }
        this.x = x;
        this.y = y;
        this.z = z;
    }

    private Vec3d set(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d relativize(Vec3d vec) {
        return Vec3d.createCached(vec.x - this.x, vec.y - this.y, vec.z - this.z);
    }

    public Vec3d normalize() {
        double d = MathHelper.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
        if (d < 1.0E-4) {
            return Vec3d.createCached(0.0, 0.0, 0.0);
        }
        return Vec3d.createCached(this.x / d, this.y / d, this.z / d);
    }

    @Environment(value=EnvType.CLIENT)
    public Vec3d crossProduct(Vec3d vec) {
        return Vec3d.createCached(this.y * vec.z - this.z * vec.y, this.z * vec.x - this.x * vec.z, this.x * vec.y - this.y * vec.x);
    }

    public Vec3d add(double x, double y, double z) {
        return Vec3d.createCached(this.x + x, this.y + y, this.z + z);
    }

    public double distanceTo(Vec3d vec) {
        double d = vec.x - this.x;
        double d2 = vec.y - this.y;
        double d3 = vec.z - this.z;
        return MathHelper.sqrt(d * d + d2 * d2 + d3 * d3);
    }

    public double squaredDistanceTo(Vec3d vec) {
        double d = vec.x - this.x;
        double d2 = vec.y - this.y;
        double d3 = vec.z - this.z;
        return d * d + d2 * d2 + d3 * d3;
    }

    public double squaredDistanceTo(double x, double y, double z) {
        double d = x - this.x;
        double d2 = y - this.y;
        double d3 = z - this.z;
        return d * d + d2 * d2 + d3 * d3;
    }

    public double length() {
        return MathHelper.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    }

    public Vec3d interpolateByX(Vec3d vec, double deltaX) {
        double d = vec.x - this.x;
        double d2 = vec.y - this.y;
        double d3 = vec.z - this.z;
        if (d * d < (double)1.0E-7f) {
            return null;
        }
        double d4 = (deltaX - this.x) / d;
        if (d4 < 0.0 || d4 > 1.0) {
            return null;
        }
        return Vec3d.createCached(this.x + d * d4, this.y + d2 * d4, this.z + d3 * d4);
    }

    public Vec3d interpolateByY(Vec3d vec, double deltaY) {
        double d = vec.x - this.x;
        double d2 = vec.y - this.y;
        double d3 = vec.z - this.z;
        if (d2 * d2 < (double)1.0E-7f) {
            return null;
        }
        double d4 = (deltaY - this.y) / d2;
        if (d4 < 0.0 || d4 > 1.0) {
            return null;
        }
        return Vec3d.createCached(this.x + d * d4, this.y + d2 * d4, this.z + d3 * d4);
    }

    public Vec3d interpolateByZ(Vec3d vec, double deltaZ) {
        double d = vec.x - this.x;
        double d2 = vec.y - this.y;
        double d3 = vec.z - this.z;
        if (d3 * d3 < (double)1.0E-7f) {
            return null;
        }
        double d4 = (deltaZ - this.z) / d3;
        if (d4 < 0.0 || d4 > 1.0) {
            return null;
        }
        return Vec3d.createCached(this.x + d * d4, this.y + d2 * d4, this.z + d3 * d4);
    }

    public String toString() {
        return "(" + this.x + ", " + this.y + ", " + this.z + ")";
    }

    @Environment(value=EnvType.CLIENT)
    public void rotateX(float angle) {
        float f = MathHelper.cos(angle);
        float f2 = MathHelper.sin(angle);
        double d = this.x;
        double d2 = this.y * (double)f + this.z * (double)f2;
        double d3 = this.z * (double)f - this.y * (double)f2;
        this.x = d;
        this.y = d2;
        this.z = d3;
    }

    @Environment(value=EnvType.CLIENT)
    public void rotateY(float angle) {
        float f = MathHelper.cos(angle);
        float f2 = MathHelper.sin(angle);
        double d = this.x * (double)f + this.z * (double)f2;
        double d2 = this.y;
        double d3 = this.z * (double)f - this.x * (double)f2;
        this.x = d;
        this.y = d2;
        this.z = d3;
    }
}

