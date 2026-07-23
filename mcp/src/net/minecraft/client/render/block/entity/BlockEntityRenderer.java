/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.block.entity.BlockEntityRenderDispatcher;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public abstract class BlockEntityRenderer {
    protected BlockEntityRenderDispatcher dispatcher;

    public abstract void render(BlockEntity var1, double var2, double var4, double var6, float var8);

    protected void bindTexture(String path) {
        TextureManager textureManager = this.dispatcher.textureManager;
        textureManager.bindTexture(textureManager.getTextureId(path));
    }

    public void setDispatcher(BlockEntityRenderDispatcher dispatcher) {
        this.dispatcher = dispatcher;
    }

    public void setWorld(World world) {
    }

    public TextRenderer getTextRenderer() {
        return this.dispatcher.getTextRenderer();
    }
}

