/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.ParticlesGui;

@Environment(value=EnvType.CLIENT)
public class GuiParticle {
    private static Random RANDOM = new Random();
    public double x;
    public double y;
    public double lastX;
    public double lastY;
    public double velocityX;
    public double velocityY;
    public double velocityChange;
    public boolean removed;
    public int age;
    public int lifetime;
    public double r;
    public double g;
    public double b;
    public double a;
    public double lastR;
    public double lastG;
    public double lastB;
    public double lastA;

    public void tickPosition(ParticlesGui gui) {
        this.x += this.velocityX;
        this.y += this.velocityY;
        this.velocityX *= this.velocityChange;
        this.velocityY *= this.velocityChange;
        this.velocityY += 0.1;
        if (++this.age > this.lifetime) {
            this.remove();
        }
        this.a = 2.0 - (double)this.age / (double)this.lifetime * 2.0;
        if (this.a > 1.0) {
            this.a = 1.0;
        }
        this.a *= this.a;
        this.a *= 0.5;
    }

    public void tickColor() {
        this.lastR = this.r;
        this.lastG = this.g;
        this.lastB = this.b;
        this.lastA = this.a;
        this.lastX = this.x;
        this.lastY = this.y;
    }

    public void remove() {
        this.removed = true;
    }
}

