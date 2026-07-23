/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.gen.feature;

import java.util.Random;
import net.minecraft.world.World;

public abstract class Feature {
    public abstract boolean generate(World var1, Random var2, int var3, int var4, int var5);

    public void prepare(double d0, double d1, double d2) {
    }
}

