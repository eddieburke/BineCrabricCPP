/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.OperatingSystem;

@Environment(value=EnvType.CLIENT)
public class OperatingSystemSwitchTable {
    public static final /* synthetic */ int[] VALUES;

    static {
        VALUES = new int[OperatingSystem.values().length];
        try {
            OperatingSystemSwitchTable.VALUES[OperatingSystem.LINUX.ordinal()] = 1;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OperatingSystem.SOLARIS.ordinal()] = 2;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OperatingSystem.WINDOWS.ordinal()] = 3;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
        try {
            OperatingSystemSwitchTable.VALUES[OperatingSystem.MACOS.ordinal()] = 4;
        }
        catch (NoSuchFieldError noSuchFieldError) {
            // empty catch block
        }
    }
}

