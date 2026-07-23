/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block;

import net.minecraft.block.Block;
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.NoteBlockBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.world.World;

public class NoteBlock
extends BlockWithEntity {
    public NoteBlock(int id) {
        super(id, 74, Material.WOOD);
    }

    public int getTexture(int side) {
        return this.textureId;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower()) {
            boolean bl = world.canTransferPower(x, y, z);
            NoteBlockBlockEntity noteBlockBlockEntity = (NoteBlockBlockEntity)world.getBlockEntity(x, y, z);
            if (noteBlockBlockEntity.powered != bl) {
                if (bl) {
                    noteBlockBlockEntity.playNote(world, x, y, z);
                }
                noteBlockBlockEntity.powered = bl;
            }
        }
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return true;
        }
        NoteBlockBlockEntity noteBlockBlockEntity = (NoteBlockBlockEntity)world.getBlockEntity(x, y, z);
        noteBlockBlockEntity.cycleNote();
        noteBlockBlockEntity.playNote(world, x, y, z);
        return true;
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        if (world.isRemote) {
            return;
        }
        NoteBlockBlockEntity noteBlockBlockEntity = (NoteBlockBlockEntity)world.getBlockEntity(x, y, z);
        noteBlockBlockEntity.playNote(world, x, y, z);
    }

    protected BlockEntity createBlockEntity() {
        return new NoteBlockBlockEntity();
    }

    public void onBlockAction(World world, int x, int y, int z, int data1, int data2) {
        float f = (float)Math.pow(2.0, (double)(data2 - 12) / 12.0);
        String string = "harp";
        if (data1 == 1) {
            string = "bd";
        }
        if (data1 == 2) {
            string = "snare";
        }
        if (data1 == 3) {
            string = "hat";
        }
        if (data1 == 4) {
            string = "bassattack";
        }
        world.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, "note." + string, 3.0f, f);
        world.addParticle("note", (double)x + 0.5, (double)y + 1.2, (double)z + 0.5, (double)data2 / 24.0, 0.0, 0.0);
    }
}

