/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.awt.GraphicsEnvironment;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;
import net.minecraft.server.PlayerManager;
import net.minecraft.server.ServerLog;
import net.minecraft.server.ServerProperties;
import net.minecraft.server.command.Command;
import net.minecraft.server.command.CommandOutput;
import net.minecraft.server.command.ServerCommandHandler;
import net.minecraft.server.dedicated.gui.DedicatedServerGui;
import net.minecraft.server.entity.EntityTracker;
import net.minecraft.server.network.ConnectionListener;
import net.minecraft.server.world.ReadOnlyServerWorld;
import net.minecraft.server.world.ServerWorldEventListener;
import net.minecraft.stat.Stats;
import net.minecraft.util.Tickable;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.World;
import net.minecraft.world.storage.RegionWorldStorage;
import net.minecraft.world.storage.RegionWorldStorageSource;
import net.minecraft.world.storage.WorldStorageSource;

@Environment(value=EnvType.SERVER)
public class MinecraftServer
implements Runnable,
CommandOutput {
    public static Logger LOGGER = Logger.getLogger("Minecraft");
    public static HashMap capturedThread = new HashMap();
    public ConnectionListener connections;
    public ServerProperties properties;
    public ServerWorld[] worlds;
    public PlayerManager playerManager;
    private ServerCommandHandler commandHandler;
    private boolean running = true;
    public boolean stopped = false;
    int ticks = 0;
    public String progressMessage;
    public int progress;
    private List tickables = new ArrayList();
    private List pendingCommands = Collections.synchronizedList(new ArrayList());
    public EntityTracker[] entityTrackers = new EntityTracker[2];
    public boolean onlineMode;
    public boolean spawnAnimals;
    public boolean pvpEnabled;
    public boolean flightEnabled;

    public MinecraftServer() {
        new BackgroundDaemon();
    }

    private boolean init() {
        this.commandHandler = new ServerCommandHandler(this);
        CommandThread commandThread = new CommandThread();
        commandThread.setDaemon(true);
        commandThread.start();
        ServerLog.init();
        LOGGER.info("Starting minecraft server version Beta 1.7.3");
        if (Runtime.getRuntime().maxMemory() / 1024L / 1024L < 512L) {
            LOGGER.warning("**** NOT ENOUGH RAM!");
            LOGGER.warning("To start the server with more ram, launch it as \"java -Xmx1024M -Xms1024M -jar minecraft_server.jar\"");
        }
        LOGGER.info("Loading properties");
        this.properties = new ServerProperties(new File("server.properties"));
        String string = this.properties.getProperty("server-ip", "");
        this.onlineMode = this.properties.getProperty("online-mode", true);
        this.spawnAnimals = this.properties.getProperty("spawn-animals", true);
        this.pvpEnabled = this.properties.getProperty("pvp", true);
        this.flightEnabled = this.properties.getProperty("allow-flight", false);
        InetAddress inetAddress = null;
        if (string.length() > 0) {
            inetAddress = InetAddress.getByName(string);
        }
        int n = this.properties.getProperty("server-port", 25565);
        LOGGER.info("Starting Minecraft server on " + (string.length() == 0 ? "*" : string) + ":" + n);
        try {
            this.connections = new ConnectionListener(this, inetAddress, n);
        }
        catch (IOException iOException) {
            LOGGER.warning("**** FAILED TO BIND TO PORT!");
            LOGGER.log(Level.WARNING, "The exception was: " + iOException.toString());
            LOGGER.warning("Perhaps a server is already running on that port?");
            return false;
        }
        if (!this.onlineMode) {
            LOGGER.warning("**** SERVER IS RUNNING IN OFFLINE/INSECURE MODE!");
            LOGGER.warning("The server will make no attempt to authenticate usernames. Beware.");
            LOGGER.warning("While this makes the game possible to play without internet access, it also opens up the ability for hackers to connect with any username they choose.");
            LOGGER.warning("To change this, set \"online-mode\" to \"true\" in the server.settings file.");
        }
        this.playerManager = new PlayerManager(this);
        this.entityTrackers[0] = new EntityTracker(this, 0);
        this.entityTrackers[1] = new EntityTracker(this, -1);
        long l = System.nanoTime();
        String string2 = this.properties.getProperty("level-name", "world");
        String string3 = this.properties.getProperty("level-seed", "");
        long l2 = new Random().nextLong();
        if (string3.length() > 0) {
            try {
                l2 = Long.parseLong(string3);
            }
            catch (NumberFormatException numberFormatException) {
                l2 = string3.hashCode();
            }
        }
        LOGGER.info("Preparing level \"" + string2 + "\"");
        this.loadWorld(new RegionWorldStorageSource(new File(".")), string2, l2);
        LOGGER.info("Done (" + (System.nanoTime() - l) + "ns)! For help, type \"help\" or \"?\"");
        return true;
    }

    private void loadWorld(WorldStorageSource storageSource, String worldDir, long seed) {
        int n;
        if (storageSource.needsConversion(worldDir)) {
            LOGGER.info("Converting map!");
            storageSource.convert(worldDir, new WorldConversionProgress());
        }
        this.worlds = new ServerWorld[2];
        RegionWorldStorage regionWorldStorage = new RegionWorldStorage(new File("."), worldDir, true);
        for (n = 0; n < this.worlds.length; ++n) {
            this.worlds[n] = n == 0 ? new ServerWorld(this, regionWorldStorage, worldDir, n == 0 ? 0 : -1, seed) : new ReadOnlyServerWorld(this, regionWorldStorage, worldDir, n == 0 ? 0 : -1, seed, this.worlds[0]);
            this.worlds[n].addEventListener(new ServerWorldEventListener(this, this.worlds[n]));
            this.worlds[n].difficulty = this.properties.getProperty("spawn-monsters", true) ? 1 : 0;
            this.worlds[n].allowSpawning(this.properties.getProperty("spawn-monsters", true), this.spawnAnimals);
            this.playerManager.saveAllPlayers(this.worlds);
        }
        n = 196;
        long l = System.currentTimeMillis();
        for (int i = 0; i < this.worlds.length; ++i) {
            LOGGER.info("Preparing start region for level " + i);
            if (i != 0 && !this.properties.getProperty("allow-nether", true)) continue;
            ServerWorld serverWorld = this.worlds[i];
            Vec3i vec3i = serverWorld.getSpawnPos();
            for (int j = -n; j <= n && this.running; j += 16) {
                for (int k = -n; k <= n && this.running; k += 16) {
                    long l2 = System.currentTimeMillis();
                    if (l2 < l) {
                        l = l2;
                    }
                    if (l2 > l + 1000L) {
                        int n2 = (n * 2 + 1) * (n * 2 + 1);
                        int n3 = (j + n) * (n * 2 + 1) + (k + 1);
                        this.logProgress("Preparing spawn area", n3 * 100 / n2);
                        l = l2;
                    }
                    serverWorld.chunkCache.loadChunk(vec3i.x + j >> 4, vec3i.z + k >> 4);
                    while (serverWorld.doLightingUpdates() && this.running) {
                    }
                }
            }
        }
        this.clearProgress();
    }

    private void logProgress(String progressType, int progress) {
        this.progressMessage = progressType;
        this.progress = progress;
        LOGGER.info(progressType + ": " + progress + "%");
    }

    private void clearProgress() {
        this.progressMessage = null;
        this.progress = 0;
    }

    private void saveWorlds() {
        LOGGER.info("Saving chunks");
        for (int i = 0; i < this.worlds.length; ++i) {
            ServerWorld serverWorld = this.worlds[i];
            serverWorld.saveWithLoadingDisplay(true, null);
            serverWorld.forceSave();
        }
    }

    private void shutdown() {
        LOGGER.info("Stopping server");
        if (this.playerManager != null) {
            this.playerManager.savePlayers();
        }
        for (int i = 0; i < this.worlds.length; ++i) {
            ServerWorld serverWorld = this.worlds[i];
            if (serverWorld == null) continue;
            this.saveWorlds();
        }
    }

    public void stop() {
        this.running = false;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    public void run() {
        try {
            if (this.init()) {
                long l = System.currentTimeMillis();
                long l2 = 0L;
                while (this.running) {
                    long l3 = System.currentTimeMillis();
                    long l4 = l3 - l;
                    if (l4 > 2000L) {
                        LOGGER.warning("Can't keep up! Did the system time change, or is the server overloaded?");
                        l4 = 2000L;
                    }
                    if (l4 < 0L) {
                        LOGGER.warning("Time ran backwards! Did the system time change?");
                        l4 = 0L;
                    }
                    l2 += l4;
                    l = l3;
                    if (this.worlds[0].canSkipNight()) {
                        this.tick();
                        l2 = 0L;
                    } else {
                        while (l2 > 50L) {
                            l2 -= 50L;
                            this.tick();
                        }
                    }
                    Thread.sleep(1L);
                }
                return;
            } else {
                while (this.running) {
                    this.runPendingCommands();
                    try {
                        Thread.sleep(10L);
                    }
                    catch (InterruptedException interruptedException) {
                        interruptedException.printStackTrace();
                    }
                }
            }
            return;
        }
        catch (Throwable throwable) {
            throwable.printStackTrace();
            LOGGER.log(Level.SEVERE, "Unexpected exception", throwable);
            while (this.running) {
                this.runPendingCommands();
                try {
                    Thread.sleep(10L);
                }
                catch (InterruptedException interruptedException) {
                    interruptedException.printStackTrace();
                }
            }
            return;
        }
        finally {
            try {
                this.shutdown();
                this.stopped = true;
            }
            catch (Throwable throwable) {
                throwable.printStackTrace();
            }
            finally {
                System.exit(0);
            }
        }
    }

    private void tick() {
        int n;
        ArrayList<String> arrayList = new ArrayList<String>();
        for (Object object : capturedThread.keySet()) {
            int n2 = (Integer)capturedThread.get(object);
            if (n2 > 0) {
                capturedThread.put(object, n2 - 1);
                continue;
            }
            arrayList.add((String)object);
        }
        for (n = 0; n < arrayList.size(); ++n) {
            capturedThread.remove(arrayList.get(n));
        }
        Box.resetCacheCount();
        Vec3d.resetCacheCount();
        ++this.ticks;
        for (n = 0; n < this.worlds.length; ++n) {
            Object object;
            if (n != 0 && !this.properties.getProperty("allow-nether", true)) continue;
            object = this.worlds[n];
            if (this.ticks % 20 == 0) {
                this.playerManager.sendToDimension(new WorldTimeUpdateS2CPacket(((World)object).getTime()), ((ServerWorld)object).dimension.id);
            }
            ((World)object).tick();
            while (((World)object).doLightingUpdates()) {
            }
            ((World)object).tickEntities();
        }
        this.connections.tick();
        this.playerManager.updateAllChunks();
        for (n = 0; n < this.entityTrackers.length; ++n) {
            this.entityTrackers[n].tick();
        }
        for (n = 0; n < this.tickables.size(); ++n) {
            ((Tickable)this.tickables.get(n)).tick();
        }
        try {
            this.runPendingCommands();
        }
        catch (Exception exception) {
            LOGGER.log(Level.WARNING, "Unexpected exception while parsing console command", exception);
        }
    }

    public void queueCommands(String string, CommandOutput arg) {
        this.pendingCommands.add(new Command(string, arg));
    }

    public void runPendingCommands() {
        while (this.pendingCommands.size() > 0) {
            Command command = (Command)this.pendingCommands.remove(0);
            this.commandHandler.executeCommand(command);
        }
    }

    public void addTickable(Tickable tickable) {
        this.tickables.add(tickable);
    }

    public static void main(String[] args) {
        Stats.initialize();
        try {
            MinecraftServer minecraftServer = new MinecraftServer();
            if (!(GraphicsEnvironment.isHeadless() || args.length > 0 && args[0].equals("nogui"))) {
                DedicatedServerGui.create(minecraftServer);
            }
            new ServerThread("Server thread", minecraftServer).start();
        }
        catch (Exception exception) {
            LOGGER.log(Level.SEVERE, "Failed to start the minecraft server", exception);
        }
    }

    public File getFile(String path) {
        return new File(path);
    }

    public void sendMessage(String message) {
        LOGGER.info(message);
    }

    public void warn(String message) {
        LOGGER.warning(message);
    }

    public String getName() {
        return "CONSOLE";
    }

    public ServerWorld getWorld(int dimensionId) {
        if (dimensionId == -1) {
            return this.worlds[1];
        }
        return this.worlds[0];
    }

    public EntityTracker getEntityTracker(int dimensionId) {
        if (dimensionId == -1) {
            return this.entityTrackers[1];
        }
        return this.entityTrackers[0];
    }

    @Environment(value=EnvType.SERVER)
    public final class ServerThread
    extends Thread {
        final /* synthetic */ MinecraftServer server;

        public ServerThread(String name, MinecraftServer server) {
            this.server = server;
            super(name);
        }

        public void run() {
            this.server.run();
        }
    }

    @Environment(value=EnvType.SERVER)
    public class WorldConversionProgress
    implements LoadingDisplay {
        private long lastLogTime = System.currentTimeMillis();

        public void progressStartNoAbort(String title) {
        }

        public void progressStagePercentage(int percentage) {
            if (System.currentTimeMillis() - this.lastLogTime >= 1000L) {
                this.lastLogTime = System.currentTimeMillis();
                LOGGER.info("Converting... " + percentage + "%");
            }
        }

        public void progressStage(String stage) {
        }
    }

    @Environment(value=EnvType.SERVER)
    public class CommandThread
    extends Thread {
        public void run() {
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in));
            String string = null;
            try {
                while (!MinecraftServer.this.stopped && MinecraftServer.this.running && (string = bufferedReader.readLine()) != null) {
                    MinecraftServer.this.queueCommands(string, MinecraftServer.this);
                }
            }
            catch (IOException iOException) {
                iOException.printStackTrace();
            }
        }
    }

    @Environment(value=EnvType.SERVER)
    public class BackgroundDaemon
    extends Thread {
        public BackgroundDaemon() {
            this.setDaemon(true);
            this.start();
        }

        public void run() {
            while (true) {
                try {
                    while (true) {
                        Thread.sleep(Integer.MAX_VALUE);
                    }
                }
                catch (InterruptedException interruptedException) {
                    continue;
                }
                break;
            }
        }
    }
}

