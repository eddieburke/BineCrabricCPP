/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.play.UpdateSignPacket;

public class SignBlockEntity
extends BlockEntity {
    public String[] texts = new String[]{"", "", "", ""};
    public int currentRow = -1;
    private boolean editable = true;

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putString("Text1", this.texts[0]);
        nbt.putString("Text2", this.texts[1]);
        nbt.putString("Text3", this.texts[2]);
        nbt.putString("Text4", this.texts[3]);
    }

    public void readNbt(NbtCompound nbt) {
        this.editable = false;
        super.readNbt(nbt);
        for (int i = 0; i < 4; ++i) {
            this.texts[i] = nbt.getString("Text" + (i + 1));
            if (this.texts[i].length() <= 15) continue;
            this.texts[i] = this.texts[i].substring(0, 15);
        }
    }

    @Environment(value=EnvType.SERVER)
    public Packet createUpdatePacket() {
        String[] stringArray = new String[4];
        for (int i = 0; i < 4; ++i) {
            stringArray[i] = this.texts[i];
        }
        return new UpdateSignPacket(this.x, this.y, this.z, stringArray);
    }

    @Environment(value=EnvType.SERVER)
    public boolean isEditable() {
        return this.editable;
    }

    @Environment(value=EnvType.SERVER)
    public void setEditable(boolean editable) {
        this.editable = editable;
    }
}

