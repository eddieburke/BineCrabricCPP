/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.util.math.Box;

@Environment(value=EnvType.CLIENT)
public interface Culler {
    public boolean isVisible(Box var1);

    public void prepare(double var1, double var3, double var5);
}

