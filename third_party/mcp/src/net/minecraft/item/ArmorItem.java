/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.item.Item;

public class ArmorItem
extends Item {
    private static final int[] PROTECTION_BY_SLOT = new int[]{3, 8, 6, 3};
    private static final int[] DURABILITY_BY_SLOT = new int[]{11, 16, 15, 13};
    public final int type;
    public final int equipmentSlot;
    public final int maxProtection;
    public final int textureIndex;

    public ArmorItem(int id, int type, int textureIndex, int equipmentSlot) {
        super(id);
        this.type = type;
        this.equipmentSlot = equipmentSlot;
        this.textureIndex = textureIndex;
        this.maxProtection = PROTECTION_BY_SLOT[equipmentSlot];
        this.setMaxDamage(DURABILITY_BY_SLOT[equipmentSlot] * 3 << type);
        this.maxCount = 1;
    }
}

