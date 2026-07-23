/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.screen;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandler;

public interface ScreenHandlerListener {
    @Environment(value=EnvType.SERVER)
    public void onContentsUpdate(ScreenHandler var1, List var2);

    public void onSlotUpdate(ScreenHandler var1, int var2, ItemStack var3);

    public void onPropertyUpdate(ScreenHandler var1, int var2, int var3);
}

