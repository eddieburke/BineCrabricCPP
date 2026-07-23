/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.Packet;
import net.minecraft.world.World;

public class NetworkSyncedItem
extends Item {
    protected NetworkSyncedItem(int i) {
        super(i);
    }

    @Environment(value=EnvType.SERVER)
    public boolean isNetworkSynced() {
        return true;
    }

    @Environment(value=EnvType.SERVER)
    public Packet getUpdatePacket(ItemStack stack, World world, PlayerEntity player) {
        return null;
    }
}

