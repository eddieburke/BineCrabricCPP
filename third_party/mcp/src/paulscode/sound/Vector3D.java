/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class Vector3D {
    public float x;
    public float y;
    public float z;

    public Vector3D() {
        this.x = 0.0f;
        this.y = 0.0f;
        this.z = 0.0f;
    }

    public Vector3D(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public Vector3D clone() {
        return new Vector3D(this.x, this.y, this.z);
    }

    public Vector3D cross(Vector3D vec1, Vector3D vec2) {
        return new Vector3D(vec1.y * vec2.z - vec2.y * vec1.z, vec1.z * vec2.x - vec2.z * vec1.x, vec1.x * vec2.y - vec2.x * vec1.y);
    }

    public Vector3D cross(Vector3D vec) {
        return new Vector3D(this.y * vec.z - vec.y * this.z, this.z * vec.x - vec.z * this.x, this.x * vec.y - vec.x * this.y);
    }

    public float dot(Vector3D vec1, Vector3D vec2) {
        return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
    }

    public float dot(Vector3D vec) {
        return this.x * vec.x + this.y * vec.y + this.z * vec.z;
    }

    public Vector3D add(Vector3D vec1, Vector3D vec2) {
        return new Vector3D(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z);
    }

    public Vector3D add(Vector3D vec) {
        return new Vector3D(this.x + vec.x, this.y + vec.y, this.z + vec.z);
    }

    public Vector3D subtract(Vector3D vec1, Vector3D vec2) {
        return new Vector3D(vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z);
    }

    public Vector3D subtract(Vector3D vec) {
        return new Vector3D(this.x - vec.x, this.y - vec.y, this.z - vec.z);
    }

    public void normalize() {
        double d = Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
        this.x = (float)((double)this.x / d);
        this.y = (float)((double)this.y / d);
        this.z = (float)((double)this.z / d);
    }
}

