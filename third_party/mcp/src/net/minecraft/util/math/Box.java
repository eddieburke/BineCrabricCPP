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
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Vec3d;

public class Box {
    private static List cache = new ArrayList();
    private static int cacheCount = 0;
    public double minX;
    public double minY;
    public double minZ;
    public double maxX;
    public double maxY;
    public double maxZ;

    public static Box create(double x1, double y1, double z1, double x2, double y2, double z2) {
        return new Box(x1, y1, z1, x2, y2, z2);
    }

    @Environment(value=EnvType.CLIENT)
    public static void clearCache() {
        cache.clear();
        cacheCount = 0;
    }

    public static void resetCacheCount() {
        cacheCount = 0;
    }

    public static Box createCached(double x1, double y1, double z1, double x2, double y2, double z2) {
        if (cacheCount >= cache.size()) {
            cache.add(Box.create(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
        }
        return ((Box)cache.get(cacheCount++)).set(x1, y1, z1, x2, y2, z2);
    }

    private Box(double x1, double y1, double z1, double x2, double y2, double z2) {
        this.minX = x1;
        this.minY = y1;
        this.minZ = z1;
        this.maxX = x2;
        this.maxY = y2;
        this.maxZ = z2;
    }

    public Box set(double x1, double y1, double z1, double x2, double y2, double z2) {
        this.minX = x1;
        this.minY = y1;
        this.minZ = z1;
        this.maxX = x2;
        this.maxY = y2;
        this.maxZ = z2;
        return this;
    }

    public Box stretch(double x, double y, double z) {
        double d = this.minX;
        double d2 = this.minY;
        double d3 = this.minZ;
        double d4 = this.maxX;
        double d5 = this.maxY;
        double d6 = this.maxZ;
        if (x < 0.0) {
            d += x;
        }
        if (x > 0.0) {
            d4 += x;
        }
        if (y < 0.0) {
            d2 += y;
        }
        if (y > 0.0) {
            d5 += y;
        }
        if (z < 0.0) {
            d3 += z;
        }
        if (z > 0.0) {
            d6 += z;
        }
        return Box.createCached(d, d2, d3, d4, d5, d6);
    }

    public Box expand(double x, double y, double z) {
        double d = this.minX - x;
        double d2 = this.minY - y;
        double d3 = this.minZ - z;
        double d4 = this.maxX + x;
        double d5 = this.maxY + y;
        double d6 = this.maxZ + z;
        return Box.createCached(d, d2, d3, d4, d5, d6);
    }

    public Box offset(double x, double y, double z) {
        return Box.createCached(this.minX + x, this.minY + y, this.minZ + z, this.maxX + x, this.maxY + y, this.maxZ + z);
    }

    public double getXOffset(Box box, double x) {
        double d;
        if (box.maxY <= this.minY || box.minY >= this.maxY) {
            return x;
        }
        if (box.maxZ <= this.minZ || box.minZ >= this.maxZ) {
            return x;
        }
        if (x > 0.0 && box.maxX <= this.minX && (d = this.minX - box.maxX) < x) {
            x = d;
        }
        if (x < 0.0 && box.minX >= this.maxX && (d = this.maxX - box.minX) > x) {
            x = d;
        }
        return x;
    }

    public double getYOffset(Box box, double y) {
        double d;
        if (box.maxX <= this.minX || box.minX >= this.maxX) {
            return y;
        }
        if (box.maxZ <= this.minZ || box.minZ >= this.maxZ) {
            return y;
        }
        if (y > 0.0 && box.maxY <= this.minY && (d = this.minY - box.maxY) < y) {
            y = d;
        }
        if (y < 0.0 && box.minY >= this.maxY && (d = this.maxY - box.minY) > y) {
            y = d;
        }
        return y;
    }

    public double getZOffset(Box box, double z) {
        double d;
        if (box.maxX <= this.minX || box.minX >= this.maxX) {
            return z;
        }
        if (box.maxY <= this.minY || box.minY >= this.maxY) {
            return z;
        }
        if (z > 0.0 && box.maxZ <= this.minZ && (d = this.minZ - box.maxZ) < z) {
            z = d;
        }
        if (z < 0.0 && box.minZ >= this.maxZ && (d = this.maxZ - box.minZ) > z) {
            z = d;
        }
        return z;
    }

    public boolean intersects(Box box) {
        if (box.maxX <= this.minX || box.minX >= this.maxX) {
            return false;
        }
        if (box.maxY <= this.minY || box.minY >= this.maxY) {
            return false;
        }
        return !(box.maxZ <= this.minZ) && !(box.minZ >= this.maxZ);
    }

    public Box translate(double x, double y, double z) {
        this.minX += x;
        this.minY += y;
        this.minZ += z;
        this.maxX += x;
        this.maxY += y;
        this.maxZ += z;
        return this;
    }

    public boolean contains(Vec3d pos) {
        if (pos.x <= this.minX || pos.x >= this.maxX) {
            return false;
        }
        if (pos.y <= this.minY || pos.y >= this.maxY) {
            return false;
        }
        return !(pos.z <= this.minZ) && !(pos.z >= this.maxZ);
    }

    @Environment(value=EnvType.CLIENT)
    public double getAverageSideLength() {
        double d = this.maxX - this.minX;
        double d2 = this.maxY - this.minY;
        double d3 = this.maxZ - this.minZ;
        return (d + d2 + d3) / 3.0;
    }

    public Box contract(double x, double y, double z) {
        double d = this.minX + x;
        double d2 = this.minY + y;
        double d3 = this.minZ + z;
        double d4 = this.maxX - x;
        double d5 = this.maxY - y;
        double d6 = this.maxZ - z;
        return Box.createCached(d, d2, d3, d4, d5, d6);
    }

    public Box copy() {
        return Box.createCached(this.minX, this.minY, this.minZ, this.maxX, this.maxY, this.maxZ);
    }

    public HitResult raycast(Vec3d min, Vec3d max) {
        Vec3d vec3d = min.interpolateByX(max, this.minX);
        Vec3d vec3d2 = min.interpolateByX(max, this.maxX);
        Vec3d vec3d3 = min.interpolateByY(max, this.minY);
        Vec3d vec3d4 = min.interpolateByY(max, this.maxY);
        Vec3d vec3d5 = min.interpolateByZ(max, this.minZ);
        Vec3d vec3d6 = min.interpolateByZ(max, this.maxZ);
        if (!this.containsInYZPlane(vec3d)) {
            vec3d = null;
        }
        if (!this.containsInYZPlane(vec3d2)) {
            vec3d2 = null;
        }
        if (!this.containsInXZPlane(vec3d3)) {
            vec3d3 = null;
        }
        if (!this.containsInXZPlane(vec3d4)) {
            vec3d4 = null;
        }
        if (!this.containsInXYPlane(vec3d5)) {
            vec3d5 = null;
        }
        if (!this.containsInXYPlane(vec3d6)) {
            vec3d6 = null;
        }
        Vec3d vec3d7 = null;
        if (vec3d != null && (vec3d7 == null || min.squaredDistanceTo(vec3d) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d;
        }
        if (vec3d2 != null && (vec3d7 == null || min.squaredDistanceTo(vec3d2) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d2;
        }
        if (vec3d3 != null && (vec3d7 == null || min.squaredDistanceTo(vec3d3) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d3;
        }
        if (vec3d4 != null && (vec3d7 == null || min.squaredDistanceTo(vec3d4) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d4;
        }
        if (vec3d5 != null && (vec3d7 == null || min.squaredDistanceTo(vec3d5) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d5;
        }
        if (vec3d6 != null && (vec3d7 == null || min.squaredDistanceTo(vec3d6) < min.squaredDistanceTo(vec3d7))) {
            vec3d7 = vec3d6;
        }
        if (vec3d7 == null) {
            return null;
        }
        int n = -1;
        if (vec3d7 == vec3d) {
            n = 4;
        }
        if (vec3d7 == vec3d2) {
            n = 5;
        }
        if (vec3d7 == vec3d3) {
            n = 0;
        }
        if (vec3d7 == vec3d4) {
            n = 1;
        }
        if (vec3d7 == vec3d5) {
            n = 2;
        }
        if (vec3d7 == vec3d6) {
            n = 3;
        }
        return new HitResult(0, 0, 0, n, vec3d7);
    }

    private boolean containsInYZPlane(Vec3d pos) {
        if (pos == null) {
            return false;
        }
        return pos.y >= this.minY && pos.y <= this.maxY && pos.z >= this.minZ && pos.z <= this.maxZ;
    }

    private boolean containsInXZPlane(Vec3d pos) {
        if (pos == null) {
            return false;
        }
        return pos.x >= this.minX && pos.x <= this.maxX && pos.z >= this.minZ && pos.z <= this.maxZ;
    }

    private boolean containsInXYPlane(Vec3d pos) {
        if (pos == null) {
            return false;
        }
        return pos.x >= this.minX && pos.x <= this.maxX && pos.y >= this.minY && pos.y <= this.maxY;
    }

    public void clone(Box other) {
        this.minX = other.minX;
        this.minY = other.minY;
        this.minZ = other.minZ;
        this.maxX = other.maxX;
        this.maxY = other.maxY;
        this.maxZ = other.maxZ;
    }

    public String toString() {
        return "box[" + this.minX + ", " + this.minY + ", " + this.minZ + " -> " + this.maxX + ", " + this.maxY + ", " + this.maxZ + "]";
    }
}

