/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.model;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.model.Quad;
import net.minecraft.client.model.Vertex;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.util.GlAllocationUtils;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ModelPart {
    private Vertex[] corners;
    private Quad[] faces;
    private int u;
    private int v;
    public float pivotX;
    public float pivotY;
    public float pivotZ;
    public float pitch;
    public float yaw;
    public float roll;
    private boolean compiled = false;
    private int list = 0;
    public boolean mirror = false;
    public boolean visible = true;
    public boolean hidden = false;

    public ModelPart(int u, int v) {
        this.u = u;
        this.v = v;
    }

    public void addCuboid(float x, float y, float z, int sizeX, int sizeY, int sizeZ) {
        this.addCuboid(x, y, z, sizeX, sizeY, sizeZ, 0.0f);
    }

    public void addCuboid(float x, float y, float z, int sizeX, int sizeY, int sizeZ, float dilation) {
        this.corners = new Vertex[8];
        this.faces = new Quad[6];
        float f = x + (float)sizeX;
        float f2 = y + (float)sizeY;
        float f3 = z + (float)sizeZ;
        x -= dilation;
        y -= dilation;
        z -= dilation;
        f += dilation;
        f2 += dilation;
        f3 += dilation;
        if (this.mirror) {
            float f4 = f;
            f = x;
            x = f4;
        }
        Vertex vertex = new Vertex(x, y, z, 0.0f, 0.0f);
        Vertex vertex2 = new Vertex(f, y, z, 0.0f, 8.0f);
        Vertex vertex3 = new Vertex(f, f2, z, 8.0f, 8.0f);
        Vertex vertex4 = new Vertex(x, f2, z, 8.0f, 0.0f);
        Vertex vertex5 = new Vertex(x, y, f3, 0.0f, 0.0f);
        Vertex vertex6 = new Vertex(f, y, f3, 0.0f, 8.0f);
        Vertex vertex7 = new Vertex(f, f2, f3, 8.0f, 8.0f);
        Vertex vertex8 = new Vertex(x, f2, f3, 8.0f, 0.0f);
        this.corners[0] = vertex;
        this.corners[1] = vertex2;
        this.corners[2] = vertex3;
        this.corners[3] = vertex4;
        this.corners[4] = vertex5;
        this.corners[5] = vertex6;
        this.corners[6] = vertex7;
        this.corners[7] = vertex8;
        this.faces[0] = new Quad(new Vertex[]{vertex6, vertex2, vertex3, vertex7}, this.u + sizeZ + sizeX, this.v + sizeZ, this.u + sizeZ + sizeX + sizeZ, this.v + sizeZ + sizeY);
        this.faces[1] = new Quad(new Vertex[]{vertex, vertex5, vertex8, vertex4}, this.u + 0, this.v + sizeZ, this.u + sizeZ, this.v + sizeZ + sizeY);
        this.faces[2] = new Quad(new Vertex[]{vertex6, vertex5, vertex, vertex2}, this.u + sizeZ, this.v + 0, this.u + sizeZ + sizeX, this.v + sizeZ);
        this.faces[3] = new Quad(new Vertex[]{vertex3, vertex4, vertex8, vertex7}, this.u + sizeZ + sizeX, this.v + 0, this.u + sizeZ + sizeX + sizeX, this.v + sizeZ);
        this.faces[4] = new Quad(new Vertex[]{vertex2, vertex, vertex4, vertex3}, this.u + sizeZ, this.v + sizeZ, this.u + sizeZ + sizeX, this.v + sizeZ + sizeY);
        this.faces[5] = new Quad(new Vertex[]{vertex5, vertex6, vertex7, vertex8}, this.u + sizeZ + sizeX + sizeZ, this.v + sizeZ, this.u + sizeZ + sizeX + sizeZ + sizeX, this.v + sizeZ + sizeY);
        if (this.mirror) {
            for (int i = 0; i < this.faces.length; ++i) {
                this.faces[i].flip();
            }
        }
    }

    public void setPivot(float x, float y, float z) {
        this.pivotX = x;
        this.pivotY = y;
        this.pivotZ = z;
    }

    public void render(float scale) {
        if (this.hidden) {
            return;
        }
        if (!this.visible) {
            return;
        }
        if (!this.compiled) {
            this.compileList(scale);
        }
        if (this.pitch != 0.0f || this.yaw != 0.0f || this.roll != 0.0f) {
            GL11.glPushMatrix();
            GL11.glTranslatef(this.pivotX * scale, this.pivotY * scale, this.pivotZ * scale);
            if (this.roll != 0.0f) {
                GL11.glRotatef(this.roll * 57.295776f, 0.0f, 0.0f, 1.0f);
            }
            if (this.yaw != 0.0f) {
                GL11.glRotatef(this.yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
            }
            if (this.pitch != 0.0f) {
                GL11.glRotatef(this.pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
            }
            GL11.glCallList(this.list);
            GL11.glPopMatrix();
        } else if (this.pivotX != 0.0f || this.pivotY != 0.0f || this.pivotZ != 0.0f) {
            GL11.glTranslatef(this.pivotX * scale, this.pivotY * scale, this.pivotZ * scale);
            GL11.glCallList(this.list);
            GL11.glTranslatef(-this.pivotX * scale, -this.pivotY * scale, -this.pivotZ * scale);
        } else {
            GL11.glCallList(this.list);
        }
    }

    public void renderForceTransform(float scale) {
        if (this.hidden) {
            return;
        }
        if (!this.visible) {
            return;
        }
        if (!this.compiled) {
            this.compileList(scale);
        }
        GL11.glPushMatrix();
        GL11.glTranslatef(this.pivotX * scale, this.pivotY * scale, this.pivotZ * scale);
        if (this.yaw != 0.0f) {
            GL11.glRotatef(this.yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
        }
        if (this.pitch != 0.0f) {
            GL11.glRotatef(this.pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
        }
        if (this.roll != 0.0f) {
            GL11.glRotatef(this.roll * 57.295776f, 0.0f, 0.0f, 1.0f);
        }
        GL11.glCallList(this.list);
        GL11.glPopMatrix();
    }

    public void transform(float scale) {
        if (this.hidden) {
            return;
        }
        if (!this.visible) {
            return;
        }
        if (!this.compiled) {
            this.compileList(scale);
        }
        if (this.pitch != 0.0f || this.yaw != 0.0f || this.roll != 0.0f) {
            GL11.glTranslatef(this.pivotX * scale, this.pivotY * scale, this.pivotZ * scale);
            if (this.roll != 0.0f) {
                GL11.glRotatef(this.roll * 57.295776f, 0.0f, 0.0f, 1.0f);
            }
            if (this.yaw != 0.0f) {
                GL11.glRotatef(this.yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
            }
            if (this.pitch != 0.0f) {
                GL11.glRotatef(this.pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
            }
        } else if (this.pivotX != 0.0f || this.pivotY != 0.0f || this.pivotZ != 0.0f) {
            GL11.glTranslatef(this.pivotX * scale, this.pivotY * scale, this.pivotZ * scale);
        }
    }

    private void compileList(float scale) {
        this.list = GlAllocationUtils.generateDisplayLists(1);
        GL11.glNewList(this.list, 4864);
        Tessellator tessellator = Tessellator.INSTANCE;
        for (int i = 0; i < this.faces.length; ++i) {
            this.faces[i].render(tessellator, scale);
        }
        GL11.glEndList();
        this.compiled = true;
    }
}

