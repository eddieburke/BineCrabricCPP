/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.stat;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Locale;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.stat.DistanceStatFormatter;
import net.minecraft.stat.IntegerStatFormatter;
import net.minecraft.stat.StatFormatter;
import net.minecraft.stat.Stats;
import net.minecraft.stat.TimeStatFormatter;
import net.minecraft.stat.achievement.AchievementMap;

public class Stat {
    public final int id;
    public final String stringId;
    public boolean localOnly = false;
    public String uuid;
    private final StatFormatter formatter;
    private static NumberFormat DEFAULT_NUMBER_FORMAT = NumberFormat.getIntegerInstance(Locale.US);
    public static StatFormatter INTEGER_FORMAT = new IntegerStatFormatter();
    private static DecimalFormat DEFAULT_DECIMAL_FORMAT = new DecimalFormat("########0.00");
    public static StatFormatter TIME_PROVIDER = new TimeStatFormatter();
    public static StatFormatter DISTANCE_PROVIDER = new DistanceStatFormatter();

    public Stat(int id, String stringId, StatFormatter statTypeProvider) {
        this.id = id;
        this.stringId = stringId;
        this.formatter = statTypeProvider;
    }

    public Stat(int id, String stringId) {
        this(id, stringId, INTEGER_FORMAT);
    }

    public Stat localOnly() {
        this.localOnly = true;
        return this;
    }

    public Stat addStat() {
        if (Stats.ID_TO_STAT.containsKey(this.id)) {
            throw new RuntimeException("Duplicate stat id: \"" + ((Stat)Stats.ID_TO_STAT.get((Object)Integer.valueOf((int)this.id))).stringId + "\" and \"" + this.stringId + "\" at id " + this.id);
        }
        Stats.ALL_STATS.add(this);
        Stats.ID_TO_STAT.put(this.id, this);
        this.uuid = AchievementMap.getUuid(this.id);
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isAchievement() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public String format(int value) {
        return this.formatter.format(value);
    }

    public String toString() {
        return this.stringId;
    }

    @Environment(value=EnvType.CLIENT)
    static /* synthetic */ NumberFormat getNumberFormat() {
        return DEFAULT_NUMBER_FORMAT;
    }

    @Environment(value=EnvType.CLIENT)
    static /* synthetic */ DecimalFormat getDecimalFormat() {
        return DEFAULT_DECIMAL_FORMAT;
    }
}

