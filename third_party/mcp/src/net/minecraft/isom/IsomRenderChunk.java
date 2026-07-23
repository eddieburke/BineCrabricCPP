/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.isom;

import java.awt.image.BufferedImage;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class IsomRenderChunk {
    public BufferedImage image;
    public World world;
    public int chunkX;
    public int chunkZ;
    public boolean rendered = false;
    public boolean empty = false;
    public int lastVisible = 0;
    public boolean toBeRendered = false;

    public IsomRenderChunk(World world, int chunkX, int chunkZ) {
        this.world = world;
        this.init(chunkX, chunkZ);
    }

    public void init(int chunkX, int chunkZ) {
        this.rendered = false;
        this.chunkX = chunkX;
        this.chunkZ = chunkZ;
        this.lastVisible = 0;
        this.toBeRendered = false;
    }

    public void init(World world, int chunkX, int chunkZ) {
        this.world = world;
        this.init(chunkX, chunkZ);
    }
}

