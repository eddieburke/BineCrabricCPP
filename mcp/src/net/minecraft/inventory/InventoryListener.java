/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.inventory;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.inventory.SimpleInventory;

@Environment(value=EnvType.CLIENT)
public interface InventoryListener {
    public void onInventoryChanged(SimpleInventory var1);
}

