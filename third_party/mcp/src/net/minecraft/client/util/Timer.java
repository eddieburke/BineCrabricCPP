/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.util;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class Timer {
    public float tps;
    private double timeSec;
    public int ticksThisFrame;
    public float partialTick;
    public float tpsScale = 1.0f;
    public float tickDelta = 0.0f;
    private long prevTickTime;
    private long prevCorrectionTime;
    private long cooldownTickTime;
    private double tickTimeCorrection = 1.0;

    public Timer(float tps) {
        this.tps = tps;
        this.prevTickTime = System.currentTimeMillis();
        this.prevCorrectionTime = System.nanoTime() / 1000000L;
    }

    public void advance() {
        long l = System.currentTimeMillis();
        long l2 = l - this.prevTickTime;
        long l3 = System.nanoTime() / 1000000L;
        double d = (double)l3 / 1000.0;
        if (l2 > 1000L) {
            this.timeSec = d;
        } else if (l2 < 0L) {
            this.timeSec = d;
        } else {
            this.cooldownTickTime += l2;
            if (this.cooldownTickTime > 1000L) {
                long l4 = l3 - this.prevCorrectionTime;
                double d2 = (double)this.cooldownTickTime / (double)l4;
                this.tickTimeCorrection += (d2 - this.tickTimeCorrection) * (double)0.2f;
                this.prevCorrectionTime = l3;
                this.cooldownTickTime = 0L;
            }
            if (this.cooldownTickTime < 0L) {
                this.prevCorrectionTime = l3;
            }
        }
        this.prevTickTime = l;
        double d3 = (d - this.timeSec) * this.tickTimeCorrection;
        this.timeSec = d;
        if (d3 < 0.0) {
            d3 = 0.0;
        }
        if (d3 > 1.0) {
            d3 = 1.0;
        }
        this.tickDelta = (float)((double)this.tickDelta + d3 * (double)this.tpsScale * (double)this.tps);
        this.ticksThisFrame = (int)this.tickDelta;
        this.tickDelta -= (float)this.ticksThisFrame;
        if (this.ticksThisFrame > 10) {
            this.ticksThisFrame = 10;
        }
        this.partialTick = this.tickDelta;
    }
}

