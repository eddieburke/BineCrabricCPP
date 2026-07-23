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

final class IntegerStatFormatter
implements StatFormatter {
    IntegerStatFormatter() {
    }

    @Environment(value=EnvType.CLIENT)
    public String format(int value) {
        return Stat.getNumberFormat().format(value);
    }
}

