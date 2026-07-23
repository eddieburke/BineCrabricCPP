/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.InteractionManager;
import net.minecraft.client.Minecraft;
import net.minecraft.client.util.Session;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class TestInteractionManager
extends InteractionManager {
    public TestInteractionManager(Minecraft minecraft) {
        super(minecraft);
        this.noTick = true;
    }

    public void preparePlayerRespawn(PlayerEntity player) {
        for (int i = 0; i < 9; ++i) {
            if (player.inventory.main[i] == null) {
                this.minecraft.player.inventory.main[i] = new ItemStack((Block)Session.CREATIVE_INVENTORY.get(i));
                continue;
            }
            this.minecraft.player.inventory.main[i].count = 1;
        }
    }

    public boolean canBeRendered() {
        return false;
    }

    public void setWorld(World world) {
        super.setWorld(world);
    }

    public void tick() {
    }
}

