/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.stat;

import net.minecraft.stat.Stat;
import net.minecraft.stat.StatFormatter;
import net.minecraft.stat.Stats;

public class SimpleStat
extends Stat {
    public SimpleStat(int i, String string, StatFormatter arg) {
        super(i, string, arg);
    }

    public SimpleStat(int i, String string) {
        super(i, string);
    }

    public Stat addStat() {
        super.addStat();
        Stats.GENERAL_STATS.add(this);
        return this;
    }
}

