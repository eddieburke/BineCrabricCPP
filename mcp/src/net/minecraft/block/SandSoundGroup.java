/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.sound.BlockSoundGroup;

final class SandSoundGroup
extends BlockSoundGroup {
    SandSoundGroup(String string, float f, float g) {
        super(string, f, g);
    }

    @Environment(value=EnvType.CLIENT)
    public String getBreakSound() {
        return "step.gravel";
    }
}

