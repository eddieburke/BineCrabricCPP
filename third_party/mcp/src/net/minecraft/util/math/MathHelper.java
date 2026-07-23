/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.util.math;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

public class MathHelper {
    private static float[] SINE_TABLE = new float[65536];

    public static final float sin(float value) {
        return SINE_TABLE[(int)(value * 10430.378f) & 0xFFFF];
    }

    public static final float cos(float value) {
        return SINE_TABLE[(int)(value * 10430.378f + 16384.0f) & 0xFFFF];
    }

    public static final float sqrt(float value) {
        return (float)Math.sqrt(value);
    }

    public static final float sqrt(double value) {
        return (float)Math.sqrt(value);
    }

    public static int floor(float value) {
        int n = (int)value;
        return value < (float)n ? n - 1 : n;
    }

    public static int floor(double value) {
        int n = (int)value;
        return value < (double)n ? n - 1 : n;
    }

    public static float abs(float value) {
        return value >= 0.0f ? value : -value;
    }

    public static double absMax(double a, double b) {
        if (a < 0.0) {
            a = -a;
        }
        if (b < 0.0) {
            b = -b;
        }
        return a > b ? a : b;
    }

    @Environment(value=EnvType.CLIENT)
    public static int floorDiv(int dividend, int divisor) {
        if (dividend < 0) {
            return -((-dividend - 1) / divisor) - 1;
        }
        return dividend / divisor;
    }

    @Environment(value=EnvType.CLIENT)
    public static boolean isNullOrEmpty(String text) {
        return text == null || text.length() == 0;
    }

    static {
        for (int i = 0; i < 65536; ++i) {
            MathHelper.SINE_TABLE[i] = (float)Math.sin((double)i * Math.PI * 2.0 / 65536.0);
        }
    }
}

