/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.stat.achievement;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

public class AchievementMap {
    public static AchievementMap INSTANCE = new AchievementMap();
    private Map uuids = new HashMap();

    private AchievementMap() {
        try {
            String string;
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(AchievementMap.class.getResourceAsStream("/achievement/map.txt")));
            while ((string = bufferedReader.readLine()) != null) {
                String[] stringArray = string.split(",");
                int n = Integer.parseInt(stringArray[0]);
                this.uuids.put(n, stringArray[1]);
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public static String getUuid(int id) {
        return (String)AchievementMap.INSTANCE.uuids.get(id);
    }
}

