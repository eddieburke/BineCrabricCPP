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
import net.minecraft.entity.LivingEntity;

@Environment(value=EnvType.CLIENT)
public class DirtyChunkSorter
implements Comparator {
    private LivingEntity camera;

    public DirtyChunkSorter(LivingEntity camera) {
        this.camera = camera;
    }

    public void update(LivingEntity camera) {
        this.camera = camera;
    }

    public int compare(ChunkBuilder arg, ChunkBuilder arg2) {
        double d;
        boolean bl = arg.inFrustum;
        boolean bl2 = arg2.inFrustum;
        if (bl && !bl2) {
            return 1;
        }
        if (bl2 && !bl) {
            return -1;
        }
        double d2 = arg.squaredDistanceTo(this.camera);
        if (d2 < (d = (double)arg2.squaredDistanceTo(this.camera))) {
            return 1;
        }
        if (d2 > d) {
            return -1;
        }
        return arg.id < arg2.id ? 1 : -1;
    }
}

