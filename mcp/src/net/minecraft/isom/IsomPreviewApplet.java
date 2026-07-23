/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.isom;

import java.applet.Applet;
import java.awt.BorderLayout;
import java.awt.Component;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.isom.IsomPreviewCanvas;

@Environment(value=EnvType.CLIENT)
public class IsomPreviewApplet
extends Applet {
    private IsomPreviewCanvas canvas = new IsomPreviewCanvas();

    public IsomPreviewApplet() {
        this.setLayout(new BorderLayout());
        this.add((Component)this.canvas, "Center");
    }

    public void start() {
        this.canvas.start();
    }

    public void stop() {
        this.canvas.stop();
    }
}

