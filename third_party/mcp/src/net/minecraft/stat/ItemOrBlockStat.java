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

public class ItemOrBlockStat
extends Stat {
    private final int itemOrBlockId;

    public ItemOrBlockStat(int statId, String translationKey, int itemIdOrBlockId) {
        super(statId, translationKey);
        this.itemOrBlockId = itemIdOrBlockId;
    }

    @Environment(value=EnvType.CLIENT)
    public int getItemOrBlockId() {
        return this.itemOrBlockId;
    }
}

