/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.isom;

import java.awt.Canvas;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferStrategy;
import java.io.File;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.isom.IsomChunkRenderer;
import net.minecraft.isom.IsomRenderChunk;
import net.minecraft.isom.OS;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.World;
import net.minecraft.world.storage.AlphaWorldStorage;

@Environment(value=EnvType.CLIENT)
public class IsomPreviewCanvas
extends Canvas
implements KeyListener,
MouseListener,
MouseMotionListener,
Runnable {
    private int frames = 0;
    private int scale = 2;
    private boolean renderDebugInfo = true;
    private World world;
    private File workingDir;
    private boolean running = true;
    private List chunksToRender = Collections.synchronizedList(new LinkedList());
    private IsomRenderChunk[][] chunks = new IsomRenderChunk[64][64];
    private int offsetX;
    private int offsetZ;
    private int mouseX;
    private int mouseY;

    public File getWorkingDirectory() {
        if (this.workingDir == null) {
            this.workingDir = this.getWorkingDirectory("minecraft");
        }
        return this.workingDir;
    }

    public File getWorkingDirectory(String name) {
        File file;
        String string = System.getProperty("user.home", ".");
        switch (IsomPreviewCanvas.getOs()) {
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

    private static OS getOs() {
        String string = System.getProperty("os.name").toLowerCase();
        if (string.contains("win")) {
            return OS.WINDOWS;
        }
        if (string.contains("mac")) {
            return OS.MACOS;
        }
        if (string.contains("solaris")) {
            return OS.SOLARIS;
        }
        if (string.contains("sunos")) {
            return OS.SOLARIS;
        }
        if (string.contains("linux")) {
            return OS.LINUX;
        }
        if (string.contains("unix")) {
            return OS.LINUX;
        }
        return OS.UNKNOWN;
    }

    public IsomPreviewCanvas() {
        this.workingDir = this.getWorkingDirectory();
        for (int i = 0; i < 64; ++i) {
            for (int j = 0; j < 64; ++j) {
                this.chunks[i][j] = new IsomRenderChunk(null, i, j);
            }
        }
        this.addMouseListener(this);
        this.addMouseMotionListener(this);
        this.addKeyListener(this);
        this.setFocusable(true);
        this.requestFocus();
        this.setBackground(Color.red);
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void openWorld(String saveName) {
        this.offsetZ = 0;
        this.offsetX = 0;
        this.world = new World(new AlphaWorldStorage(new File(this.workingDir, "saves"), saveName, false), saveName, new Random().nextLong());
        this.world.ambientDarkness = 0;
        List list = this.chunksToRender;
        synchronized (list) {
            this.chunksToRender.clear();
            for (int i = 0; i < 64; ++i) {
                for (int j = 0; j < 64; ++j) {
                    this.chunks[i][j].init(this.world, i, j);
                }
            }
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private void setAmbientDarkness(int ambientDarkness) {
        List list = this.chunksToRender;
        synchronized (list) {
            this.world.ambientDarkness = ambientDarkness;
            this.chunksToRender.clear();
            for (int i = 0; i < 64; ++i) {
                for (int j = 0; j < 64; ++j) {
                    this.chunks[i][j].init(this.world, i, j);
                }
            }
        }
    }

    public void start() {
        new Thread(){

            public void run() {
                while (IsomPreviewCanvas.this.running) {
                    IsomPreviewCanvas.this.update();
                    try {
                        Thread.sleep(1L);
                    }
                    catch (Exception exception) {}
                }
            }
        }.start();
        for (int i = 0; i < 8; ++i) {
            new Thread(this).start();
        }
    }

    public void stop() {
        this.running = false;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private IsomRenderChunk getChunk(int x, int y) {
        int n = x & 0x3F;
        int n2 = y & 0x3F;
        IsomRenderChunk isomRenderChunk = this.chunks[n][n2];
        if (isomRenderChunk.chunkX == x && isomRenderChunk.chunkZ == y) {
            return isomRenderChunk;
        }
        List list = this.chunksToRender;
        synchronized (list) {
            this.chunksToRender.remove(isomRenderChunk);
        }
        isomRenderChunk.init(x, y);
        return isomRenderChunk;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void run() {
        IsomChunkRenderer isomChunkRenderer = new IsomChunkRenderer();
        while (this.running) {
            IsomRenderChunk isomRenderChunk = null;
            List list = this.chunksToRender;
            synchronized (list) {
                if (this.chunksToRender.size() > 0) {
                    isomRenderChunk = (IsomRenderChunk)this.chunksToRender.remove(0);
                }
            }
            if (isomRenderChunk != null) {
                if (this.frames - isomRenderChunk.lastVisible < 2) {
                    isomChunkRenderer.render(isomRenderChunk);
                    this.repaint();
                } else {
                    isomRenderChunk.toBeRendered = false;
                }
            }
            try {
                Thread.sleep(2L);
            }
            catch (InterruptedException interruptedException) {
                interruptedException.printStackTrace();
            }
        }
    }

    public void update(Graphics graphics) {
    }

    public void paint(Graphics graphics) {
    }

    public void update() {
        BufferStrategy bufferStrategy = this.getBufferStrategy();
        if (bufferStrategy == null) {
            this.createBufferStrategy(2);
            return;
        }
        this.render((Graphics2D)bufferStrategy.getDrawGraphics());
        bufferStrategy.show();
    }

    public void render(Graphics2D graphics) {
        int n;
        Object object;
        ++this.frames;
        AffineTransform affineTransform = graphics.getTransform();
        graphics.setClip(0, 0, this.getWidth(), this.getHeight());
        graphics.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
        graphics.translate(this.getWidth() / 2, this.getHeight() / 2);
        graphics.scale(this.scale, this.scale);
        graphics.translate(this.offsetX, this.offsetZ);
        if (this.world != null) {
            object = this.world.getSpawnPos();
            graphics.translate(-(((Vec3i)object).x + ((Vec3i)object).z), -(-((Vec3i)object).x + ((Vec3i)object).z) + 64);
        }
        object = graphics.getClipBounds();
        graphics.setColor(new Color(-15724512));
        graphics.fillRect(((Rectangle)object).x, ((Rectangle)object).y, ((Rectangle)object).width, ((Rectangle)object).height);
        int n2 = 16;
        int n3 = 3;
        int n4 = ((Rectangle)object).x / n2 / 2 - 2 - n3;
        int n5 = (((Rectangle)object).x + ((Rectangle)object).width) / n2 / 2 + 1 + n3;
        int n6 = ((Rectangle)object).y / n2 - 1 - n3 * 2;
        int n7 = (((Rectangle)object).y + ((Rectangle)object).height + 16 + 128) / n2 + 1 + n3 * 2;
        for (n = n6; n <= n7; ++n) {
            for (int i = n4; i <= n5; ++i) {
                int n8 = i - (n >> 1);
                int n9 = i + (n + 1 >> 1);
                IsomRenderChunk isomRenderChunk = this.getChunk(n8, n9);
                isomRenderChunk.lastVisible = this.frames;
                if (!isomRenderChunk.rendered) {
                    if (isomRenderChunk.toBeRendered) continue;
                    isomRenderChunk.toBeRendered = true;
                    this.chunksToRender.add(isomRenderChunk);
                    continue;
                }
                isomRenderChunk.toBeRendered = false;
                if (isomRenderChunk.empty) continue;
                int n10 = i * n2 * 2 + (n & 1) * n2;
                int n11 = n * n2 - 128 - 16;
                graphics.drawImage((Image)isomRenderChunk.image, n10, n11, null);
            }
        }
        if (this.renderDebugInfo) {
            graphics.setTransform(affineTransform);
            n = this.getHeight() - 32 - 4;
            graphics.setColor(new Color(Integer.MIN_VALUE, true));
            graphics.fillRect(4, this.getHeight() - 32 - 4, this.getWidth() - 8, 32);
            graphics.setColor(Color.WHITE);
            String string = "F1 - F5: load levels   |   0-9: Set time of day   |   Space: return to spawn   |   Double click: zoom   |   Escape: hide this text";
            graphics.drawString(string, this.getWidth() / 2 - graphics.getFontMetrics().stringWidth(string) / 2, n + 20);
        }
        graphics.dispose();
    }

    public void mouseDragged(MouseEvent mouseEvent) {
        int n = mouseEvent.getX() / this.scale;
        int n2 = mouseEvent.getY() / this.scale;
        this.offsetX += n - this.mouseX;
        this.offsetZ += n2 - this.mouseY;
        this.mouseX = n;
        this.mouseY = n2;
        this.repaint();
    }

    public void mouseMoved(MouseEvent mouseEvent) {
    }

    public void mouseClicked(MouseEvent mouseEvent) {
        if (mouseEvent.getClickCount() == 2) {
            this.scale = 3 - this.scale;
            this.repaint();
        }
    }

    public void mouseEntered(MouseEvent mouseEvent) {
    }

    public void mouseExited(MouseEvent mouseEvent) {
    }

    public void mousePressed(MouseEvent mouseEvent) {
        int n = mouseEvent.getX() / this.scale;
        int n2 = mouseEvent.getY() / this.scale;
        this.mouseX = n;
        this.mouseY = n2;
    }

    public void mouseReleased(MouseEvent mouseEvent) {
    }

    public void keyPressed(KeyEvent keyEvent) {
        if (keyEvent.getKeyCode() == 48) {
            this.setAmbientDarkness(11);
        }
        if (keyEvent.getKeyCode() == 49) {
            this.setAmbientDarkness(10);
        }
        if (keyEvent.getKeyCode() == 50) {
            this.setAmbientDarkness(9);
        }
        if (keyEvent.getKeyCode() == 51) {
            this.setAmbientDarkness(7);
        }
        if (keyEvent.getKeyCode() == 52) {
            this.setAmbientDarkness(6);
        }
        if (keyEvent.getKeyCode() == 53) {
            this.setAmbientDarkness(5);
        }
        if (keyEvent.getKeyCode() == 54) {
            this.setAmbientDarkness(3);
        }
        if (keyEvent.getKeyCode() == 55) {
            this.setAmbientDarkness(2);
        }
        if (keyEvent.getKeyCode() == 56) {
            this.setAmbientDarkness(1);
        }
        if (keyEvent.getKeyCode() == 57) {
            this.setAmbientDarkness(0);
        }
        if (keyEvent.getKeyCode() == 112) {
            this.openWorld("World1");
        }
        if (keyEvent.getKeyCode() == 113) {
            this.openWorld("World2");
        }
        if (keyEvent.getKeyCode() == 114) {
            this.openWorld("World3");
        }
        if (keyEvent.getKeyCode() == 115) {
            this.openWorld("World4");
        }
        if (keyEvent.getKeyCode() == 116) {
            this.openWorld("World5");
        }
        if (keyEvent.getKeyCode() == 32) {
            this.offsetZ = 0;
            this.offsetX = 0;
        }
        if (keyEvent.getKeyCode() == 27) {
            this.renderDebugInfo = !this.renderDebugInfo;
        }
        this.repaint();
    }

    public void keyReleased(KeyEvent keyEvent) {
    }

    public void keyTyped(KeyEvent keyEvent) {
    }
}

