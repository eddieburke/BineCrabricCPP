/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.math;

public class DhApiVec3i {
    public int x;
    public int y;
    public int z;

    public DhApiVec3i() {
        this.x = 0;
        this.y = 0;
        this.z = 0;
    }

    public DhApiVec3i(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj != null && this.getClass() == obj.getClass()) {
            DhApiVec3i Vec3f = (DhApiVec3i)obj;
            if (Float.compare(Vec3f.x, this.x) != 0) {
                return false;
            }
            if (Float.compare(Vec3f.y, this.y) != 0) {
                return false;
            }
            return Float.compare(Vec3f.z, this.z) == 0;
        }
        return false;
    }

    public int hashCode() {
        int i = Float.floatToIntBits(this.x);
        i = 31 * i + Float.floatToIntBits(this.y);
        return 31 * i + Float.floatToIntBits(this.z);
    }

    public String toString() {
        return "[" + this.x + ", " + this.y + ", " + this.z + "]";
    }
}

