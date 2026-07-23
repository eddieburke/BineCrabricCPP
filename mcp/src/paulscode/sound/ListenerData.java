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
import paulscode.sound.Vector3D;

@Environment(value=EnvType.CLIENT)
public class ListenerData {
    public Vector3D position;
    public Vector3D lookAt;
    public Vector3D up;
    public float angle = 0.0f;

    public ListenerData() {
        this.position = new Vector3D(0.0f, 0.0f, 0.0f);
        this.lookAt = new Vector3D(0.0f, 0.0f, -1.0f);
        this.up = new Vector3D(0.0f, 1.0f, 0.0f);
        this.angle = 0.0f;
    }

    public ListenerData(float f, float g, float h, float i, float j, float k, float l, float m, float n, float o) {
        this.position = new Vector3D(f, g, h);
        this.lookAt = new Vector3D(i, j, k);
        this.up = new Vector3D(l, m, n);
        this.angle = o;
    }

    public ListenerData(Vector3D vector3D, Vector3D vector3D2, Vector3D vector3D3, float f) {
        this.position = vector3D.clone();
        this.lookAt = vector3D2.clone();
        this.up = vector3D3.clone();
        this.angle = f;
    }

    public void setData(float f, float g, float h, float i, float j, float k, float l, float m, float n, float o) {
        this.position.x = f;
        this.position.y = g;
        this.position.z = h;
        this.lookAt.x = i;
        this.lookAt.y = j;
        this.lookAt.z = k;
        this.up.x = l;
        this.up.y = m;
        this.up.z = n;
        this.angle = o;
    }

    public void setData(Vector3D vector3D, Vector3D vector3D2, Vector3D vector3D3, float f) {
        this.position.x = vector3D.x;
        this.position.y = vector3D.y;
        this.position.z = vector3D.z;
        this.lookAt.x = vector3D2.x;
        this.lookAt.y = vector3D2.y;
        this.lookAt.z = vector3D2.z;
        this.up.x = vector3D3.x;
        this.up.y = vector3D3.y;
        this.up.z = vector3D3.z;
        this.angle = f;
    }

    public void setData(ListenerData listenerData) {
        this.position.x = listenerData.position.x;
        this.position.y = listenerData.position.y;
        this.position.z = listenerData.position.z;
        this.lookAt.x = listenerData.lookAt.x;
        this.lookAt.y = listenerData.lookAt.y;
        this.lookAt.z = listenerData.lookAt.z;
        this.up.x = listenerData.up.x;
        this.up.y = listenerData.up.y;
        this.up.z = listenerData.up.z;
        this.angle = listenerData.angle;
    }

    public void setPosition(float f, float g, float h) {
        this.position.x = f;
        this.position.y = g;
        this.position.z = h;
    }

    public void setPosition(Vector3D vector3D) {
        this.position.x = vector3D.x;
        this.position.y = vector3D.y;
        this.position.z = vector3D.z;
    }

    public void setOrientation(float f, float g, float h, float i, float j, float k) {
        this.lookAt.x = f;
        this.lookAt.y = g;
        this.lookAt.z = h;
        this.up.x = i;
        this.up.y = j;
        this.up.z = k;
    }

    public void setOrientation(Vector3D vector3D, Vector3D vector3D2) {
        this.lookAt.x = vector3D.x;
        this.lookAt.y = vector3D.y;
        this.lookAt.z = vector3D.z;
        this.up.x = vector3D2.x;
        this.up.y = vector3D2.y;
        this.up.z = vector3D2.z;
    }

    public void setAngle(float f) {
        this.angle = f;
        this.lookAt.x = -1.0f * (float)Math.sin(this.angle);
        this.lookAt.z = -1.0f * (float)Math.cos(this.angle);
    }
}

