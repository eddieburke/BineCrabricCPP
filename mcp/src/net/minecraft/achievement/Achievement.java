/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.achievement;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.stat.Stat;
import net.minecraft.stat.achievement.AchievementStatFormatter;

public class Achievement
extends Stat {
    public final int column;
    public final int row;
    public final Achievement parent;
    private final String translationKey;
    @Environment(value=EnvType.CLIENT)
    private AchievementStatFormatter translationHelper;
    public final ItemStack icon;
    private boolean isChallenge;

    public Achievement(int id, String key, int column, int row, Item displayItem, Achievement parent) {
        this(id, key, column, row, new ItemStack(displayItem), parent);
    }

    public Achievement(int id, String key, int column, int row, Block displayBlock, Achievement parent) {
        this(id, key, column, row, new ItemStack(displayBlock), parent);
    }

    public Achievement(int id, String key, int column, int row, ItemStack icon, Achievement parent) {
        super(0x500000 + id, I18n.getTranslation("achievement." + key));
        this.icon = icon;
        this.translationKey = I18n.getTranslation("achievement." + key + ".desc");
        this.column = column;
        this.row = row;
        if (column < Achievements.minColumn) {
            Achievements.minColumn = column;
        }
        if (row < Achievements.minRow) {
            Achievements.minRow = row;
        }
        if (column > Achievements.maxColumn) {
            Achievements.maxColumn = column;
        }
        if (row > Achievements.maxRow) {
            Achievements.maxRow = row;
        }
        this.parent = parent;
    }

    public Achievement localOnly() {
        this.localOnly = true;
        return this;
    }

    public Achievement challenge() {
        this.isChallenge = true;
        return this;
    }

    public Achievement addStat() {
        super.addStat();
        Achievements.ACHIEVEMENTS.add(this);
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isAchievement() {
        return true;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslatedDescription() {
        if (this.translationHelper != null) {
            return this.translationHelper.format(this.translationKey);
        }
        return this.translationKey;
    }

    @Environment(value=EnvType.CLIENT)
    public Achievement setTranslationHelper(AchievementStatFormatter translationHelper) {
        this.translationHelper = translationHelper;
        return this;
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isChallenge() {
        return this.isChallenge;
    }
}

