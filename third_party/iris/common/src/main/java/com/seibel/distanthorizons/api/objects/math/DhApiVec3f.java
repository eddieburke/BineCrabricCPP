/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.math;

import com.seibel.distanthorizons.api.interfaces.util.IDhApiCopyable;

public class DhApiVec3f
implements IDhApiCopyable {
    public float x;
    public float y;
    public float z;

    public DhApiVec3f() {
        this.x = 0.0f;
        this.y = 0.0f;
        this.z = 0.0f;
    }

    public DhApiVec3f(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj != null && this.getClass() == obj.getClass()) {
            DhApiVec3f Vec3f = (DhApiVec3f)obj;
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

    @Override
    public DhApiVec3f copy() {
        return new DhApiVec3f(this.x, this.y, this.z);
    }
}

