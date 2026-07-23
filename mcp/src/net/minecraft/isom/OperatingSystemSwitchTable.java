/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.isom;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.isom.OS;

@Environment(value=EnvType.CLIENT)
class OperatingSystemSwitchTable {
    static final /* synthetic */ int[] VALUES;

    static {
        VALUES = new int[OS.values().length];
        try {
            OperatingSystemSwitchTable.VALUES[OS.LINUX.ordinal()] = 1;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OS.SOLARIS.ordinal()] = 2;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OS.WINDOWS.ordinal()] = 3;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OS.MACOS.ordinal()] = 4;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
    }
}

