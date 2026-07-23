/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.entity;

import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class NoteBlockBlockEntity
extends BlockEntity {
    public byte note = 0;
    public boolean powered = false;

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putByte("note", this.note);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.note = nbt.getByte("note");
        if (this.note < 0) {
            this.note = 0;
        }
        if (this.note > 24) {
            this.note = (byte)24;
        }
    }

    public void cycleNote() {
        this.note = (byte)((this.note + 1) % 25);
        this.markDirty();
    }

    public void playNote(World world, int x, int y, int z) {
        if (world.getMaterial(x, y + 1, z) != Material.AIR) {
            return;
        }
        Material material = world.getMaterial(x, y - 1, z);
        int n = 0;
        if (material == Material.STONE) {
            n = 1;
        }
        if (material == Material.SAND) {
            n = 2;
        }
        if (material == Material.GLASS) {
            n = 3;
        }
        if (material == Material.WOOD) {
            n = 4;
        }
        world.playNoteBlockActionAt(x, y, z, n, this.note);
    }
}

