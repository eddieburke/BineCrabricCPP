/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.entity;

import net.minecraft.block.entity.BlockEntity;
import net.minecraft.nbt.NbtCompound;

public class JukeboxBlockEntity
extends BlockEntity {
    public int recordId;

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.recordId = nbt.getInt("Record");
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        if (this.recordId > 0) {
            nbt.putInt("Record", this.recordId);
        }
    }
}

