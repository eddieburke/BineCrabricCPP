/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.resource.pack;

import java.io.InputStream;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;

@Environment(value=EnvType.CLIENT)
public abstract class TexturePack {
    public String name;
    public String descriptionLine1;
    public String descriptionLine2;
    public String key;

    public void open() {
    }

    public void close() {
    }

    public void load(Minecraft minecraft) {
    }

    public void unload(Minecraft minecraft) {
    }

    public void bindIcon(Minecraft minecraft) {
    }

    public InputStream getResource(String path) {
        return TexturePack.class.getResourceAsStream(path);
    }
}

