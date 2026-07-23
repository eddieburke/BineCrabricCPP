/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import java.awt.BorderLayout;
import java.awt.Canvas;
import java.awt.Color;
import java.awt.Component;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.net.HttpURLConnection;
import java.net.URL;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.Block;
import net.minecraft.client.CrashReportPanel;
import net.minecraft.client.InteractionManager;
import net.minecraft.client.MinecraftApplet;
import net.minecraft.client.Mouse;
import net.minecraft.client.Screenshot;
import net.minecraft.client.TestInteractionManager;
import net.minecraft.client.color.world.FoliageColors;
import net.minecraft.client.color.world.GrassColors;
import net.minecraft.client.color.world.WaterColors;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.hud.InGameHud;
import net.minecraft.client.gui.hud.toast.AchievementToast;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.gui.screen.ConnectScreen;
import net.minecraft.client.gui.screen.DeathScreen;
import net.minecraft.client.gui.screen.FatalErrorScreen;
import net.minecraft.client.gui.screen.GameMenuScreen;
import net.minecraft.client.gui.screen.OutOfMemoryScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.SleepingChatScreen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.client.gui.screen.ingame.InventoryScreen;
import net.minecraft.client.gui.screen.world.WorldSaveConflictScreen;
import net.minecraft.client.input.KeyboardInput;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.network.MultiplayerClientPlayerEntity;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.particle.ParticleManager;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.OpenGlCapabilities;
import net.minecraft.client.render.ProgressRenderError;
import net.minecraft.client.render.ProgressRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.WorldRenderer;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.chunk.ChunkBuilder;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.client.render.item.HeldItemRenderer;
import net.minecraft.client.render.texture.ClockSprite;
import net.minecraft.client.render.texture.CompassSprite;
import net.minecraft.client.render.texture.FireSprite;
import net.minecraft.client.render.texture.LavaSideSprite;
import net.minecraft.client.render.texture.LavaSprite;
import net.minecraft.client.render.texture.NetherPortalSprite;
import net.minecraft.client.render.texture.WaterSideSprite;
import net.minecraft.client.render.texture.WaterSprite;
import net.minecraft.client.resource.ResourceDownloadThread;
import net.minecraft.client.resource.pack.TexturePacks;
import net.minecraft.client.sound.SoundManager;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.client.util.GlAllocationUtils;
import net.minecraft.client.util.ScreenScaler;
import net.minecraft.client.util.Session;
import net.minecraft.client.util.Timer;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.ClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.stat.PlayerStats;
import net.minecraft.stat.Stats;
import net.minecraft.util.OperatingSystem;
import net.minecraft.util.crash.CrashReport;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.hit.HitResultType;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.World;
import net.minecraft.world.chunk.ChunkSource;
import net.minecraft.world.chunk.LegacyChunkCache;
import net.minecraft.world.dimension.Dimension;
import net.minecraft.world.dimension.PortalForcer;
import net.minecraft.world.storage.RegionWorldStorageSource;
import net.minecraft.world.storage.WorldStorage;
import net.minecraft.world.storage.WorldStorageSource;
import net.minecraft.world.storage.exception.SessionLockException;
import org.lwjgl.LWJGLException;
import org.lwjgl.input.Controllers;
import org.lwjgl.input.Keyboard;
import org.lwjgl.opengl.Display;
import org.lwjgl.opengl.DisplayMode;
import org.lwjgl.opengl.GL11;
import org.lwjgl.util.glu.GLU;

@Environment(value=EnvType.CLIENT)
public abstract class Minecraft
implements Runnable {
    public static byte[] MEMORY_RESERVED_FOR_CRASH = new byte[0xA00000];
    private static Minecraft INSTANCE;
    public InteractionManager interactionManager;
    private boolean fullscreen = false;
    private boolean crashed = false;
    public int displayWidth;
    public int displayHeight;
    private OpenGlCapabilities openGlCapabilities;
    private Timer timer = new Timer(20.0f);
    public World world;
    public WorldRenderer worldRenderer;
    public ClientPlayerEntity player;
    public LivingEntity camera;
    public ParticleManager particleManager;
    public Session session = null;
    public String hostAddress;
    public Canvas canvas;
    public boolean isApplet = true;
    public volatile boolean paused = false;
    public TextureManager textureManager;
    public TextRenderer textRenderer;
    public Screen currentScreen = null;
    public ProgressRenderer progressRenderer = new ProgressRenderer(this);
    public GameRenderer gameRenderer;
    private ResourceDownloadThread resourceDownloadThread;
    private int ticksPlayed = 0;
    private int attackCooldown = 0;
    private int initWidth;
    private int initHeight;
    public AchievementToast toast = new AchievementToast(this);
    public InGameHud inGameHud;
    public boolean skipGameRender = false;
    public BipedEntityModel bipedModel = new BipedEntityModel(0.0f);
    public HitResult crosshairTarget = null;
    public GameOptions options;
    protected MinecraftApplet applet;
    public SoundManager soundManager = new SoundManager();
    public Mouse mouse;
    public TexturePacks texturePacks;
    private File runDirectory;
    private WorldStorageSource worldStorageSource;
    public static long[] frameTimes;
    public static long[] tickTimes;
    public static int frameTimeIndex;
    public static long failedSessionCheckTime;
    public PlayerStats stats;
    private String startupServerAddress;
    private int startupServerPort;
    private WaterSprite waterSprite = new WaterSprite();
    private LavaSprite lavaSprite = new LavaSprite();
    private static File runDirectoryCache;
    public volatile boolean running = true;
    public String debugText = "";
    boolean screenshotKeyDown = false;
    long timeAfterLastTick = -1L;
    public boolean focused = false;
    private int lastClickTicks = 0;
    public boolean raining = false;
    long lastTickTime = System.currentTimeMillis();
    private int joinPlayerCounter = 0;

    public Minecraft(Component component, Canvas canvas, MinecraftApplet applet, int width, int height, boolean fullscreen) {
        Stats.initialize();
        this.initHeight = height;
        this.fullscreen = fullscreen;
        this.applet = applet;
        new TimerHackThread("Timer hack thread");
        this.canvas = canvas;
        this.displayWidth = width;
        this.displayHeight = height;
        this.fullscreen = fullscreen;
        if (applet == null || "true".equals(applet.getParameter("stand-alone"))) {
            this.isApplet = false;
        }
        INSTANCE = this;
    }

    public void gameCrashed(CrashReport crashReport) {
        this.crashed = true;
        this.handleCrash(crashReport);
    }

    public abstract void handleCrash(CrashReport var1);

    public void setStartupServer(String address, int port) {
        this.startupServerAddress = address;
        this.startupServerPort = port;
    }

    public void init() {
        if (this.canvas != null) {
            Graphics graphics = this.canvas.getGraphics();
            if (graphics != null) {
                graphics.setColor(Color.BLACK);
                graphics.fillRect(0, 0, this.displayWidth, this.displayHeight);
                graphics.dispose();
            }
            Display.setParent(this.canvas);
        } else if (this.fullscreen) {
            Display.setFullscreen(true);
            this.displayWidth = Display.getDisplayMode().getWidth();
            this.displayHeight = Display.getDisplayMode().getHeight();
            if (this.displayWidth <= 0) {
                this.displayWidth = 1;
            }
            if (this.displayHeight <= 0) {
                this.displayHeight = 1;
            }
        } else {
            Display.setDisplayMode(new DisplayMode(this.displayWidth, this.displayHeight));
        }
        Display.setTitle("Minecraft Minecraft Beta 1.7.3");
        try {
            Display.create();
        }
        catch (LWJGLException lWJGLException) {
            lWJGLException.printStackTrace();
            try {
                Thread.sleep(1000L);
            }
            catch (InterruptedException interruptedException) {
                // empty catch block
            }
            Display.create();
        }
        this.runDirectory = Minecraft.getRunDirectory();
        this.worldStorageSource = new RegionWorldStorageSource(new File(this.runDirectory, "saves"));
        this.options = new GameOptions(this, this.runDirectory);
        this.texturePacks = new TexturePacks(this, this.runDirectory);
        this.textureManager = new TextureManager(this.texturePacks, this.options);
        this.textRenderer = new TextRenderer(this.options, "/font/default.png", this.textureManager);
        WaterColors.setColorMap(this.textureManager.getColors("/misc/watercolor.png"));
        GrassColors.setColorMap(this.textureManager.getColors("/misc/grasscolor.png"));
        FoliageColors.setColorMap(this.textureManager.getColors("/misc/foliagecolor.png"));
        this.gameRenderer = new GameRenderer(this);
        EntityRenderDispatcher.INSTANCE.heldItemRenderer = new HeldItemRenderer(this);
        this.stats = new PlayerStats(this.session, this.runDirectory);
        Achievements.OPEN_INVENTORY.setTranslationHelper(new AchievementStatFormatter());
        this.renderLoadingScreen();
        Keyboard.create();
        org.lwjgl.input.Mouse.create();
        this.mouse = new Mouse(this.canvas);
        try {
            Controllers.create();
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        this.logGlError("Pre startup");
        GL11.glEnable(3553);
        GL11.glShadeModel(7425);
        GL11.glClearDepth(1.0);
        GL11.glEnable(2929);
        GL11.glDepthFunc(515);
        GL11.glEnable(3008);
        GL11.glAlphaFunc(516, 0.1f);
        GL11.glCullFace(1029);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glMatrixMode(5888);
        this.logGlError("Startup");
        this.openGlCapabilities = new OpenGlCapabilities();
        this.soundManager.loadSounds(this.options);
        this.textureManager.addDynamicTexture(this.lavaSprite);
        this.textureManager.addDynamicTexture(this.waterSprite);
        this.textureManager.addDynamicTexture(new NetherPortalSprite());
        this.textureManager.addDynamicTexture(new CompassSprite(this));
        this.textureManager.addDynamicTexture(new ClockSprite(this));
        this.textureManager.addDynamicTexture(new WaterSideSprite());
        this.textureManager.addDynamicTexture(new LavaSideSprite());
        this.textureManager.addDynamicTexture(new FireSprite(0));
        this.textureManager.addDynamicTexture(new FireSprite(1));
        this.worldRenderer = new WorldRenderer(this, this.textureManager);
        GL11.glViewport(0, 0, this.displayWidth, this.displayHeight);
        this.particleManager = new ParticleManager(this.world, this.textureManager);
        try {
            this.resourceDownloadThread = new ResourceDownloadThread(this.runDirectory, this);
            this.resourceDownloadThread.start();
        }
        catch (Exception exception) {
            // empty catch block
        }
        this.logGlError("Post startup");
        this.inGameHud = new InGameHud(this);
        if (this.startupServerAddress != null) {
            this.setScreen(new ConnectScreen(this, this.startupServerAddress, this.startupServerPort));
        } else {
            this.setScreen(new TitleScreen());
        }
    }

    private void renderLoadingScreen() {
        ScreenScaler screenScaler = new ScreenScaler(this.options, this.displayWidth, this.displayHeight);
        GL11.glClear(16640);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glOrtho(0.0, screenScaler.rawScaledWidth, screenScaler.rawScaledHeight, 0.0, 1000.0, 3000.0);
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        GL11.glTranslatef(0.0f, 0.0f, -2000.0f);
        GL11.glViewport(0, 0, this.displayWidth, this.displayHeight);
        GL11.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glDisable(2896);
        GL11.glEnable(3553);
        GL11.glDisable(2912);
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/title/mojang.png"));
        tessellator.startQuads();
        tessellator.color(0xFFFFFF);
        tessellator.vertex(0.0, this.displayHeight, 0.0, 0.0, 0.0);
        tessellator.vertex(this.displayWidth, this.displayHeight, 0.0, 0.0, 0.0);
        tessellator.vertex(this.displayWidth, 0.0, 0.0, 0.0, 0.0);
        tessellator.vertex(0.0, 0.0, 0.0, 0.0, 0.0);
        tessellator.draw();
        int n = 256;
        int n2 = 256;
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        tessellator.color(0xFFFFFF);
        this.draw((screenScaler.getScaledWidth() - n) / 2, (screenScaler.getScaledHeight() - n2) / 2, 0, 0, n, n2);
        GL11.glDisable(2896);
        GL11.glDisable(2912);
        GL11.glEnable(3008);
        GL11.glAlphaFunc(516, 0.1f);
        Display.swapBuffers();
    }

    public void draw(int x, int y, int u, int v, int width, int height) {
        float f = 0.00390625f;
        float f2 = 0.00390625f;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(x + 0, y + height, 0.0, (float)(u + 0) * f, (float)(v + height) * f2);
        tessellator.vertex(x + width, y + height, 0.0, (float)(u + width) * f, (float)(v + height) * f2);
        tessellator.vertex(x + width, y + 0, 0.0, (float)(u + width) * f, (float)(v + 0) * f2);
        tessellator.vertex(x + 0, y + 0, 0.0, (float)(u + 0) * f, (float)(v + 0) * f2);
        tessellator.draw();
    }

    public static File getRunDirectory() {
        if (runDirectoryCache == null) {
            runDirectoryCache = Minecraft.getApplicationDirectory("minecraft");
        }
        return runDirectoryCache;
    }

    public static File getApplicationDirectory(String name) {
        File file;
        String string = System.getProperty("user.home", ".");
        switch (Minecraft.getOperatingSystem()) {
            case LINUX: 
            case SOLARIS: {
                file = new File(string, '.' + name + '/');
                break;
            }
            case WINDOWS: {
                String string2 = System.getenv("APPDATA");
                if (string2 != null) {
                    file = new File(string2, "." + name + '/');
                    break;
                }
                file = new File(string, '.' + name + '/');
                break;
            }
            case MACOS: {
                file = new File(string, "Library/Application Support/" + name);
                break;
            }
            default: {
                file = new File(string, name + '/');
            }
        }
        if (!file.exists() && !file.mkdirs()) {
            throw new RuntimeException("The working directory could not be created: " + file);
        }
        return file;
    }

    private static OperatingSystem getOperatingSystem() {
        String string = System.getProperty("os.name").toLowerCase();
        if (string.contains("win")) {
            return OperatingSystem.WINDOWS;
        }
        if (string.contains("mac")) {
            return OperatingSystem.MACOS;
        }
        if (string.contains("solaris")) {
            return OperatingSystem.SOLARIS;
        }
        if (string.contains("sunos")) {
            return OperatingSystem.SOLARIS;
        }
        if (string.contains("linux")) {
            return OperatingSystem.LINUX;
        }
        if (string.contains("unix")) {
            return OperatingSystem.LINUX;
        }
        return OperatingSystem.UNKNOWN;
    }

    public WorldStorageSource getWorldStorageSource() {
        return this.worldStorageSource;
    }

    public void setScreen(Screen screen) {
        if (this.currentScreen instanceof FatalErrorScreen) {
            return;
        }
        if (this.currentScreen != null) {
            this.currentScreen.removed();
        }
        if (screen instanceof TitleScreen) {
            this.stats.syncStats();
        }
        this.stats.save();
        if (screen == null && this.world == null) {
            screen = new TitleScreen();
        } else if (screen == null && this.player.health <= 0) {
            screen = new DeathScreen();
        }
        if (screen instanceof TitleScreen) {
            this.inGameHud.clearChat();
        }
        this.currentScreen = screen;
        if (screen != null) {
            this.unlockMouse();
            ScreenScaler screenScaler = new ScreenScaler(this.options, this.displayWidth, this.displayHeight);
            int n = screenScaler.getScaledWidth();
            int n2 = screenScaler.getScaledHeight();
            screen.init(this, n, n2);
            this.skipGameRender = false;
        } else {
            this.lockMouse();
        }
    }

    private void logGlError(String phase) {
        int n = GL11.glGetError();
        if (n != 0) {
            String string = GLU.gluErrorString(n);
            System.out.println("########## GL ERROR ##########");
            System.out.println("@ " + phase);
            System.out.println(n + ": " + string);
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void stop() {
        try {
            this.stats.syncStats();
            this.stats.save();
            if (this.applet != null) {
                this.applet.clearMemory();
            }
            try {
                if (this.resourceDownloadThread != null) {
                    this.resourceDownloadThread.cancel();
                }
            }
            catch (Exception exception) {
                // empty catch block
            }
            System.out.println("Stopping!");
            try {
                this.setWorld(null);
            }
            catch (Throwable throwable) {
                // empty catch block
            }
            try {
                GlAllocationUtils.clear();
            }
            catch (Throwable throwable) {
                // empty catch block
            }
            this.soundManager.stop();
            org.lwjgl.input.Mouse.destroy();
            Keyboard.destroy();
        }
        finally {
            Display.destroy();
            if (!this.crashed) {
                System.exit(0);
            }
        }
        System.gc();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void run() {
        this.running = true;
        try {
            this.init();
        }
        catch (Exception exception) {
            exception.printStackTrace();
            this.gameCrashed(new CrashReport("Failed to start game", exception));
            return;
        }
        try {
            long l = System.currentTimeMillis();
            int n = 0;
            while (this.running) {
                try {
                    if (this.applet != null && !this.applet.isActive()) {
                        break;
                    }
                    Box.resetCacheCount();
                    Vec3d.resetCacheCount();
                    if (this.canvas == null && Display.isCloseRequested()) {
                        this.scheduleStop();
                    }
                    if (this.paused && this.world != null) {
                        float f = this.timer.partialTick;
                        this.timer.advance();
                        this.timer.partialTick = f;
                    } else {
                        this.timer.advance();
                    }
                    long l2 = System.nanoTime();
                    for (int i = 0; i < this.timer.ticksThisFrame; ++i) {
                        ++this.ticksPlayed;
                        try {
                            this.tick();
                            continue;
                        }
                        catch (SessionLockException sessionLockException) {
                            this.world = null;
                            this.setWorld(null);
                            this.setScreen(new WorldSaveConflictScreen());
                        }
                    }
                    long l3 = System.nanoTime() - l2;
                    this.logGlError("Pre render");
                    BlockRenderManager.fancyGraphics = this.options.fancyGraphics;
                    this.soundManager.updateListenerPosition(this.player, this.timer.partialTick);
                    GL11.glEnable(3553);
                    if (this.world != null) {
                        this.world.doLightingUpdates();
                    }
                    if (!Keyboard.isKeyDown(65)) {
                        Display.update();
                    }
                    if (this.player != null && this.player.isInsideWall()) {
                        this.options.thirdPerson = false;
                    }
                    if (!this.skipGameRender) {
                        if (this.interactionManager != null) {
                            this.interactionManager.update(this.timer.partialTick);
                        }
                        this.gameRenderer.onFrameUpdate(this.timer.partialTick);
                    }
                    if (!Display.isActive()) {
                        if (this.fullscreen) {
                            this.toggleFullscreen();
                        }
                        Thread.sleep(10L);
                    }
                    if (this.options.debugHud) {
                        this.renderProfilerChart(l3);
                    } else {
                        this.timeAfterLastTick = System.nanoTime();
                    }
                    this.toast.tick();
                    Thread.yield();
                    if (Keyboard.isKeyDown(65)) {
                        Display.update();
                    }
                    this.handleScreenshotKey();
                    if (!(this.canvas == null || this.fullscreen || this.canvas.getWidth() == this.displayWidth && this.canvas.getHeight() == this.displayHeight)) {
                        this.displayWidth = this.canvas.getWidth();
                        this.displayHeight = this.canvas.getHeight();
                        if (this.displayWidth <= 0) {
                            this.displayWidth = 1;
                        }
                        if (this.displayHeight <= 0) {
                            this.displayHeight = 1;
                        }
                        this.resize(this.displayWidth, this.displayHeight);
                    }
                    this.logGlError("Post render");
                    ++n;
                    boolean bl = this.paused = !this.isWorldRemote() && this.currentScreen != null && this.currentScreen.shouldPause();
                    while (System.currentTimeMillis() >= l + 1000L) {
                        this.debugText = n + " fps, " + ChunkBuilder.chunkUpdates + " chunk updates";
                        ChunkBuilder.chunkUpdates = 0;
                        l += 1000L;
                        n = 0;
                    }
                }
                catch (SessionLockException sessionLockException) {
                    this.world = null;
                    this.setWorld(null);
                    this.setScreen(new WorldSaveConflictScreen());
                }
                catch (OutOfMemoryError outOfMemoryError) {
                    this.cleanHeap();
                    this.setScreen(new OutOfMemoryScreen());
                    System.gc();
                }
            }
        }
        catch (ProgressRenderError progressRenderError) {
        }
        catch (Throwable throwable) {
            this.cleanHeap();
            throwable.printStackTrace();
            this.gameCrashed(new CrashReport("Unexpected error", throwable));
        }
        finally {
            this.stop();
        }
    }

    public void cleanHeap() {
        try {
            MEMORY_RESERVED_FOR_CRASH = new byte[0];
            this.worldRenderer.releaseGlLists();
        }
        catch (Throwable throwable) {
            // empty catch block
        }
        try {
            System.gc();
            Box.clearCache();
            Vec3d.clearCache();
        }
        catch (Throwable throwable) {
            // empty catch block
        }
        try {
            System.gc();
            this.setWorld(null);
        }
        catch (Throwable throwable) {
            // empty catch block
        }
        System.gc();
    }

    private void handleScreenshotKey() {
        if (Keyboard.isKeyDown(60)) {
            if (!this.screenshotKeyDown) {
                this.screenshotKeyDown = true;
                this.inGameHud.addChatMessage(Screenshot.take(runDirectoryCache, this.displayWidth, this.displayHeight));
            }
        } else {
            this.screenshotKeyDown = false;
        }
    }

    private void renderProfilerChart(long tickTime) {
        int n;
        long l = 16666666L;
        if (this.timeAfterLastTick == -1L) {
            this.timeAfterLastTick = System.nanoTime();
        }
        long l2 = System.nanoTime();
        Minecraft.tickTimes[Minecraft.frameTimeIndex & Minecraft.frameTimes.length - 1] = tickTime;
        Minecraft.frameTimes[Minecraft.frameTimeIndex++ & Minecraft.frameTimes.length - 1] = l2 - this.timeAfterLastTick;
        this.timeAfterLastTick = l2;
        GL11.glClear(256);
        GL11.glMatrixMode(5889);
        GL11.glLoadIdentity();
        GL11.glOrtho(0.0, this.displayWidth, this.displayHeight, 0.0, 1000.0, 3000.0);
        GL11.glMatrixMode(5888);
        GL11.glLoadIdentity();
        GL11.glTranslatef(0.0f, 0.0f, -2000.0f);
        GL11.glLineWidth(1.0f);
        GL11.glDisable(3553);
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.start(GL11.GL_QUADS);
        int n2 = (int)(l / 200000L);
        tessellator.color(0x20000000);
        tessellator.vertex(0.0, this.displayHeight - n2, 0.0);
        tessellator.vertex(0.0, this.displayHeight, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight - n2, 0.0);
        tessellator.color(0x20200000);
        tessellator.vertex(0.0, this.displayHeight - n2 * 2, 0.0);
        tessellator.vertex(0.0, this.displayHeight - n2, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight - n2, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight - n2 * 2, 0.0);
        tessellator.draw();
        long l3 = 0L;
        for (n = 0; n < frameTimes.length; ++n) {
            l3 += frameTimes[n];
        }
        n = (int)(l3 / 200000L / (long)frameTimes.length);
        tessellator.start(GL11.GL_QUADS);
        tessellator.color(0x20400000);
        tessellator.vertex(0.0, this.displayHeight - n, 0.0);
        tessellator.vertex(0.0, this.displayHeight, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight, 0.0);
        tessellator.vertex(frameTimes.length, this.displayHeight - n, 0.0);
        tessellator.draw();
        tessellator.start(GL11.GL_LINES);
        for (int i = 0; i < frameTimes.length; ++i) {
            int n3 = (i - frameTimeIndex & frameTimes.length - 1) * 255 / frameTimes.length;
            int n4 = n3 * n3 / 255;
            n4 = n4 * n4 / 255;
            int n5 = n4 * n4 / 255;
            n5 = n5 * n5 / 255;
            if (frameTimes[i] > l) {
                tessellator.color(-16777216 + n4 * 65536);
            } else {
                tessellator.color(-16777216 + n4 * 256);
            }
            long l4 = frameTimes[i] / 200000L;
            long l5 = tickTimes[i] / 200000L;
            tessellator.vertex((float)i + 0.5f, (float)((long)this.displayHeight - l4) + 0.5f, 0.0);
            tessellator.vertex((float)i + 0.5f, (float)this.displayHeight + 0.5f, 0.0);
            tessellator.color(-16777216 + n4 * 65536 + n4 * 256 + n4 * 1);
            tessellator.vertex((float)i + 0.5f, (float)((long)this.displayHeight - l4) + 0.5f, 0.0);
            tessellator.vertex((float)i + 0.5f, (float)((long)this.displayHeight - (l4 - l5)) + 0.5f, 0.0);
        }
        tessellator.draw();
        GL11.glEnable(3553);
    }

    public void scheduleStop() {
        this.running = false;
    }

    public void lockMouse() {
        if (!Display.isActive()) {
            return;
        }
        if (this.focused) {
            return;
        }
        this.focused = true;
        this.mouse.lockCursor();
        this.setScreen(null);
        this.attackCooldown = 10000;
        this.lastClickTicks = this.ticksPlayed + 10000;
    }

    public void unlockMouse() {
        if (!this.focused) {
            return;
        }
        if (this.player != null) {
            this.player.releaseAllKeys();
        }
        this.focused = false;
        this.mouse.unlockCursor();
    }

    public void pauseGame() {
        if (this.currentScreen != null) {
            return;
        }
        this.setScreen(new GameMenuScreen());
    }

    private void handleMouseDown(int button, boolean holdingAttack) {
        if (this.interactionManager.noTick) {
            return;
        }
        if (!holdingAttack) {
            this.attackCooldown = 0;
        }
        if (button == 0 && this.attackCooldown > 0) {
            return;
        }
        if (holdingAttack && this.crosshairTarget != null && this.crosshairTarget.type == HitResultType.BLOCK && button == 0) {
            int n = this.crosshairTarget.blockX;
            int n2 = this.crosshairTarget.blockY;
            int n3 = this.crosshairTarget.blockZ;
            this.interactionManager.processBlockBreakingAction(n, n2, n3, this.crosshairTarget.side);
            this.particleManager.addBlockBreakingParticles(n, n2, n3, this.crosshairTarget.side);
        } else {
            this.interactionManager.cancelBlockBreaking();
        }
    }

    private void handleMouseClick(int button) {
        ItemStack itemStack;
        if (button == 0 && this.attackCooldown > 0) {
            return;
        }
        if (button == 0) {
            this.player.swingHand();
        }
        boolean bl = true;
        if (this.crosshairTarget == null) {
            if (button == 0 && !(this.interactionManager instanceof TestInteractionManager)) {
                this.attackCooldown = 10;
            }
        } else if (this.crosshairTarget.type == HitResultType.ENTITY) {
            if (button == 0) {
                this.interactionManager.attackEntity(this.player, this.crosshairTarget.entity);
            }
            if (button == 1) {
                this.interactionManager.interactEntity(this.player, this.crosshairTarget.entity);
            }
        } else if (this.crosshairTarget.type == HitResultType.BLOCK) {
            int n = this.crosshairTarget.blockX;
            int n2 = this.crosshairTarget.blockY;
            int n3 = this.crosshairTarget.blockZ;
            int n4 = this.crosshairTarget.side;
            if (button == 0) {
                this.interactionManager.attackBlock(n, n2, n3, this.crosshairTarget.side);
            } else {
                int n5;
                ItemStack itemStack2 = this.player.inventory.getSelectedItem();
                int n6 = n5 = itemStack2 != null ? itemStack2.count : 0;
                if (this.interactionManager.interactBlock(this.player, this.world, itemStack2, n, n2, n3, n4)) {
                    bl = false;
                    this.player.swingHand();
                }
                if (itemStack2 == null) {
                    return;
                }
                if (itemStack2.count == 0) {
                    this.player.inventory.main[this.player.inventory.selectedSlot] = null;
                } else if (itemStack2.count != n5) {
                    this.gameRenderer.heldItemRenderer.place();
                }
            }
        }
        if (bl && button == 1 && (itemStack = this.player.inventory.getSelectedItem()) != null && this.interactionManager.interactItem(this.player, this.world, itemStack)) {
            this.gameRenderer.heldItemRenderer.use();
        }
    }

    public void toggleFullscreen() {
        try {
            boolean bl = this.fullscreen = !this.fullscreen;
            if (this.fullscreen) {
                Display.setDisplayMode(Display.getDesktopDisplayMode());
                this.displayWidth = Display.getDisplayMode().getWidth();
                this.displayHeight = Display.getDisplayMode().getHeight();
                if (this.displayWidth <= 0) {
                    this.displayWidth = 1;
                }
                if (this.displayHeight <= 0) {
                    this.displayHeight = 1;
                }
            } else {
                if (this.canvas != null) {
                    this.displayWidth = this.canvas.getWidth();
                    this.displayHeight = this.canvas.getHeight();
                } else {
                    this.displayWidth = this.initWidth;
                    this.displayHeight = this.initHeight;
                }
                if (this.displayWidth <= 0) {
                    this.displayWidth = 1;
                }
                if (this.displayHeight <= 0) {
                    this.displayHeight = 1;
                }
            }
            if (this.currentScreen != null) {
                this.resize(this.displayWidth, this.displayHeight);
            }
            Display.setFullscreen(this.fullscreen);
            Display.update();
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    private void resize(int width, int height) {
        if (width <= 0) {
            width = 1;
        }
        if (height <= 0) {
            height = 1;
        }
        this.displayWidth = width;
        this.displayHeight = height;
        if (this.currentScreen != null) {
            ScreenScaler screenScaler = new ScreenScaler(this.options, width, height);
            int n = screenScaler.getScaledWidth();
            int n2 = screenScaler.getScaledHeight();
            this.currentScreen.init(this, n, n2);
        }
    }

    private void handlePickBlock() {
        if (this.crosshairTarget != null) {
            int n = this.world.getBlockId(this.crosshairTarget.blockX, this.crosshairTarget.blockY, this.crosshairTarget.blockZ);
            if (n == Block.GRASS_BLOCK.id) {
                n = Block.DIRT.id;
            }
            if (n == Block.DOUBLE_SLAB.id) {
                n = Block.SLAB.id;
            }
            if (n == Block.BEDROCK.id) {
                n = Block.STONE.id;
            }
            this.player.inventory.setHeldItem(n, this.interactionManager instanceof TestInteractionManager);
        }
    }

    private void startSessionCheck() {
        new SessionCheckThread().start();
    }

    public void tick() {
        int n;
        ChunkSource chunkSource;
        if (this.ticksPlayed == 6000) {
            this.startSessionCheck();
        }
        this.stats.tick();
        this.inGameHud.tick();
        this.gameRenderer.updateTargetedEntity(1.0f);
        if (this.player != null && (chunkSource = this.world.getChunkSource()) instanceof LegacyChunkCache) {
            LegacyChunkCache legacyChunkCache = (LegacyChunkCache)chunkSource;
            n = MathHelper.floor((int)this.player.x) >> 4;
            int n2 = MathHelper.floor((int)this.player.z) >> 4;
            legacyChunkCache.setSpawnPoint(n, n2);
        }
        if (!this.paused && this.world != null) {
            this.interactionManager.tick();
        }
        GL11.glBindTexture(3553, this.textureManager.getTextureId("/terrain.png"));
        if (!this.paused) {
            this.textureManager.tick();
        }
        if (this.currentScreen == null && this.player != null) {
            if (this.player.health <= 0) {
                this.setScreen(null);
            } else if (this.player.isSleeping() && this.world != null && this.world.isRemote) {
                this.setScreen(new SleepingChatScreen());
            }
        } else if (this.currentScreen != null && this.currentScreen instanceof SleepingChatScreen && !this.player.isSleeping()) {
            this.setScreen(null);
        }
        if (this.currentScreen != null) {
            this.attackCooldown = 10000;
            this.lastClickTicks = this.ticksPlayed + 10000;
        }
        if (this.currentScreen != null) {
            this.currentScreen.tickInput();
            if (this.currentScreen != null) {
                this.currentScreen.particlesGui.tick();
                this.currentScreen.tick();
            }
        }
        if (this.currentScreen == null || this.currentScreen.passEvents) {
            while (org.lwjgl.input.Mouse.next()) {
                long l = System.currentTimeMillis() - this.lastTickTime;
                if (l > 200L) continue;
                n = org.lwjgl.input.Mouse.getEventDWheel();
                if (n != 0) {
                    this.player.inventory.scrollInHotbar(n);
                    if (this.options.discreteScroll) {
                        if (n > 0) {
                            n = 1;
                        }
                        if (n < 0) {
                            n = -1;
                        }
                        this.options.totalDiscreteScroll += (float)n * 0.25f;
                    }
                }
                if (this.currentScreen == null) {
                    if (!this.focused && org.lwjgl.input.Mouse.getEventButtonState()) {
                        this.lockMouse();
                        continue;
                    }
                    if (org.lwjgl.input.Mouse.getEventButton() == 0 && org.lwjgl.input.Mouse.getEventButtonState()) {
                        this.handleMouseClick(0);
                        this.lastClickTicks = this.ticksPlayed;
                    }
                    if (org.lwjgl.input.Mouse.getEventButton() == 1 && org.lwjgl.input.Mouse.getEventButtonState()) {
                        this.handleMouseClick(1);
                        this.lastClickTicks = this.ticksPlayed;
                    }
                    if (org.lwjgl.input.Mouse.getEventButton() != 2 || !org.lwjgl.input.Mouse.getEventButtonState()) continue;
                    this.handlePickBlock();
                    continue;
                }
                if (this.currentScreen == null) continue;
                this.currentScreen.onMouseEvent();
            }
            if (this.attackCooldown > 0) {
                --this.attackCooldown;
            }
            while (Keyboard.next()) {
                this.player.updateKey(Keyboard.getEventKey(), Keyboard.getEventKeyState());
                if (!Keyboard.getEventKeyState()) continue;
                if (Keyboard.getEventKey() == 87) {
                    this.toggleFullscreen();
                    continue;
                }
                if (this.currentScreen != null) {
                    this.currentScreen.onKeyboardEvent();
                } else {
                    if (Keyboard.getEventKey() == 1) {
                        this.pauseGame();
                    }
                    if (Keyboard.getEventKey() == 31 && Keyboard.isKeyDown(61)) {
                        this.forceResourceReload();
                    }
                    if (Keyboard.getEventKey() == 59) {
                        boolean bl = this.options.hideHud = !this.options.hideHud;
                    }
                    if (Keyboard.getEventKey() == 61) {
                        boolean bl = this.options.debugHud = !this.options.debugHud;
                    }
                    if (Keyboard.getEventKey() == 63) {
                        boolean bl = this.options.thirdPerson = !this.options.thirdPerson;
                    }
                    if (Keyboard.getEventKey() == 66) {
                        boolean bl = this.options.cinematicMode = !this.options.cinematicMode;
                    }
                    if (Keyboard.getEventKey() == this.options.inventoryKey.code) {
                        this.setScreen(new InventoryScreen(this.player));
                    }
                    if (Keyboard.getEventKey() == this.options.dropKey.code) {
                        this.player.dropSelectedItem();
                    }
                    if (this.isWorldRemote() && Keyboard.getEventKey() == this.options.chatKey.code) {
                        this.setScreen(new ChatScreen());
                    }
                }
                for (int i = 0; i < 9; ++i) {
                    if (Keyboard.getEventKey() != 2 + i) continue;
                    this.player.inventory.selectedSlot = i;
                }
                if (Keyboard.getEventKey() != this.options.fogKey.code) continue;
                this.options.setInt(Option.RENDER_DISTANCE, Keyboard.isKeyDown(42) || Keyboard.isKeyDown(54) ? -1 : 1);
            }
            if (this.currentScreen == null) {
                if (org.lwjgl.input.Mouse.isButtonDown(0) && (float)(this.ticksPlayed - this.lastClickTicks) >= this.timer.tps / 4.0f && this.focused) {
                    this.handleMouseClick(0);
                    this.lastClickTicks = this.ticksPlayed;
                }
                if (org.lwjgl.input.Mouse.isButtonDown(1) && (float)(this.ticksPlayed - this.lastClickTicks) >= this.timer.tps / 4.0f && this.focused) {
                    this.handleMouseClick(1);
                    this.lastClickTicks = this.ticksPlayed;
                }
            }
            this.handleMouseDown(0, this.currentScreen == null && org.lwjgl.input.Mouse.isButtonDown(0) && this.focused);
        }
        if (this.world != null) {
            if (this.player != null) {
                ++this.joinPlayerCounter;
                if (this.joinPlayerCounter == 30) {
                    this.joinPlayerCounter = 0;
                    this.world.loadChunksNearEntity(this.player);
                }
            }
            this.world.difficulty = this.options.difficulty;
            if (this.world.isRemote) {
                this.world.difficulty = 3;
            }
            if (!this.paused) {
                this.gameRenderer.updateCamera();
            }
            if (!this.paused) {
                this.worldRenderer.tick();
            }
            if (!this.paused) {
                if (this.world.lightningTicksLeft > 0) {
                    --this.world.lightningTicksLeft;
                }
                this.world.tickEntities();
            }
            if (!this.paused || this.isWorldRemote()) {
                this.world.allowSpawning(this.options.difficulty > 0, true);
                this.world.tick();
            }
            if (!this.paused && this.world != null) {
                this.world.displayTick(MathHelper.floor(this.player.x), MathHelper.floor(this.player.y), MathHelper.floor(this.player.z));
            }
            if (!this.paused) {
                this.particleManager.removeDeadParticles();
            }
        }
        this.lastTickTime = System.currentTimeMillis();
    }

    private void forceResourceReload() {
        System.out.println("FORCING RELOAD!");
        this.soundManager = new SoundManager();
        this.soundManager.loadSounds(this.options);
        this.resourceDownloadThread.reload();
    }

    public boolean isWorldRemote() {
        return this.world != null && this.world.isRemote;
    }

    public void startGame(String worldName, String name, long seed) {
        this.setWorld(null);
        System.gc();
        if (this.worldStorageSource.needsConversion(worldName)) {
            this.convertAndSaveWorld(worldName, name);
        } else {
            WorldStorage worldStorage = this.worldStorageSource.getSaveLoader(worldName, false);
            World world = null;
            world = new World(worldStorage, name, seed);
            if (world.newWorld) {
                this.stats.increment(Stats.CREATE_WORLD, 1);
                this.stats.increment(Stats.START_GAME, 1);
                this.setWorld(world, "Generating level");
            } else {
                this.stats.increment(Stats.LOAD_WORLD, 1);
                this.stats.increment(Stats.START_GAME, 1);
                this.setWorld(world, "Loading level");
            }
        }
    }

    public void changeDimension() {
        System.out.println("Toggling dimension!!");
        this.player.dimensionId = this.player.dimensionId == -1 ? 0 : -1;
        this.world.remove(this.player);
        this.player.dead = false;
        double d = this.player.x;
        double d2 = this.player.z;
        double d3 = 8.0;
        if (this.player.dimensionId == -1) {
            this.player.setPositionAndAnglesKeepPrevAngles(d /= d3, this.player.y, d2 /= d3, this.player.yaw, this.player.pitch);
            if (this.player.isAlive()) {
                this.world.updateEntity(this.player, false);
            }
            World world = null;
            world = new World(this.world, Dimension.fromId(-1));
            this.setWorld(world, "Entering the Nether", this.player);
        } else {
            this.player.setPositionAndAnglesKeepPrevAngles(d *= d3, this.player.y, d2 *= d3, this.player.yaw, this.player.pitch);
            if (this.player.isAlive()) {
                this.world.updateEntity(this.player, false);
            }
            World world = null;
            world = new World(this.world, Dimension.fromId(0));
            this.setWorld(world, "Leaving the Nether", this.player);
        }
        this.player.world = this.world;
        if (this.player.isAlive()) {
            this.player.setPositionAndAnglesKeepPrevAngles(d, this.player.y, d2, this.player.yaw, this.player.pitch);
            this.world.updateEntity(this.player, false);
            new PortalForcer().moveToPortal(this.world, this.player);
        }
    }

    public void setWorld(World world) {
        this.setWorld(world, "");
    }

    public void setWorld(World world, String message) {
        this.setWorld(world, message, null);
    }

    public void setWorld(World world, String message, PlayerEntity player) {
        this.stats.syncStats();
        this.stats.save();
        this.camera = null;
        this.progressRenderer.progressStart(message);
        this.progressRenderer.progressStage("");
        this.soundManager.playStreaming(null, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (this.world != null) {
            this.world.savingProgress(this.progressRenderer);
        }
        this.world = world;
        if (world != null) {
            ChunkSource chunkSource;
            this.interactionManager.setWorld(world);
            if (!this.isWorldRemote()) {
                if (player == null) {
                    this.player = (ClientPlayerEntity)world.getPlayerForProxy(ClientPlayerEntity.class);
                }
            } else if (this.player != null) {
                this.player.teleportTop();
                if (world != null) {
                    world.spawnEntity(this.player);
                }
            }
            if (!world.isRemote) {
                this.prepareWorld(message);
            }
            if (this.player == null) {
                this.player = (ClientPlayerEntity)this.interactionManager.createPlayer(world);
                this.player.teleportTop();
                this.interactionManager.preparePlayer(this.player);
            }
            this.player.input = new KeyboardInput(this.options);
            if (this.worldRenderer != null) {
                this.worldRenderer.setWorld(world);
            }
            if (this.particleManager != null) {
                this.particleManager.setWorld(world);
            }
            this.interactionManager.preparePlayerRespawn(this.player);
            if (player != null) {
                world.saveWorldData();
            }
            if ((chunkSource = world.getChunkSource()) instanceof LegacyChunkCache) {
                LegacyChunkCache legacyChunkCache = (LegacyChunkCache)chunkSource;
                int n = MathHelper.floor((int)this.player.x) >> 4;
                int n2 = MathHelper.floor((int)this.player.z) >> 4;
                legacyChunkCache.setSpawnPoint(n, n2);
            }
            world.addPlayer(this.player);
            if (world.newWorld) {
                world.savingProgress(this.progressRenderer);
            }
            this.camera = this.player;
        } else {
            this.player = null;
        }
        System.gc();
        this.lastTickTime = 0L;
    }

    private void convertAndSaveWorld(String worldName, String name) {
        this.progressRenderer.progressStart("Converting World to " + this.worldStorageSource.getName());
        this.progressRenderer.progressStage("This may take a while :)");
        this.worldStorageSource.convert(worldName, this.progressRenderer);
        this.startGame(worldName, name, 0L);
    }

    private void prepareWorld(String worldName) {
        this.progressRenderer.progressStart(worldName);
        this.progressRenderer.progressStage("Building terrain");
        int n = 128;
        int n2 = 0;
        int n3 = n * 2 / 16 + 1;
        n3 *= n3;
        ChunkSource chunkSource = this.world.getChunkSource();
        Vec3i vec3i = this.world.getSpawnPos();
        if (this.player != null) {
            vec3i.x = (int)this.player.x;
            vec3i.z = (int)this.player.z;
        }
        if (chunkSource instanceof LegacyChunkCache) {
            LegacyChunkCache legacyChunkCache = (LegacyChunkCache)chunkSource;
            legacyChunkCache.setSpawnPoint(vec3i.x >> 4, vec3i.z >> 4);
        }
        for (int i = -n; i <= n; i += 16) {
            for (int j = -n; j <= n; j += 16) {
                this.progressRenderer.progressStagePercentage(n2++ * 100 / n3);
                this.world.getBlockId(vec3i.x + i, 64, vec3i.z + j);
                while (this.world.doLightingUpdates()) {
                }
            }
        }
        this.progressRenderer.progressStage("Simulating world for a bit");
        n3 = 2000;
        this.world.tickChunks();
    }

    public void loadResource(String path, File file) {
        int n = path.indexOf("/");
        String string = path.substring(0, n);
        path = path.substring(n + 1);
        if (string.equalsIgnoreCase("sound")) {
            this.soundManager.loadSound(path, file);
        } else if (string.equalsIgnoreCase("newsound")) {
            this.soundManager.loadSound(path, file);
        } else if (string.equalsIgnoreCase("streaming")) {
            this.soundManager.loadStreaming(path, file);
        } else if (string.equalsIgnoreCase("music")) {
            this.soundManager.loadMusic(path, file);
        } else if (string.equalsIgnoreCase("newmusic")) {
            this.soundManager.loadMusic(path, file);
        }
    }

    public OpenGlCapabilities getOpenGlCapabilities() {
        return this.openGlCapabilities;
    }

    public String getRenderChunkDebugInfo() {
        return this.worldRenderer.getChunkDebugInfo();
    }

    public String getRenderEntityDebugInfo() {
        return this.worldRenderer.getEntityDebugInfo();
    }

    public String getChunkSourceDebugInfo() {
        return this.world.getDebugInfo();
    }

    public String getWorldDebugInfo() {
        return "P: " + this.particleManager.toString() + ". T: " + this.world.getEntityCount();
    }

    public void respawnPlayer(boolean worldSpawn, int dimension) {
        ChunkSource chunkSource;
        if (!this.world.isRemote && !this.world.dimension.hasWorldSpawn()) {
            this.changeDimension();
        }
        Vec3i vec3i = null;
        Vec3i vec3i2 = null;
        boolean bl = true;
        if (this.player != null && !worldSpawn && (vec3i = this.player.getSpawnPos()) != null && (vec3i2 = PlayerEntity.findRespawnPosition(this.world, vec3i)) == null) {
            this.player.sendMessage("tile.bed.notValid");
        }
        if (vec3i2 == null) {
            vec3i2 = this.world.getSpawnPos();
            bl = false;
        }
        if ((chunkSource = this.world.getChunkSource()) instanceof LegacyChunkCache) {
            LegacyChunkCache legacyChunkCache = (LegacyChunkCache)chunkSource;
            legacyChunkCache.setSpawnPoint(vec3i2.x >> 4, vec3i2.z >> 4);
        }
        this.world.updateSpawnPosition();
        this.world.updateEntityLists();
        int n = 0;
        if (this.player != null) {
            n = this.player.id;
            this.world.remove(this.player);
        }
        this.camera = null;
        this.player = (ClientPlayerEntity)this.interactionManager.createPlayer(this.world);
        this.player.dimensionId = dimension;
        this.camera = this.player;
        this.player.teleportTop();
        if (bl) {
            this.player.setSpawnPos(vec3i);
            this.player.setPositionAndAnglesKeepPrevAngles((float)vec3i2.x + 0.5f, (float)vec3i2.y + 0.1f, (float)vec3i2.z + 0.5f, 0.0f, 0.0f);
        }
        this.interactionManager.preparePlayer(this.player);
        this.world.addPlayer(this.player);
        this.player.input = new KeyboardInput(this.options);
        this.player.id = n;
        this.player.spawn();
        this.interactionManager.preparePlayerRespawn(this.player);
        this.prepareWorld("Respawning");
        if (this.currentScreen instanceof DeathScreen) {
            this.setScreen(null);
        }
    }

    public static void start(String username, String sessionId) {
        Minecraft.startAndConnect(username, sessionId, null);
    }

    public static void startAndConnect(String username, String sessionId, String server) {
        boolean bl = false;
        String string = username;
        Frame frame = new Frame("Minecraft");
        Canvas canvas = new Canvas();
        frame.setLayout(new BorderLayout());
        frame.add((Component)canvas, "Center");
        canvas.setPreferredSize(new java.awt.Dimension(854, 480));
        frame.pack();
        frame.setLocationRelativeTo(null);
        RunnableMinecraft runnableMinecraft = new RunnableMinecraft(frame, canvas, null, 854, 480, bl, frame);
        Thread thread = new Thread((Runnable)runnableMinecraft, "Minecraft main thread");
        thread.setPriority(10);
        runnableMinecraft.hostAddress = "www.minecraft.net";
        runnableMinecraft.session = string != null && sessionId != null ? new Session(string, sessionId) : new Session("Player" + System.currentTimeMillis() % 1000L, "");
        if (server != null) {
            String[] stringArray = server.split(":");
            runnableMinecraft.setStartupServer(stringArray[0], Integer.parseInt(stringArray[1]));
        }
        frame.setVisible(true);
        frame.addWindowListener(runnableMinecraft.new class_638(thread));
        thread.start();
    }

    public ClientNetworkHandler getNetworkHandler() {
        if (this.player instanceof MultiplayerClientPlayerEntity) {
            return ((MultiplayerClientPlayerEntity)this.player).networkHandler;
        }
        return null;
    }

    public static void main(String[] args) {
        String string = null;
        String string2 = null;
        string = "Player" + System.currentTimeMillis() % 1000L;
        if (args.length > 0) {
            string = args[0];
        }
        string2 = "-";
        if (args.length > 1) {
            string2 = args[1];
        }
        Minecraft.start(string, string2);
    }

    public static boolean isDisplayGui() {
        return INSTANCE == null || !Minecraft.INSTANCE.options.hideHud;
    }

    public static boolean isFancyGraphicsEnabled() {
        return INSTANCE != null && Minecraft.INSTANCE.options.fancyGraphics;
    }

    public static boolean isAmbientOcclusionEnabled() {
        return INSTANCE != null && Minecraft.INSTANCE.options.ao;
    }

    public static boolean isDebugProfilerEnabled() {
        return INSTANCE != null && Minecraft.INSTANCE.options.debugHud;
    }

    public boolean isCommand(String message) {
        if (message.startsWith("/")) {
            // empty if block
        }
        return false;
    }

    static {
        frameTimes = new long[512];
        tickTimes = new long[512];
        frameTimeIndex = 0;
        failedSessionCheckTime = 0L;
        runDirectoryCache = null;
    }

    @Environment(value=EnvType.CLIENT)
    public class TimerHackThread
    extends Thread {
        public TimerHackThread(String string) {
            super(string);
            this.setDaemon(true);
            this.start();
        }

        public void run() {
            while (Minecraft.this.running) {
                try {
                    Thread.sleep(Integer.MAX_VALUE);
                }
                catch (InterruptedException interruptedException) {}
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public class AchievementStatFormatter
    implements net.minecraft.stat.achievement.AchievementStatFormatter {
        public String format(String text) {
            return String.format(text, Keyboard.getKeyName(Minecraft.this.options.inventoryKey.code));
        }
    }

    @Environment(value=EnvType.CLIENT)
    public final class class_638
    extends WindowAdapter {
        final /* synthetic */ Thread capturedThread;

        public class_638(Thread thread) {
            this.capturedThread = thread;
        }

        public void windowClosing(WindowEvent windowEvent) {
            Minecraft.this.scheduleStop();
            try {
                this.capturedThread.join();
            }
            catch (InterruptedException interruptedException) {
                interruptedException.printStackTrace();
            }
            System.exit(0);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public class SessionCheckThread
    extends Thread {
        public void run() {
            try {
                HttpURLConnection httpURLConnection = (HttpURLConnection)new URL("https://login.minecraft.net/session?name=" + Minecraft.this.session.username + "&session=" + Minecraft.this.session.sessionId).openConnection();
                httpURLConnection.connect();
                if (httpURLConnection.getResponseCode() == 400) {
                    failedSessionCheckTime = System.currentTimeMillis();
                }
                httpURLConnection.disconnect();
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public final class RunnableMinecraft
    extends Minecraft {
        final /* synthetic */ Frame frame;

        public RunnableMinecraft(Component component, Canvas canvas, MinecraftApplet applet, int i, int j, boolean bl, Frame frame) {
            this.frame = frame;
            super(component, canvas, applet, i, j, bl);
        }

        public void handleCrash(CrashReport crashReport) {
            this.frame.removeAll();
            this.frame.add((Component)new CrashReportPanel(crashReport), "Center");
            this.frame.validate();
        }
    }
}

