/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import java.applet.Applet;
import java.awt.BorderLayout;
import java.awt.Canvas;
import java.awt.Component;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.CrashReportPanel;
import net.minecraft.client.Minecraft;
import net.minecraft.client.util.Session;
import net.minecraft.util.crash.CrashReport;

@Environment(value=EnvType.CLIENT)
public class MinecraftApplet
extends Applet {
    private Canvas canvas;
    private Minecraft minecraft;
    private Thread thread = null;

    public void init() {
        this.canvas = new AppletCanvas();
        boolean bl = false;
        if (this.getParameter("fullscreen") != null) {
            bl = this.getParameter("fullscreen").equalsIgnoreCase("true");
        }
        this.minecraft = new AppletMinecraft(this, this.canvas, this, this.getWidth(), this.getHeight(), bl);
        this.minecraft.hostAddress = this.getDocumentBase().getHost();
        if (this.getDocumentBase().getPort() > 0) {
            this.minecraft.hostAddress = this.minecraft.hostAddress + ":" + this.getDocumentBase().getPort();
        }
        if (this.getParameter("username") != null && this.getParameter("sessionid") != null) {
            this.minecraft.session = new Session(this.getParameter("username"), this.getParameter("sessionid"));
            System.out.println("Setting user: " + this.minecraft.session.username + ", " + this.minecraft.session.sessionId);
            if (this.getParameter("mppass") != null) {
                this.minecraft.session.mpPass = this.getParameter("mppass");
            }
        } else {
            this.minecraft.session = new Session("Player", "");
        }
        if (this.getParameter("server") != null && this.getParameter("port") != null) {
            this.minecraft.setStartupServer(this.getParameter("server"), Integer.parseInt(this.getParameter("port")));
        }
        this.minecraft.isApplet = true;
        this.setLayout(new BorderLayout());
        this.add((Component)this.canvas, "Center");
        this.canvas.setFocusable(true);
        this.validate();
    }

    public void startThread() {
        if (this.thread != null) {
            return;
        }
        this.thread = new Thread((Runnable)this.minecraft, "Minecraft main thread");
        this.thread.start();
    }

    public void start() {
        if (this.minecraft != null) {
            this.minecraft.paused = false;
        }
    }

    public void stop() {
        if (this.minecraft != null) {
            this.minecraft.paused = true;
        }
    }

    public void destroy() {
        this.stopThread();
    }

    public void stopThread() {
        if (this.thread == null) {
            return;
        }
        this.minecraft.scheduleStop();
        try {
            this.thread.join(10000L);
        }
        catch (InterruptedException interruptedException) {
            try {
                this.minecraft.stop();
            }
            catch (Exception exception) {
                exception.printStackTrace();
            }
        }
        this.thread = null;
    }

    public void clearMemory() {
        this.canvas = null;
        this.minecraft = null;
        this.thread = null;
        try {
            this.removeAll();
            this.validate();
        }
        catch (Exception exception) {
            // empty catch block
        }
    }

    @Environment(value=EnvType.CLIENT)
    public class AppletMinecraft
    extends Minecraft {
        public AppletMinecraft(Component component, Canvas canvas, MinecraftApplet applet, int i, int j, boolean bl) {
            super(component, canvas, applet, i, j, bl);
        }

        public void handleCrash(CrashReport crashReport) {
            MinecraftApplet.this.removeAll();
            MinecraftApplet.this.setLayout(new BorderLayout());
            MinecraftApplet.this.add((Component)new CrashReportPanel(crashReport), "Center");
            MinecraftApplet.this.validate();
        }
    }

    @Environment(value=EnvType.CLIENT)
    public class AppletCanvas
    extends Canvas {
        public synchronized void addNotify() {
            super.addNotify();
            MinecraftApplet.this.startThread();
        }

        public synchronized void removeNotify() {
            MinecraftApplet.this.stopThread();
            super.removeNotify();
        }
    }
}

