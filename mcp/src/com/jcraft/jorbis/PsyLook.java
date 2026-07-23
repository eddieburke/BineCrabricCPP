/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jorbis.PsyInfo;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class PsyLook {
    int n;
    PsyInfo vi;
    float[][][] tonecurves;
    float[][] peakatt;
    float[][][] noisecurves;
    float[] ath;
    int[] octave;

    PsyLook() {
    }

    void init(PsyInfo psyInfo, int i, int j) {
    }
}

