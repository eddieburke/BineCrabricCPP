/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.util;

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class CharacterUtils {
    public static final String VALID_CHARACTERS = CharacterUtils.loadValidCharacters();
    public static final char[] INVALID_CHARS_WORLD_NAME = new char[]{'/', '\n', '\r', '\t', '\u0000', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'};

    private static String loadValidCharacters() {
        String string = "";
        try {
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(CharacterUtils.class.getResourceAsStream("/font.txt"), "UTF-8"));
            String string2 = "";
            while ((string2 = bufferedReader.readLine()) != null) {
                if (string2.startsWith("#")) continue;
                string = string + string2;
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            // empty catch block
        }
        return string;
    }
}

