/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.input;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.input.Input;
import net.minecraft.client.option.GameOptions;
import net.minecraft.entity.player.PlayerEntity;

@Environment(value=EnvType.CLIENT)
public class KeyboardInput
extends Input {
    private boolean[] keys = new boolean[10];
    private GameOptions options;

    public KeyboardInput(GameOptions options) {
        this.options = options;
    }

    public void updateKey(int key, boolean keyDown) {
        int n = -1;
        if (key == this.options.forwardKey.code) {
            n = 0;
        }
        if (key == this.options.backKey.code) {
            n = 1;
        }
        if (key == this.options.leftKey.code) {
            n = 2;
        }
        if (key == this.options.rightKey.code) {
            n = 3;
        }
        if (key == this.options.jumpKey.code) {
            n = 4;
        }
        if (key == this.options.sneakKey.code) {
            n = 5;
        }
        if (n >= 0) {
            this.keys[n] = keyDown;
        }
    }

    public void reset() {
        for (int i = 0; i < 10; ++i) {
            this.keys[i] = false;
        }
    }

    public void update(PlayerEntity player) {
        this.movementSideways = 0.0f;
        this.movementForward = 0.0f;
        if (this.keys[0]) {
            this.movementForward += 1.0f;
        }
        if (this.keys[1]) {
            this.movementForward -= 1.0f;
        }
        if (this.keys[2]) {
            this.movementSideways += 1.0f;
        }
        if (this.keys[3]) {
            this.movementSideways -= 1.0f;
        }
        this.jumping = this.keys[4];
        this.sneaking = this.keys[5];
        if (this.sneaking) {
            this.movementSideways = (float)((double)this.movementSideways * 0.3);
            this.movementForward = (float)((double)this.movementForward * 0.3);
        }
    }
}

