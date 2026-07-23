/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class KeyBinding {
    public String translationKey;
    public int code;

    public KeyBinding(String translationKey, int code) {
        this.translationKey = translationKey;
        this.code = code;
    }
}

