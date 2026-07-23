/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import java.awt.Canvas;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.io.IOException;
import javax.imageio.ImageIO;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.CrashReportPanel;

@Environment(value=EnvType.CLIENT)
class LogoCanvas
extends Canvas {
    private BufferedImage logo;

    public LogoCanvas() {
        try {
            this.logo = ImageIO.read(CrashReportPanel.class.getResource("/gui/logo.png"));
        }
        catch (IOException iOException) {
            // empty catch block
        }
        int n = 100;
        this.setPreferredSize(new Dimension(n, n));
        this.setMinimumSize(new Dimension(n, n));
    }

    public void paint(Graphics graphics) {
        super.paint(graphics);
        graphics.drawImage(this.logo, this.getWidth() / 2 - this.logo.getWidth() / 2, 32, null);
    }
}

