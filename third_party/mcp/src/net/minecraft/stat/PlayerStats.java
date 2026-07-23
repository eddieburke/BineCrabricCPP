/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.stat;

import argo.jdom.JdomParser;
import argo.jdom.JsonNode;
import argo.jdom.JsonRootNode;
import argo.jdom.JsonStringNode;
import argo.saj.InvalidSyntaxException;
import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievement;
import net.minecraft.client.util.Session;
import net.minecraft.stat.Stat;
import net.minecraft.stat.Stats;
import net.minecraft.stat.StatsSyncer;
import net.minecraft.util.MD5MessageDigest;

@Environment(value=EnvType.CLIENT)
public class PlayerStats {
    private Map stats = new HashMap();
    private Map unsentStats = new HashMap();
    private boolean dirty = false;
    private StatsSyncer syncer;

    public PlayerStats(Session session, File parentDir) {
        File file = new File(parentDir, "stats");
        if (!file.exists()) {
            file.mkdir();
        }
        for (File file2 : parentDir.listFiles()) {
            File file3;
            if (!file2.getName().startsWith("stats_") || !file2.getName().endsWith(".dat") || (file3 = new File(file, file2.getName())).exists()) continue;
            System.out.println("Relocating " + file2.getName());
            file2.renameTo(file3);
        }
        this.syncer = new StatsSyncer(session, this, file);
    }

    public void increment(Stat stat, int amount) {
        this.doIncrement(this.unsentStats, stat, amount);
        this.doIncrement(this.stats, stat, amount);
        this.dirty = true;
    }

    private void doIncrement(Map stats, Stat stat, int amount) {
        Integer n = (Integer)stats.get(stat);
        int n2 = n == null ? 0 : n;
        stats.put(stat, n2 + amount);
    }

    public Map doIncrement() {
        return new HashMap(this.unsentStats);
    }

    public void increment(Map amounts) {
        if (amounts == null) {
            return;
        }
        this.dirty = true;
        for (Stat stat : amounts.keySet()) {
            this.doIncrement(this.unsentStats, stat, (Integer)amounts.get(stat));
            this.doIncrement(this.stats, stat, (Integer)amounts.get(stat));
        }
    }

    public void tick(Map stats) {
        if (stats == null) {
            return;
        }
        for (Stat stat : stats.keySet()) {
            Integer n = (Integer)this.unsentStats.get(stat);
            int n2 = n == null ? 0 : n;
            this.stats.put(stat, (Integer)stats.get(stat) + n2);
        }
    }

    public void tickUnsent(Map stats) {
        if (stats == null) {
            return;
        }
        this.dirty = true;
        for (Stat stat : stats.keySet()) {
            this.doIncrement(this.unsentStats, stat, (Integer)stats.get(stat));
        }
    }

    public static Map deserialize(String data) {
        HashMap<Stat, Integer> hashMap = new HashMap<Stat, Integer>();
        try {
            Object object2;
            String string = "local";
            StringBuilder stringBuilder = new StringBuilder();
            JsonRootNode jsonRootNode = new JdomParser().parse(data);
            List list = jsonRootNode.getArrayNode("stats-change");
            for (Object object2 : list) {
                Map map = ((JsonNode)object2).getFields();
                Map.Entry entry = map.entrySet().iterator().next();
                int n = Integer.parseInt(((JsonStringNode)entry.getKey()).getText());
                int n2 = Integer.parseInt(((JsonNode)entry.getValue()).getText());
                Stat stat = Stats.getStatById(n);
                if (stat == null) {
                    System.out.println(n + " is not a valid stat");
                    continue;
                }
                stringBuilder.append(Stats.getStatById((int)n).uuid).append(",");
                stringBuilder.append(n2).append(",");
                hashMap.put(stat, n2);
            }
            MD5MessageDigest mD5MessageDigest = new MD5MessageDigest(string);
            object2 = mD5MessageDigest.digest(stringBuilder.toString());
            if (!((String)object2).equals(jsonRootNode.getStringValue("checksum"))) {
                System.out.println("CHECKSUM MISMATCH");
                return null;
            }
        }
        catch (InvalidSyntaxException invalidSyntaxException) {
            invalidSyntaxException.printStackTrace();
        }
        return hashMap;
    }

    public static String serialize(String playerName, String sessionId, Map stats) {
        StringBuilder stringBuilder = new StringBuilder();
        StringBuilder stringBuilder2 = new StringBuilder();
        boolean bl = true;
        stringBuilder.append("{\r\n");
        if (playerName != null && sessionId != null) {
            stringBuilder.append("  \"user\":{\r\n");
            stringBuilder.append("    \"name\":\"").append(playerName).append("\",\r\n");
            stringBuilder.append("    \"sessionid\":\"").append(sessionId).append("\"\r\n");
            stringBuilder.append("  },\r\n");
        }
        stringBuilder.append("  \"stats-change\":[");
        for (Stat stat : stats.keySet()) {
            if (!bl) {
                stringBuilder.append("},");
            } else {
                bl = false;
            }
            stringBuilder.append("\r\n    {\"").append(stat.id).append("\":").append(stats.get(stat));
            stringBuilder2.append(stat.uuid).append(",");
            stringBuilder2.append(stats.get(stat)).append(",");
        }
        if (!bl) {
            stringBuilder.append("}");
        }
        MD5MessageDigest mD5MessageDigest = new MD5MessageDigest(sessionId);
        stringBuilder.append("\r\n  ],\r\n");
        stringBuilder.append("  \"checksum\":\"").append(mD5MessageDigest.digest(stringBuilder2.toString())).append("\"\r\n");
        stringBuilder.append("}");
        return stringBuilder.toString();
    }

    public boolean hasAchievement(Achievement achievement) {
        return this.stats.containsKey(achievement);
    }

    public boolean hasParentAchievement(Achievement achievement) {
        return achievement.parent == null || this.hasAchievement(achievement.parent);
    }

    public int get(Stat stat) {
        Integer n = (Integer)this.stats.get(stat);
        return n == null ? 0 : n;
    }

    public void syncStats() {
    }

    public void save() {
        this.syncer.saveUnsent(this.doIncrement());
    }

    public void tick() {
        if (this.dirty && this.syncer.canSave()) {
            this.syncer.save(this.doIncrement());
        }
        this.syncer.tick();
    }
}

