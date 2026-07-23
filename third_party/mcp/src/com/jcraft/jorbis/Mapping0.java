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
import com.jcraft.jorbis.FuncFloor;
import com.jcraft.jorbis.FuncMapping;
import com.jcraft.jorbis.FuncResidue;
import com.jcraft.jorbis.FuncTime;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.InfoMode;
import com.jcraft.jorbis.Mdct;
import com.jcraft.jorbis.PsyLook;
import com.jcraft.jorbis.Util;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
class Mapping0
extends FuncMapping {
    static int seq = 0;
    float[][] pcmbundle = null;
    int[] zerobundle = null;
    int[] nonzero = null;
    Object[] floormemo = null;

    Mapping0() {
    }

    void free_info(Object object) {
    }

    void free_look(Object object) {
    }

    Object look(DspState dspState, InfoMode infoMode, Object object) {
        Info info = dspState.vi;
        LookMapping0 lookMapping0 = new LookMapping0();
        InfoMapping0 infoMapping0 = lookMapping0.map = (InfoMapping0)object;
        lookMapping0.mode = infoMode;
        lookMapping0.time_look = new Object[infoMapping0.submaps];
        lookMapping0.floor_look = new Object[infoMapping0.submaps];
        lookMapping0.residue_look = new Object[infoMapping0.submaps];
        lookMapping0.time_func = new FuncTime[infoMapping0.submaps];
        lookMapping0.floor_func = new FuncFloor[infoMapping0.submaps];
        lookMapping0.residue_func = new FuncResidue[infoMapping0.submaps];
        for (int i = 0; i < infoMapping0.submaps; ++i) {
            int n = infoMapping0.timesubmap[i];
            int n2 = infoMapping0.floorsubmap[i];
            int n3 = infoMapping0.residuesubmap[i];
            lookMapping0.time_func[i] = FuncTime.time_P[info.time_type[n]];
            lookMapping0.time_look[i] = lookMapping0.time_func[i].look(dspState, infoMode, info.time_param[n]);
            lookMapping0.floor_func[i] = FuncFloor.floor_P[info.floor_type[n2]];
            lookMapping0.floor_look[i] = lookMapping0.floor_func[i].look(dspState, infoMode, info.floor_param[n2]);
            lookMapping0.residue_func[i] = FuncResidue.residue_P[info.residue_type[n3]];
            lookMapping0.residue_look[i] = lookMapping0.residue_func[i].look(dspState, infoMode, info.residue_param[n3]);
        }
        if (info.psys == 0 || dspState.analysisp != 0) {
            // empty if block
        }
        lookMapping0.ch = info.channels;
        return lookMapping0;
    }

    void pack(Info info, Object object, Buffer buffer) {
        int n;
        InfoMapping0 infoMapping0 = (InfoMapping0)object;
        if (infoMapping0.submaps > 1) {
            buffer.write(1, 1);
            buffer.write(infoMapping0.submaps - 1, 4);
        } else {
            buffer.write(0, 1);
        }
        if (infoMapping0.coupling_steps > 0) {
            buffer.write(1, 1);
            buffer.write(infoMapping0.coupling_steps - 1, 8);
            for (n = 0; n < infoMapping0.coupling_steps; ++n) {
                buffer.write(infoMapping0.coupling_mag[n], Util.ilog2(info.channels));
                buffer.write(infoMapping0.coupling_ang[n], Util.ilog2(info.channels));
            }
        } else {
            buffer.write(0, 1);
        }
        buffer.write(0, 2);
        if (infoMapping0.submaps > 1) {
            for (n = 0; n < info.channels; ++n) {
                buffer.write(infoMapping0.chmuxlist[n], 4);
            }
        }
        for (n = 0; n < infoMapping0.submaps; ++n) {
            buffer.write(infoMapping0.timesubmap[n], 8);
            buffer.write(infoMapping0.floorsubmap[n], 8);
            buffer.write(infoMapping0.residuesubmap[n], 8);
        }
    }

    Object unpack(Info info, Buffer buffer) {
        int n;
        InfoMapping0 infoMapping0 = new InfoMapping0();
        infoMapping0.submaps = buffer.read(1) != 0 ? buffer.read(4) + 1 : 1;
        if (buffer.read(1) != 0) {
            infoMapping0.coupling_steps = buffer.read(8) + 1;
            for (n = 0; n < infoMapping0.coupling_steps; ++n) {
                int n2 = infoMapping0.coupling_mag[n] = buffer.read(Util.ilog2(info.channels));
                int n3 = infoMapping0.coupling_ang[n] = buffer.read(Util.ilog2(info.channels));
                if (n2 >= 0 && n3 >= 0 && n2 != n3 && n2 < info.channels && n3 < info.channels) continue;
                infoMapping0.free();
                return null;
            }
        }
        if (buffer.read(2) > 0) {
            infoMapping0.free();
            return null;
        }
        if (infoMapping0.submaps > 1) {
            for (n = 0; n < info.channels; ++n) {
                infoMapping0.chmuxlist[n] = buffer.read(4);
                if (infoMapping0.chmuxlist[n] < infoMapping0.submaps) continue;
                infoMapping0.free();
                return null;
            }
        }
        for (n = 0; n < infoMapping0.submaps; ++n) {
            infoMapping0.timesubmap[n] = buffer.read(8);
            if (infoMapping0.timesubmap[n] >= info.times) {
                infoMapping0.free();
                return null;
            }
            infoMapping0.floorsubmap[n] = buffer.read(8);
            if (infoMapping0.floorsubmap[n] >= info.floors) {
                infoMapping0.free();
                return null;
            }
            infoMapping0.residuesubmap[n] = buffer.read(8);
            if (infoMapping0.residuesubmap[n] < info.residues) continue;
            infoMapping0.free();
            return null;
        }
        return infoMapping0;
    }

    synchronized int inverse(Block block, Object object) {
        int n;
        int n2;
        int n3;
        DspState dspState = block.vd;
        Info info = dspState.vi;
        LookMapping0 lookMapping0 = (LookMapping0)object;
        InfoMapping0 infoMapping0 = lookMapping0.map;
        InfoMode infoMode = lookMapping0.mode;
        int n4 = block.pcmend = info.blocksizes[block.W];
        float[] fArray = dspState.window[block.W][block.lW][block.nW][infoMode.windowtype];
        if (this.pcmbundle == null || this.pcmbundle.length < info.channels) {
            this.pcmbundle = new float[info.channels][];
            this.nonzero = new int[info.channels];
            this.zerobundle = new int[info.channels];
            this.floormemo = new Object[info.channels];
        }
        for (n3 = 0; n3 < info.channels; ++n3) {
            float[] fArray2 = block.pcm[n3];
            n2 = infoMapping0.chmuxlist[n3];
            this.floormemo[n3] = lookMapping0.floor_func[n2].inverse1(block, lookMapping0.floor_look[n2], this.floormemo[n3]);
            this.nonzero[n3] = this.floormemo[n3] != null ? 1 : 0;
            for (n = 0; n < n4 / 2; ++n) {
                fArray2[n] = 0.0f;
            }
        }
        for (n3 = 0; n3 < infoMapping0.coupling_steps; ++n3) {
            if (this.nonzero[infoMapping0.coupling_mag[n3]] == 0 && this.nonzero[infoMapping0.coupling_ang[n3]] == 0) continue;
            this.nonzero[infoMapping0.coupling_mag[n3]] = 1;
            this.nonzero[infoMapping0.coupling_ang[n3]] = 1;
        }
        for (n3 = 0; n3 < infoMapping0.submaps; ++n3) {
            int n5 = 0;
            for (n2 = 0; n2 < info.channels; ++n2) {
                if (infoMapping0.chmuxlist[n2] != n3) continue;
                this.zerobundle[n5] = this.nonzero[n2] != 0 ? 1 : 0;
                this.pcmbundle[n5++] = block.pcm[n2];
            }
            lookMapping0.residue_func[n3].inverse(block, lookMapping0.residue_look[n3], this.pcmbundle, this.zerobundle, n5);
        }
        for (n3 = infoMapping0.coupling_steps - 1; n3 >= 0; --n3) {
            float[] fArray3 = block.pcm[infoMapping0.coupling_mag[n3]];
            float[] fArray4 = block.pcm[infoMapping0.coupling_ang[n3]];
            for (n = 0; n < n4 / 2; ++n) {
                float f = fArray3[n];
                float f2 = fArray4[n];
                if (f > 0.0f) {
                    if (f2 > 0.0f) {
                        fArray3[n] = f;
                        fArray4[n] = f - f2;
                        continue;
                    }
                    fArray4[n] = f;
                    fArray3[n] = f + f2;
                    continue;
                }
                if (f2 > 0.0f) {
                    fArray3[n] = f;
                    fArray4[n] = f + f2;
                    continue;
                }
                fArray4[n] = f;
                fArray3[n] = f - f2;
            }
        }
        for (n3 = 0; n3 < info.channels; ++n3) {
            float[] fArray5 = block.pcm[n3];
            int n6 = infoMapping0.chmuxlist[n3];
            lookMapping0.floor_func[n6].inverse2(block, lookMapping0.floor_look[n6], this.floormemo[n3], fArray5);
        }
        for (n3 = 0; n3 < info.channels; ++n3) {
            float[] fArray6 = block.pcm[n3];
            ((Mdct)dspState.transform[block.W][0]).backward(fArray6, fArray6);
        }
        for (n3 = 0; n3 < info.channels; ++n3) {
            int n7;
            float[] fArray7 = block.pcm[n3];
            if (this.nonzero[n3] != 0) {
                for (n7 = 0; n7 < n4; ++n7) {
                    int n8 = n7;
                    fArray7[n8] = fArray7[n8] * fArray[n7];
                }
                continue;
            }
            for (n7 = 0; n7 < n4; ++n7) {
                fArray7[n7] = 0.0f;
            }
        }
        return 0;
    }

    @Environment(value=EnvType.CLIENT)
    class InfoMapping0 {
        int submaps;
        int[] chmuxlist = new int[256];
        int[] timesubmap = new int[16];
        int[] floorsubmap = new int[16];
        int[] residuesubmap = new int[16];
        int[] psysubmap = new int[16];
        int coupling_steps;
        int[] coupling_mag = new int[256];
        int[] coupling_ang = new int[256];

        InfoMapping0() {
        }

        void free() {
            this.chmuxlist = null;
            this.timesubmap = null;
            this.floorsubmap = null;
            this.residuesubmap = null;
            this.psysubmap = null;
            this.coupling_mag = null;
            this.coupling_ang = null;
        }
    }

    @Environment(value=EnvType.CLIENT)
    class LookMapping0 {
        InfoMode mode;
        InfoMapping0 map;
        Object[] time_look;
        Object[] floor_look;
        Object[] floor_state;
        Object[] residue_look;
        PsyLook[] psy_look;
        FuncTime[] time_func;
        FuncFloor[] floor_func;
        FuncResidue[] residue_func;
        int ch;
        float[][] decay;
        int lastframe;

        LookMapping0() {
        }
    }
}

