/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.stat;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.util.Session;
import net.minecraft.stat.PlayerStats;

@Environment(value=EnvType.CLIENT)
public class StatsSyncer {
    private volatile boolean busy = false;
    private volatile Map stats = null;
    private volatile Map unsentStats = null;
    private PlayerStats statHandler;
    private File unsentStatsFile;
    private File statsFile;
    private File tempUnsentStatsFile;
    private File tempStatsFile;
    private File oldUnsentStatsFile;
    private File oldStatsFile;
    private Session session;
    private int saveCooldown = 0;
    private int saveUnsentCooldown = 0;

    public StatsSyncer(Session session, PlayerStats stats, File dir) {
        this.unsentStatsFile = new File(dir, "stats_" + session.username.toLowerCase() + "_unsent.dat");
        this.statsFile = new File(dir, "stats_" + session.username.toLowerCase() + ".dat");
        this.oldUnsentStatsFile = new File(dir, "stats_" + session.username.toLowerCase() + "_unsent.old");
        this.oldStatsFile = new File(dir, "stats_" + session.username.toLowerCase() + ".old");
        this.tempUnsentStatsFile = new File(dir, "stats_" + session.username.toLowerCase() + "_unsent.tmp");
        this.tempStatsFile = new File(dir, "stats_" + session.username.toLowerCase() + ".tmp");
        if (!session.username.toLowerCase().equals(session.username)) {
            this.renameFile(dir, "stats_" + session.username + "_unsent.dat", this.unsentStatsFile);
            this.renameFile(dir, "stats_" + session.username + ".dat", this.statsFile);
            this.renameFile(dir, "stats_" + session.username + "_unsent.old", this.oldUnsentStatsFile);
            this.renameFile(dir, "stats_" + session.username + ".old", this.oldStatsFile);
            this.renameFile(dir, "stats_" + session.username + "_unsent.tmp", this.tempUnsentStatsFile);
            this.renameFile(dir, "stats_" + session.username + ".tmp", this.tempStatsFile);
        }
        this.statHandler = stats;
        this.session = session;
        if (this.unsentStatsFile.exists()) {
            stats.increment(this.loadStats(this.unsentStatsFile, this.tempUnsentStatsFile, this.oldUnsentStatsFile));
        }
        this.load();
    }

    private void renameFile(File dir, String name, File to) {
        File file = new File(dir, name);
        if (file.exists() && !file.isDirectory() && !to.exists()) {
            file.renameTo(to);
        }
    }

    private Map loadStats(File file, File tempFile, File oldFile) {
        if (file.exists()) {
            return this.loadStats(file);
        }
        if (oldFile.exists()) {
            return this.loadStats(oldFile);
        }
        if (tempFile.exists()) {
            return this.loadStats(tempFile);
        }
        return null;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private Map loadStats(File file) {
        BufferedReader bufferedReader = null;
        try {
            bufferedReader = new BufferedReader(new FileReader(file));
            String string = "";
            StringBuilder stringBuilder = new StringBuilder();
            while ((string = bufferedReader.readLine()) != null) {
                stringBuilder.append(string);
            }
            Map map = PlayerStats.deserialize(stringBuilder.toString());
            return map;
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        finally {
            if (bufferedReader != null) {
                try {
                    bufferedReader.close();
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                }
            }
        }
        return null;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private void save(Map stats, File file, File tempFile, File oldFile) {
        PrintWriter printWriter = new PrintWriter(new FileWriter(tempFile, false));
        try {
            printWriter.print(PlayerStats.serialize(this.session.username, "local", stats));
        }
        finally {
            printWriter.close();
        }
        if (oldFile.exists()) {
            oldFile.delete();
        }
        if (file.exists()) {
            file.renameTo(oldFile);
        }
        tempFile.renameTo(file);
    }

    public void load() {
        if (this.busy) {
            throw new IllegalStateException("Can't get stats from server while StatsSyncher is busy!");
        }
        this.saveCooldown = 100;
        this.busy = true;
        new Thread(){

            /*
             * WARNING - Removed try catching itself - possible behaviour change.
             */
            public void run() {
                try {
                    if (StatsSyncer.this.stats != null) {
                        StatsSyncer.this.save(StatsSyncer.this.stats, StatsSyncer.this.statsFile, StatsSyncer.this.tempStatsFile, StatsSyncer.this.oldStatsFile);
                    } else if (StatsSyncer.this.statsFile.exists()) {
                        StatsSyncer.this.stats = StatsSyncer.this.loadStats(StatsSyncer.this.statsFile, StatsSyncer.this.tempStatsFile, StatsSyncer.this.oldStatsFile);
                    }
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                }
                finally {
                    StatsSyncer.this.busy = false;
                }
            }
        }.start();
    }

    public void save(final Map stats) {
        if (this.busy) {
            throw new IllegalStateException("Can't save stats while StatsSyncher is busy!");
        }
        this.saveCooldown = 100;
        this.busy = true;
        new Thread(){

            /*
             * WARNING - Removed try catching itself - possible behaviour change.
             */
            public void run() {
                try {
                    StatsSyncer.this.save(stats, StatsSyncer.this.unsentStatsFile, StatsSyncer.this.tempUnsentStatsFile, StatsSyncer.this.oldUnsentStatsFile);
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                }
                finally {
                    StatsSyncer.this.busy = false;
                }
            }
        }.start();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void saveUnsent(Map stats) {
        int n = 30;
        while (this.busy && --n > 0) {
            try {
                Thread.sleep(100L);
            }
            catch (InterruptedException interruptedException) {
                interruptedException.printStackTrace();
            }
        }
        this.busy = true;
        try {
            this.save(stats, this.unsentStatsFile, this.tempUnsentStatsFile, this.oldUnsentStatsFile);
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        finally {
            this.busy = false;
        }
    }

    public boolean canSave() {
        return this.saveCooldown <= 0 && !this.busy && this.unsentStats == null;
    }

    public void tick() {
        if (this.saveCooldown > 0) {
            --this.saveCooldown;
        }
        if (this.saveUnsentCooldown > 0) {
            --this.saveUnsentCooldown;
        }
        if (this.unsentStats != null) {
            this.statHandler.tickUnsent(this.unsentStats);
            this.unsentStats = null;
        }
        if (this.stats != null) {
            this.statHandler.tick(this.stats);
            this.stats = null;
        }
    }
}

