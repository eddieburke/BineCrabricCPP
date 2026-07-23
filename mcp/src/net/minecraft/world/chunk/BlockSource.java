/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.chunk;

import net.minecraft.block.Block;

public class BlockSource {
    private static byte[] BLOCKS = new byte[256];

    public static void fill(byte[] blocks) {
        for (int i = 0; i < blocks.length; ++i) {
            blocks[i] = BLOCKS[blocks[i] & 0xFF];
        }
    }

    static {
        try {
            for (int i = 0; i < 256; ++i) {
                byte by = (byte)i;
                if (by != 0 && Block.BLOCKS[by & 0xFF] == null) {
                    by = 0;
                }
                BlockSource.BLOCKS[i] = by;
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }
}

