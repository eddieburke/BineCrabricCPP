/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.decoration.painting;

public enum PaintingVariants {
    KEBAB("Kebab", 16, 16, 0, 0),
    AZTEC("Aztec", 16, 16, 16, 0),
    ALBAN("Alban", 16, 16, 32, 0),
    AZTEC2("Aztec2", 16, 16, 48, 0),
    BOMB("Bomb", 16, 16, 64, 0),
    PLANT("Plant", 16, 16, 80, 0),
    WASTELAND("Wasteland", 16, 16, 96, 0),
    POOL("Pool", 32, 16, 0, 32),
    COURBET("Courbet", 32, 16, 32, 32),
    SEA("Sea", 32, 16, 64, 32),
    SUNSET("Sunset", 32, 16, 96, 32),
    CREEBET("Creebet", 32, 16, 128, 32),
    WANDERER("Wanderer", 16, 32, 0, 64),
    GRAHAM("Graham", 16, 32, 16, 64),
    MATCH("Match", 32, 32, 0, 128),
    BUST("Bust", 32, 32, 32, 128),
    STAGE("Stage", 32, 32, 64, 128),
    VOID("Void", 32, 32, 96, 128),
    SKULL_AND_ROSES("SkullAndRoses", 32, 32, 128, 128),
    FIGHTERS("Fighters", 64, 32, 0, 96),
    POINTER("Pointer", 64, 64, 0, 192),
    PIGSCENE("Pigscene", 64, 64, 64, 192),
    BURNING_SKULL("BurningSkull", 64, 64, 128, 192),
    SKELETON("Skeleton", 64, 48, 192, 64),
    DONKEY_KONG("DonkeyKong", 64, 48, 192, 112);

    public static final int LONGEST_NAME_LENGTH;
    public final String id;
    public final int width;
    public final int height;
    public final int textureOffsetX;
    public final int textureOffsetY;

    /*
     * WARNING - Possible parameter corruption
     * WARNING - void declaration
     */
    private PaintingVariants(int id, int width, int height) {
        void textureOffsetY;
        void textureOffsetX;
        this.id = (String)id;
        this.width = width;
        this.height = height;
        this.textureOffsetX = textureOffsetX;
        this.textureOffsetY = textureOffsetY;
    }

    static {
        LONGEST_NAME_LENGTH = "SkullAndRoses".length();
    }
}

