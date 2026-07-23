/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.sound;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

public class BlockSoundGroup {
    public final String soundName;
    public final float volume;
    public final float pitch;

    public BlockSoundGroup(String soundName, float volume, float pitch) {
        this.soundName = soundName;
        this.volume = volume;
        this.pitch = pitch;
    }

    public float getVolume() {
        return this.volume;
    }

    public float getPitch() {
        return this.pitch;
    }

    @Environment(value=EnvType.CLIENT)
    public String getBreakSound() {
        return "step." + this.soundName;
    }

    public String getSound() {
        return "step." + this.soundName;
    }
}

