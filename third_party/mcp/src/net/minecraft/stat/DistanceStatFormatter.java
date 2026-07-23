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

final class DistanceStatFormatter
implements StatFormatter {
    DistanceStatFormatter() {
    }

    @Environment(value=EnvType.CLIENT)
    public String format(int value) {
        int n = value;
        double d = (double)n / 100.0;
        double d2 = d / 1000.0;
        if (d2 > 0.5) {
            return Stat.getDecimalFormat().format(d2) + " km";
        }
        if (d > 0.5) {
            return Stat.getDecimalFormat().format(d) + " m";
        }
        return value + " cm";
    }
}

