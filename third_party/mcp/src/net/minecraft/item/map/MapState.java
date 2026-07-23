/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item.map;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.PersistentState;
import net.minecraft.world.World;

public class MapState
extends PersistentState {
    public int centerX;
    public int centerZ;
    public byte dimension;
    public byte scale;
    public byte[] colors = new byte[16384];
    public int inventoryTicks;
    public List updateTrackers = new ArrayList();
    private Map updateTrackersByPlayer = new HashMap();
    public List icons = new ArrayList();

    public MapState(String string) {
        super(string);
    }

    public void readNbt(NbtCompound nbt) {
        this.dimension = nbt.getByte("dimension");
        this.centerX = nbt.getInt("xCenter");
        this.centerZ = nbt.getInt("zCenter");
        this.scale = nbt.getByte("scale");
        if (this.scale < 0) {
            this.scale = 0;
        }
        if (this.scale > 4) {
            this.scale = (byte)4;
        }
        int n = nbt.getShort("width");
        int n2 = nbt.getShort("height");
        if (n == 128 && n2 == 128) {
            this.colors = nbt.getByteArray("colors");
        } else {
            byte[] byArray = nbt.getByteArray("colors");
            this.colors = new byte[16384];
            int n3 = (128 - n) / 2;
            int n4 = (128 - n2) / 2;
            for (int i = 0; i < n2; ++i) {
                int n5 = i + n4;
                if (n5 < 0 && n5 >= 128) continue;
                for (int j = 0; j < n; ++j) {
                    int n6 = j + n3;
                    if (n6 < 0 && n6 >= 128) continue;
                    this.colors[n6 + n5 * 128] = byArray[j + i * n];
                }
            }
        }
    }

    public void writeNbt(NbtCompound nbt) {
        nbt.putByte("dimension", this.dimension);
        nbt.putInt("xCenter", this.centerX);
        nbt.putInt("zCenter", this.centerZ);
        nbt.putByte("scale", this.scale);
        nbt.putShort("width", (short)128);
        nbt.putShort("height", (short)128);
        nbt.putByteArray("colors", this.colors);
    }

    public void update(PlayerEntity player, ItemStack stack) {
        if (!this.updateTrackersByPlayer.containsKey(player)) {
            PlayerUpdateTracker playerUpdateTracker = new PlayerUpdateTracker(player);
            this.updateTrackersByPlayer.put(player, playerUpdateTracker);
            this.updateTrackers.add(playerUpdateTracker);
        }
        this.icons.clear();
        for (int i = 0; i < this.updateTrackers.size(); ++i) {
            PlayerUpdateTracker playerUpdateTracker = (PlayerUpdateTracker)this.updateTrackers.get(i);
            if (playerUpdateTracker.player.dead || !playerUpdateTracker.player.inventory.contains(stack)) {
                this.updateTrackersByPlayer.remove(playerUpdateTracker.player);
                this.updateTrackers.remove(playerUpdateTracker);
                continue;
            }
            float f = (float)(playerUpdateTracker.player.x - (double)this.centerX) / (float)(1 << this.scale);
            float f2 = (float)(playerUpdateTracker.player.z - (double)this.centerZ) / (float)(1 << this.scale);
            int n = 64;
            int n2 = 64;
            if (!(f >= (float)(-n)) || !(f2 >= (float)(-n2)) || !(f <= (float)n) || !(f2 <= (float)n2)) continue;
            byte by = 0;
            byte by2 = (byte)((double)(f * 2.0f) + 0.5);
            byte by3 = (byte)((double)(f2 * 2.0f) + 0.5);
            byte by4 = (byte)((double)(player.yaw * 16.0f / 360.0f) + 0.5);
            if (this.dimension < 0) {
                int n3 = this.inventoryTicks / 10;
                by4 = (byte)(n3 * n3 * 34187121 + n3 * 121 >> 15 & 0xF);
            }
            if (playerUpdateTracker.player.dimensionId != this.dimension) continue;
            this.icons.add(new MapIcon(by, by2, by3, by4));
        }
    }

    @Environment(value=EnvType.SERVER)
    public byte[] getPlayerMarkerPacket(ItemStack stack, World world, PlayerEntity player) {
        PlayerUpdateTracker playerUpdateTracker = (PlayerUpdateTracker)this.updateTrackersByPlayer.get(player);
        if (playerUpdateTracker == null) {
            return null;
        }
        byte[] byArray = playerUpdateTracker.getUpdateData(stack);
        return byArray;
    }

    public void markDirty(int x, int startZ, int endZ) {
        super.markDirty();
        for (int i = 0; i < this.updateTrackers.size(); ++i) {
            PlayerUpdateTracker playerUpdateTracker = (PlayerUpdateTracker)this.updateTrackers.get(i);
            if (playerUpdateTracker.startZ[x] < 0 || playerUpdateTracker.startZ[x] > startZ) {
                playerUpdateTracker.startZ[x] = startZ;
            }
            if (playerUpdateTracker.endZ[x] >= 0 && playerUpdateTracker.endZ[x] >= endZ) continue;
            playerUpdateTracker.endZ[x] = endZ;
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void readUpdateData(byte[] updateData) {
        if (updateData[0] == 0) {
            int n = updateData[1] & 0xFF;
            int n2 = updateData[2] & 0xFF;
            for (int i = 0; i < updateData.length - 3; ++i) {
                this.colors[(i + n2) * 128 + n] = updateData[i + 3];
            }
            this.markDirty();
        } else if (updateData[0] == 1) {
            this.icons.clear();
            for (int i = 0; i < (updateData.length - 1) / 3; ++i) {
                byte by = (byte)(updateData[i * 3 + 1] % 16);
                byte by2 = updateData[i * 3 + 2];
                byte by3 = updateData[i * 3 + 3];
                byte by4 = (byte)(updateData[i * 3 + 1] / 16);
                this.icons.add(new MapIcon(by, by2, by3, by4));
            }
        }
    }

    public class MapIcon {
        public byte type;
        public byte x;
        public byte z;
        public byte rotation;

        public MapIcon(byte type, byte x, byte z, byte rotation) {
            this.type = type;
            this.x = x;
            this.z = z;
            this.rotation = rotation;
        }
    }

    public class PlayerUpdateTracker {
        public final PlayerEntity player;
        public int[] startZ = new int[128];
        public int[] endZ = new int[128];
        private int nextDirtyPixel = 0;
        private int colorsUpdateInterval = 0;
        @Environment(value=EnvType.SERVER)
        private byte[] iconsData;

        public PlayerUpdateTracker(PlayerEntity player) {
            this.player = player;
            for (int i = 0; i < this.startZ.length; ++i) {
                this.startZ[i] = 0;
                this.endZ[i] = 127;
            }
        }

        @Environment(value=EnvType.SERVER)
        public byte[] getUpdateData(ItemStack stack) {
            int n;
            if (--this.colorsUpdateInterval < 0) {
                this.colorsUpdateInterval = 4;
                byte[] byArray = new byte[MapState.this.icons.size() * 3 + 1];
                byArray[0] = 1;
                for (n = 0; n < MapState.this.icons.size(); ++n) {
                    MapIcon mapIcon = (MapIcon)MapState.this.icons.get(n);
                    byArray[n * 3 + 1] = (byte)(mapIcon.type + (mapIcon.rotation & 0xF) * 16);
                    byArray[n * 3 + 2] = mapIcon.x;
                    byArray[n * 3 + 3] = mapIcon.z;
                }
                n = 1;
                if (this.iconsData == null || this.iconsData.length != byArray.length) {
                    n = 0;
                } else {
                    for (int i = 0; i < byArray.length; ++i) {
                        if (byArray[i] == this.iconsData[i]) continue;
                        n = 0;
                        break;
                    }
                }
                if (n == 0) {
                    this.iconsData = byArray;
                    return byArray;
                }
            }
            for (int i = 0; i < 10; ++i) {
                n = this.nextDirtyPixel * 11 % 128;
                ++this.nextDirtyPixel;
                if (this.startZ[n] < 0) continue;
                int n2 = this.endZ[n] - this.startZ[n] + 1;
                int n3 = this.startZ[n];
                byte[] byArray = new byte[n2 + 3];
                byArray[0] = 0;
                byArray[1] = (byte)n;
                byArray[2] = (byte)n3;
                for (int j = 0; j < byArray.length - 3; ++j) {
                    byArray[j + 3] = MapState.this.colors[(j + n3) * 128 + n];
                }
                this.endZ[n] = -1;
                this.startZ[n] = -1;
                return byArray;
            }
            return null;
        }
    }
}

