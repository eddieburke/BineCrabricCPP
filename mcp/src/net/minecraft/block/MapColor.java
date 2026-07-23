/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

public class MapColor {
    public static final MapColor[] COLORS = new MapColor[16];
    public static final MapColor CLEAR = new MapColor(0, 0);
    public static final MapColor PALE_GREEN = new MapColor(1, 8368696);
    public static final MapColor PALE_YELLOW = new MapColor(2, 16247203);
    public static final MapColor LIGHT_GRAY = new MapColor(3, 0xA7A7A7);
    public static final MapColor RED = new MapColor(4, 0xFF0000);
    public static final MapColor LIGHT_BLUE = new MapColor(5, 0xA0A0FF);
    public static final MapColor LIGHT_GRAY2 = new MapColor(6, 0xA7A7A7);
    public static final MapColor GREEN = new MapColor(7, 31744);
    public static final MapColor WHITE = new MapColor(8, 0xFFFFFF);
    public static final MapColor SILVER = new MapColor(9, 10791096);
    public static final MapColor ORANGE = new MapColor(10, 12020271);
    public static final MapColor GRAY = new MapColor(11, 0x707070);
    public static final MapColor BLUE = new MapColor(12, 0x4040FF);
    public static final MapColor BROWN = new MapColor(13, 6837042);
    public final int color;
    public final int id;

    private MapColor(int id, int color) {
        this.id = id;
        this.color = color;
        MapColor.COLORS[id] = this;
    }
}

