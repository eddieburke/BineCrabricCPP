/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.event.listener;

import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;

public interface GameEventListener {
    public void blockUpdate(int var1, int var2, int var3);

    public void setBlocksDirty(int var1, int var2, int var3, int var4, int var5, int var6);

    public void playSound(String var1, double var2, double var4, double var6, float var8, float var9);

    public void addParticle(String var1, double var2, double var4, double var6, double var8, double var10, double var12);

    public void notifyEntityAdded(Entity var1);

    public void notifyEntityRemoved(Entity var1);

    public void notifyAmbientDarknessChanged();

    public void playStreaming(String var1, int var2, int var3, int var4);

    public void updateBlockEntity(int var1, int var2, int var3, BlockEntity var4);

    public void worldEvent(PlayerEntity var1, int var2, int var3, int var4, int var5, int var6);
}

