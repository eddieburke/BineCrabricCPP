/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.world;

import java.util.Comparator;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.render.chunk.ChunkBuilder;
import net.minecraft.entity.Entity;

@Environment(value=EnvType.CLIENT)
public class DistanceChunkSorter
implements Comparator {
    private double cameraX;
    private double cameraY;
    private double cameraZ;

    public DistanceChunkSorter(Entity camera) {
        this.cameraX = -camera.x;
        this.cameraY = -camera.y;
        this.cameraZ = -camera.z;
    }

    public void update(Entity camera) {
        this.cameraX = -camera.x;
        this.cameraY = -camera.y;
        this.cameraZ = -camera.z;
    }

    public int compare(ChunkBuilder arg, ChunkBuilder arg2) {
        double d = (double)arg.centerX + this.cameraX;
        double d2 = (double)arg.centerY + this.cameraY;
        double d3 = (double)arg.centerZ + this.cameraZ;
        double d4 = (double)arg2.centerX + this.cameraX;
        double d5 = (double)arg2.centerY + this.cameraY;
        double d6 = (double)arg2.centerZ + this.cameraZ;
        return (int)((d * d + d2 * d2 + d3 * d3 - (d4 * d4 + d5 * d5 + d6 * d6)) * 1024.0);
    }
}

