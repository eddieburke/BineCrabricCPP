/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.stat;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.stat.Stat;
import net.minecraft.stat.StatFormatter;

final class TimeStatFormatter
implements StatFormatter {
    TimeStatFormatter() {
    }

    @Environment(value=EnvType.CLIENT)
    public String format(int value) {
        double d = (double)value / 20.0;
        double d2 = d / 60.0;
        double d3 = d2 / 60.0;
        double d4 = d3 / 24.0;
        double d5 = d4 / 365.0;
        if (d5 > 0.5) {
            return Stat.getDecimalFormat().format(d5) + " y";
        }
        if (d4 > 0.5) {
            return Stat.getDecimalFormat().format(d4) + " d";
        }
        if (d3 > 0.5) {
            return Stat.getDecimalFormat().format(d3) + " h";
        }
        if (d2 > 0.5) {
            return Stat.getDecimalFormat().format(d2) + " m";
        }
        return d + " s";
    }
}

