/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jorbis;

import com.jcraft.jogg.Buffer;
import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.DspState;
import com.jcraft.jorbis.FuncTime;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.InfoMode;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Time0
extends FuncTime {
    Time0() {
    }

    void pack(Object object, Buffer buffer) {
    }

    Object unpack(Info info, Buffer buffer) {
        return "";
    }

    Object look(DspState dspState, InfoMode infoMode, Object object) {
        return "";
    }

    void free_info(Object object) {
    }

    void free_look(Object object) {
    }

    int inverse(Block block, Object object, float[] fs, float[] gs) {
        return 0;
    }
}

