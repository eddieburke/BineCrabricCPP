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
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.InfoMode;
import com.jcraft.jorbis.Residue0;
import com.jcraft.jorbis.Residue1;
import com.jcraft.jorbis.Residue2;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
abstract class FuncResidue {
    public static FuncResidue[] residue_P = new FuncResidue[]{new Residue0(), new Residue1(), new Residue2()};

    FuncResidue() {
    }

    abstract void pack(Object var1, Buffer var2);

    abstract Object unpack(Info var1, Buffer var2);

    abstract Object look(DspState var1, InfoMode var2, Object var3);

    abstract void free_info(Object var1);

    abstract void free_look(Object var1);

    abstract int inverse(Block var1, Object var2, float[][] var3, int[] var4, int var5);
}

