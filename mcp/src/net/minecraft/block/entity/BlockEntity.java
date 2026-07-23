/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block.entity;

import java.util.HashMap;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.entity.ChestBlockEntity;
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.block.entity.JukeboxBlockEntity;
import net.minecraft.block.entity.MobSpawnerBlockEntity;
import net.minecraft.block.entity.NoteBlockBlockEntity;
import net.minecraft.block.entity.PistonBlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.network.packet.Packet;
import net.minecraft.world.World;

public class BlockEntity {
    private static Map idToClass = new HashMap();
    private static Map classToId = new HashMap();
    public World world;
    public int x;
    public int y;
    public int z;
    protected boolean removed;

    private static void create(Class blockEntityClass, String id) {
        if (classToId.containsKey(id)) {
            throw new IllegalArgumentException("Duplicate id: " + id);
        }
        idToClass.put(id, blockEntityClass);
        classToId.put(blockEntityClass, id);
    }

    public void readNbt(NbtCompound nbt) {
        this.x = nbt.getInt("x");
        this.y = nbt.getInt("y");
        this.z = nbt.getInt("z");
    }

    public void writeNbt(NbtCompound nbt) {
        String string = (String)classToId.get(this.getClass());
        if (string == null) {
            throw new RuntimeException(this.getClass() + " is missing a mapping! This is a bug!");
        }
        nbt.putString("id", string);
        nbt.putInt("x", this.x);
        nbt.putInt("y", this.y);
        nbt.putInt("z", this.z);
    }

    public void tick() {
    }

    public static BlockEntity createFromNbt(NbtCompound nbt) {
        BlockEntity blockEntity = null;
        try {
            Class clazz = (Class)idToClass.get(nbt.getString("id"));
            if (clazz != null) {
                blockEntity = (BlockEntity)clazz.newInstance();
            }
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
        if (blockEntity != null) {
            blockEntity.readNbt(nbt);
        } else {
            System.out.println("Skipping TileEntity with id " + nbt.getString("id"));
        }
        return blockEntity;
    }

    public int getPushedBlockData() {
        return this.world.getBlockMeta(this.x, this.y, this.z);
    }

    public void markDirty() {
        if (this.world != null) {
            this.world.updateBlockEntity(this.x, this.y, this.z, this);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public double distanceFrom(double x, double y, double z) {
        double d = (double)this.x + 0.5 - x;
        double d2 = (double)this.y + 0.5 - y;
        double d3 = (double)this.z + 0.5 - z;
        return d * d + d2 * d2 + d3 * d3;
    }

    @Environment(value=EnvType.CLIENT)
    public Block getBlock() {
        return Block.BLOCKS[this.world.getBlockId(this.x, this.y, this.z)];
    }

    @Environment(value=EnvType.SERVER)
    public Packet createUpdatePacket() {
        return null;
    }

    public boolean isRemoved() {
        return this.removed;
    }

    public void markRemoved() {
        this.removed = true;
    }

    public void cancelRemoval() {
        this.removed = false;
    }

    static {
        BlockEntity.create(FurnaceBlockEntity.class, "Furnace");
        BlockEntity.create(ChestBlockEntity.class, "Chest");
        BlockEntity.create(JukeboxBlockEntity.class, "RecordPlayer");
        BlockEntity.create(DispenserBlockEntity.class, "Trap");
        BlockEntity.create(SignBlockEntity.class, "Sign");
        BlockEntity.create(MobSpawnerBlockEntity.class, "MobSpawner");
        BlockEntity.create(NoteBlockBlockEntity.class, "Music");
        BlockEntity.create(PistonBlockEntity.class, "Piston");
    }
}

