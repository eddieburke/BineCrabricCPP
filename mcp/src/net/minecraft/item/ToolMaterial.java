/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

public enum ToolMaterial {
    WOOD(0, 59, 2.0f, 0),
    STONE(1, 131, 4.0f, 1),
    IRON(2, 250, 6.0f, 2),
    DIAMOND(3, 1561, 8.0f, 3),
    GOLD(0, 32, 12.0f, 0);

    private final int miningLevel;
    private final int itemDurability;
    private final float miningSpeed;
    private final int attackDamage;

    /*
     * WARNING - Possible parameter corruption
     * WARNING - void declaration
     */
    private ToolMaterial(float miningLevel, int itemDurability) {
        void attackDamage;
        void miningSpeed;
        this.miningLevel = (int)miningLevel;
        this.itemDurability = itemDurability;
        this.miningSpeed = miningSpeed;
        this.attackDamage = attackDamage;
    }

    public int getDurability() {
        return this.itemDurability;
    }

    public float getMiningSpeedMultiplier() {
        return this.miningSpeed;
    }

    public int getAttackDamage() {
        return this.attackDamage;
    }

    public int getMiningLevel() {
        return this.miningLevel;
    }
}

