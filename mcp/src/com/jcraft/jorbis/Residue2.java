/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.Residue0;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Residue2
extends Residue0 {
    Residue2() {
    }

    int inverse(Block block, Object object, float[][] fs, int[] is, int i) {
        int n = 0;
        for (n = 0; n < i && is[n] == 0; ++n) {
        }
        if (n == i) {
            return 0;
        }
        return Residue2._2inverse(block, object, fs, i);
    }
}

