/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import java.awt.Component;
import java.nio.IntBuffer;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.util.GlAllocationUtils;
import org.lwjgl.LWJGLException;
import org.lwjgl.input.Cursor;

@Environment(value=EnvType.CLIENT)
public class Mouse {
    private Component parent;
    private Cursor cursor;
    public int deltaX;
    public int deltaY;
    private int unused = 10;

    public Mouse(Component parent) {
        this.parent = parent;
        IntBuffer intBuffer = GlAllocationUtils.allocateIntBuffer(1);
        intBuffer.put(0);
        intBuffer.flip();
        IntBuffer intBuffer2 = GlAllocationUtils.allocateIntBuffer(1024);
        try {
            this.cursor = new Cursor(32, 32, 16, 16, 1, intBuffer2, intBuffer);
        }
        catch (LWJGLException lWJGLException) {
            lWJGLException.printStackTrace();
        }
    }

    public void lockCursor() {
        org.lwjgl.input.Mouse.setGrabbed(true);
        this.deltaX = 0;
        this.deltaY = 0;
    }

    public void unlockCursor() {
        org.lwjgl.input.Mouse.setCursorPosition(this.parent.getWidth() / 2, this.parent.getHeight() / 2);
        org.lwjgl.input.Mouse.setGrabbed(false);
    }

    public void poll() {
        this.deltaX = org.lwjgl.input.Mouse.getDX();
        this.deltaY = org.lwjgl.input.Mouse.getDY();
    }
}

