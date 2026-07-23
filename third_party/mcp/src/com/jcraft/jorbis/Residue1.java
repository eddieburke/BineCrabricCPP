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
class Residue1
extends Residue0 {
    Residue1() {
    }

    int inverse(Block block, Object object, float[][] fs, int[] is, int i) {
        int n = 0;
        for (int j = 0; j < i; ++j) {
            if (is[j] == 0) continue;
            fs[n++] = fs[j];
        }
        if (n != 0) {
            return Residue1._01inverse(block, object, fs, n, 1);
        }
        return 0;
    }
}

