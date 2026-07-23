/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.world.storage;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.LoadingDisplay;
import net.minecraft.world.WorldProperties;
import net.minecraft.world.storage.WorldStorage;

public interface WorldStorageSource {
    @Environment(value=EnvType.CLIENT)
    public String getName();

    @Environment(value=EnvType.CLIENT)
    public WorldStorage getSaveLoader(String var1, boolean var2);

    @Environment(value=EnvType.CLIENT)
    public List getAll();

    @Environment(value=EnvType.CLIENT)
    public void flush();

    @Environment(value=EnvType.CLIENT)
    public WorldProperties getWorldProperties(String var1);

    @Environment(value=EnvType.CLIENT)
    public void delete(String var1);

    @Environment(value=EnvType.CLIENT)
    public void rename(String var1, String var2);

    public boolean needsConversion(String var1);

    public boolean convert(String var1, LoadingDisplay var2);
}

