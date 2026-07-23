/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.color.world;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class GrassColors {
    private static int[] colorMap = new int[65536];

    public static void setColorMap(int[] colorMap) {
        GrassColors.colorMap = colorMap;
    }

    public static int getColor(double temperature, double humidity) {
        int n = (int)((1.0 - temperature) * 255.0);
        int n2 = (int)((1.0 - (humidity *= temperature)) * 255.0);
        return colorMap[n2 << 8 | n];
    }
}

