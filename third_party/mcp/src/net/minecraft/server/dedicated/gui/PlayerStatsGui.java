/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.dedicated.gui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JComponent;
import javax.swing.Timer;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.Connection;

@Environment(value=EnvType.SERVER)
public class PlayerStatsGui
extends JComponent {
    private int[] memoryUsePercentage = new int[256];
    private int memoryUsage = 0;
    private String[] lines = new String[10];

    public PlayerStatsGui() {
        this.setPreferredSize(new Dimension(256, 196));
        this.setMinimumSize(new Dimension(256, 196));
        this.setMaximumSize(new Dimension(256, 196));
        new Timer(500, new ActionListener(){

            public void actionPerformed(ActionEvent actionEvent) {
                PlayerStatsGui.this.update();
            }
        }).start();
        this.setBackground(Color.BLACK);
    }

    private void update() {
        long l = Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory();
        System.gc();
        this.lines[0] = "Memory use: " + l / 1024L / 1024L + " mb (" + Runtime.getRuntime().freeMemory() * 100L / Runtime.getRuntime().maxMemory() + "% free)";
        this.lines[1] = "Threads: " + Connection.READ_THREAD_COUNTER + " + " + Connection.WRITE_THREAD_COUNTER;
        this.memoryUsePercentage[this.memoryUsage++ & 0xFF] = (int)(l * 100L / Runtime.getRuntime().maxMemory());
        this.repaint();
    }

    public void paint(Graphics graphics) {
        int n;
        graphics.setColor(new Color(0xFFFFFF));
        graphics.fillRect(0, 0, 256, 192);
        for (n = 0; n < 256; ++n) {
            int n2 = this.memoryUsePercentage[n + this.memoryUsage & 0xFF];
            graphics.setColor(new Color(n2 + 28 << 16));
            graphics.fillRect(n, 100 - n2, 1, n2);
        }
        graphics.setColor(Color.BLACK);
        for (n = 0; n < this.lines.length; ++n) {
            String string = this.lines[n];
            if (string == null) continue;
            graphics.drawString(string, 32, 116 + n * 16);
        }
    }
}

