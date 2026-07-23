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
import net.minecraft.client.model.Vertex;
import net.minecraft.client.render.Tessellator;
import net.minecraft.util.math.Vec3d;

@Environment(value=EnvType.CLIENT)
public class Quad {
    public Vertex[] vertices;
    public int verticesCount = 0;
    private boolean flipNormal = false;

    public Quad(Vertex[] vertices) {
        this.vertices = vertices;
        this.verticesCount = vertices.length;
    }

    public Quad(Vertex[] vertices, int u1, int v1, int u2, int v2) {
        this(vertices);
        float f = 0.0015625f;
        float f2 = 0.003125f;
        vertices[0] = vertices[0].remap((float)u2 / 64.0f - f, (float)v1 / 32.0f + f2);
        vertices[1] = vertices[1].remap((float)u1 / 64.0f + f, (float)v1 / 32.0f + f2);
        vertices[2] = vertices[2].remap((float)u1 / 64.0f + f, (float)v2 / 32.0f - f2);
        vertices[3] = vertices[3].remap((float)u2 / 64.0f - f, (float)v2 / 32.0f - f2);
    }

    public void flip() {
        Vertex[] vertexArray = new Vertex[this.vertices.length];
        for (int i = 0; i < this.vertices.length; ++i) {
            vertexArray[i] = this.vertices[this.vertices.length - i - 1];
        }
        this.vertices = vertexArray;
    }

    public void render(Tessellator tessellator, float f) {
        Vec3d vec3d = this.vertices[1].pos.relativize(this.vertices[0].pos);
        Vec3d vec3d2 = this.vertices[1].pos.relativize(this.vertices[2].pos);
        Vec3d vec3d3 = vec3d2.crossProduct(vec3d).normalize();
        tessellator.startQuads();
        if (this.flipNormal) {
            tessellator.normal(-((float)vec3d3.x), -((float)vec3d3.y), -((float)vec3d3.z));
        } else {
            tessellator.normal((float)vec3d3.x, (float)vec3d3.y, (float)vec3d3.z);
        }
        for (int i = 0; i < 4; ++i) {
            Vertex vertex = this.vertices[i];
            tessellator.vertex((float)vertex.pos.x * f, (float)vertex.pos.y * f, (float)vertex.pos.z * f, vertex.u, vertex.v);
        }
        tessellator.draw();
    }
}

