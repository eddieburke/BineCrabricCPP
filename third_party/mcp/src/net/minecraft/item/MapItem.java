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
import net.minecraft.block.Block;
import net.minecraft.block.MapColor;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.NetworkSyncedItem;
import net.minecraft.item.map.MapState;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.s2c.play.MapUpdateS2CPacket;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public class MapItem
extends NetworkSyncedItem {
    protected MapItem(int i) {
        super(i);
        this.setMaxCount(1);
    }

    @Environment(value=EnvType.CLIENT)
    public static MapState getMapState(short mapId, World world) {
        String string = "map_" + mapId;
        MapState mapState = (MapState)world.getOrCreateState(MapState.class, "map_" + mapId);
        if (mapState == null) {
            int n = world.getIdCount("map");
            string = "map_" + n;
            mapState = new MapState(string);
            world.setState(string, mapState);
        }
        return mapState;
    }

    public MapState getSavedMapState(ItemStack stack, World world) {
        String string = "map_" + stack.getDamage();
        MapState mapState = (MapState)world.getOrCreateState(MapState.class, "map_" + stack.getDamage());
        if (mapState == null) {
            stack.setDamage(world.getIdCount("map"));
            string = "map_" + stack.getDamage();
            mapState = new MapState(string);
            mapState.centerX = world.getProperties().getSpawnX();
            mapState.centerZ = world.getProperties().getSpawnZ();
            mapState.scale = (byte)3;
            mapState.dimension = (byte)world.dimension.id;
            mapState.markDirty();
            world.setState(string, mapState);
        }
        return mapState;
    }

    public void update(World world, Entity entity, MapState map) {
        if (world.dimension.id != map.dimension) {
            return;
        }
        int n = 128;
        int n2 = 128;
        int n3 = 1 << map.scale;
        int n4 = map.centerX;
        int n5 = map.centerZ;
        int n6 = MathHelper.floor(entity.x - (double)n4) / n3 + n / 2;
        int n7 = MathHelper.floor(entity.z - (double)n5) / n3 + n2 / 2;
        int n8 = 128 / n3;
        if (world.dimension.hasCeiling) {
            n8 /= 2;
        }
        ++map.inventoryTicks;
        for (int i = n6 - n8 + 1; i < n6 + n8; ++i) {
            if ((i & 0xF) != (map.inventoryTicks & 0xF)) continue;
            int n9 = 255;
            int n10 = 0;
            double d = 0.0;
            for (int j = n7 - n8 - 1; j < n7 + n8; ++j) {
                byte by;
                byte by2;
                int n11;
                int n12;
                int n13;
                int n14;
                int n15;
                if (i < 0 || j < -1 || i >= n || j >= n2) continue;
                int n16 = i - n6;
                int n17 = j - n7;
                boolean bl = n16 * n16 + n17 * n17 > (n8 - 2) * (n8 - 2);
                int n18 = (n4 / n3 + i - n / 2) * n3;
                int n19 = (n5 / n3 + j - n2 / 2) * n3;
                int n20 = 0;
                int n21 = 0;
                int n22 = 0;
                int[] nArray = new int[256];
                Chunk chunk = world.getChunkFromPos(n18, n19);
                int n23 = n18 & 0xF;
                int n24 = n19 & 0xF;
                int n25 = 0;
                double d2 = 0.0;
                if (world.dimension.hasCeiling) {
                    n15 = n18 + n19 * 231871;
                    if (((n15 = n15 * n15 * 31287121 + n15 * 11) >> 20 & 1) == 0) {
                        int n26 = Block.DIRT.id;
                        nArray[n26] = nArray[n26] + 10;
                    } else {
                        int n27 = Block.STONE.id;
                        nArray[n27] = nArray[n27] + 10;
                    }
                    d2 = 100.0;
                } else {
                    for (n15 = 0; n15 < n3; ++n15) {
                        for (n14 = 0; n14 < n3; ++n14) {
                            n13 = chunk.getHeight(n15 + n23, n14 + n24) + 1;
                            int n28 = 0;
                            if (n13 > 1) {
                                n12 = 0;
                                do {
                                    n12 = 1;
                                    n28 = chunk.getBlockId(n15 + n23, n13 - 1, n14 + n24);
                                    if (n28 == 0) {
                                        n12 = 0;
                                    } else if (n13 > 0 && n28 > 0 && Block.BLOCKS[n28].material.mapColor == MapColor.CLEAR) {
                                        n12 = 0;
                                    }
                                    if (n12 != 0) continue;
                                    n28 = chunk.getBlockId(n15 + n23, --n13 - 1, n14 + n24);
                                } while (n12 == 0);
                                if (n28 != 0 && Block.BLOCKS[n28].material.isFluid()) {
                                    n11 = n13 - 1;
                                    int n29 = 0;
                                    do {
                                        n29 = chunk.getBlockId(n15 + n23, n11--, n14 + n24);
                                        ++n25;
                                    } while (n11 > 0 && n29 != 0 && Block.BLOCKS[n29].material.isFluid());
                                }
                            }
                            d2 += (double)n13 / (double)(n3 * n3);
                            int n30 = n28;
                            nArray[n30] = nArray[n30] + 1;
                        }
                    }
                }
                n25 /= n3 * n3;
                n20 /= n3 * n3;
                n21 /= n3 * n3;
                n22 /= n3 * n3;
                n15 = 0;
                n14 = 0;
                for (n13 = 0; n13 < 256; ++n13) {
                    if (nArray[n13] <= n15) continue;
                    n14 = n13;
                    n15 = nArray[n13];
                }
                double d3 = (d2 - d) * 4.0 / (double)(n3 + 4) + ((double)(i + j & 1) - 0.5) * 0.4;
                n12 = 1;
                if (d3 > 0.6) {
                    n12 = 2;
                }
                if (d3 < -0.6) {
                    n12 = 0;
                }
                n11 = 0;
                if (n14 > 0) {
                    MapColor mapColor = Block.BLOCKS[n14].material.mapColor;
                    if (mapColor == MapColor.BLUE) {
                        d3 = (double)n25 * 0.1 + (double)(i + j & 1) * 0.2;
                        n12 = 1;
                        if (d3 < 0.5) {
                            n12 = 2;
                        }
                        if (d3 > 0.9) {
                            n12 = 0;
                        }
                    }
                    n11 = mapColor.id;
                }
                d = d2;
                if (j < 0 || n16 * n16 + n17 * n17 >= n8 * n8 || bl && (i + j & 1) == 0 || (by2 = map.colors[i + j * n]) == (by = (byte)(n11 * 4 + n12))) continue;
                if (n9 > j) {
                    n9 = j;
                }
                if (n10 < j) {
                    n10 = j;
                }
                map.colors[i + j * n] = by;
            }
            if (n9 > n10) continue;
            map.markDirty(i, n9, n10);
        }
    }

    public void inventoryTick(ItemStack stack, World world, Entity entity, int slot, boolean selected) {
        if (world.isRemote) {
            return;
        }
        MapState mapState = this.getSavedMapState(stack, world);
        if (entity instanceof PlayerEntity) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            mapState.update(playerEntity, stack);
        }
        if (selected) {
            this.update(world, entity, mapState);
        }
    }

    public void onCraft(ItemStack stack, World world, PlayerEntity player) {
        stack.setDamage(world.getIdCount("map"));
        String string = "map_" + stack.getDamage();
        MapState mapState = new MapState(string);
        world.setState(string, mapState);
        mapState.centerX = MathHelper.floor(player.x);
        mapState.centerZ = MathHelper.floor(player.z);
        mapState.scale = (byte)3;
        mapState.dimension = (byte)world.dimension.id;
        mapState.markDirty();
    }

    @Environment(value=EnvType.SERVER)
    public Packet getUpdatePacket(ItemStack stack, World world, PlayerEntity player) {
        byte[] byArray = this.getSavedMapState(stack, world).getPlayerMarkerPacket(stack, world, player);
        if (byArray == null) {
            return null;
        }
        return new MapUpdateS2CPacket((short)Item.MAP.id, (short)stack.getDamage(), byArray);
    }
}

