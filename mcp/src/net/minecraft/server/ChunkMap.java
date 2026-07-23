/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDataS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkStatusUpdateS2CPacket;
import net.minecraft.server.MinecraftServer;
import net.minecraft.util.LongObjectHashMap;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.world.ServerWorld;

@Environment(value=EnvType.SERVER)
public class ChunkMap {
    public List players = new ArrayList();
    private LongObjectHashMap chunkMapping = new LongObjectHashMap();
    private List chunksToUpdate = new ArrayList();
    private MinecraftServer server;
    private int dimensionId;
    private int viewDistance;
    private final int[][] DIRECTIONS = new int[][]{{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    public ChunkMap(MinecraftServer server, int dimensionId, int viewRadius) {
        if (viewRadius > 15) {
            throw new IllegalArgumentException("Too big view radius!");
        }
        if (viewRadius < 3) {
            throw new IllegalArgumentException("Too small view radius!");
        }
        this.viewDistance = viewRadius;
        this.server = server;
        this.dimensionId = dimensionId;
    }

    public ServerWorld getWorld() {
        return this.server.getWorld(this.dimensionId);
    }

    public void updateChunks() {
        for (int i = 0; i < this.chunksToUpdate.size(); ++i) {
            ((TrackedChunk)this.chunksToUpdate.get(i)).updateChunk();
        }
        this.chunksToUpdate.clear();
    }

    private TrackedChunk getOrCreateChunk(int chunkX, int chunkZ, boolean createIfAbsent) {
        long l = (long)chunkX + Integer.MAX_VALUE | (long)chunkZ + Integer.MAX_VALUE << 32;
        TrackedChunk trackedChunk = (TrackedChunk)this.chunkMapping.get(l);
        if (trackedChunk == null && createIfAbsent) {
            trackedChunk = new TrackedChunk(chunkX, chunkZ);
            this.chunkMapping.put(l, trackedChunk);
        }
        return trackedChunk;
    }

    public void markBlockForUpdate(int x, int y, int z) {
        int n = x >> 4;
        int n2 = z >> 4;
        TrackedChunk trackedChunk = this.getOrCreateChunk(n, n2, false);
        if (trackedChunk != null) {
            trackedChunk.updatePlayerChunks(x & 0xF, y, z & 0xF);
        }
    }

    public void addPlayer(ServerPlayerEntity player) {
        int n;
        int n2 = (int)player.x >> 4;
        int n3 = (int)player.z >> 4;
        player.lastX = player.x;
        player.lastZ = player.z;
        int n4 = 0;
        int n5 = this.viewDistance;
        int n6 = 0;
        int n7 = 0;
        this.getOrCreateChunk(n2, n3, true).addPlayer(player);
        for (n = 1; n <= n5 * 2; ++n) {
            for (int i = 0; i < 2; ++i) {
                int[] nArray = this.DIRECTIONS[n4++ % 4];
                for (int j = 0; j < n; ++j) {
                    this.getOrCreateChunk(n2 + (n6 += nArray[0]), n3 + (n7 += nArray[1]), true).addPlayer(player);
                }
            }
        }
        n4 %= 4;
        for (n = 0; n < n5 * 2; ++n) {
            this.getOrCreateChunk(n2 + (n6 += this.DIRECTIONS[n4][0]), n3 + (n7 += this.DIRECTIONS[n4][1]), true).addPlayer(player);
        }
        this.players.add(player);
    }

    public void removePlayer(ServerPlayerEntity player) {
        int n = (int)player.lastX >> 4;
        int n2 = (int)player.lastZ >> 4;
        for (int i = n - this.viewDistance; i <= n + this.viewDistance; ++i) {
            for (int j = n2 - this.viewDistance; j <= n2 + this.viewDistance; ++j) {
                TrackedChunk trackedChunk = this.getOrCreateChunk(i, j, false);
                if (trackedChunk == null) continue;
                trackedChunk.removePlayer(player);
            }
        }
        this.players.remove(player);
    }

    private boolean isWithinViewDistance(int chunkX, int chunkZ, int centerX, int centerZ) {
        int n = chunkX - centerX;
        int n2 = chunkZ - centerZ;
        if (n < -this.viewDistance || n > this.viewDistance) {
            return false;
        }
        return n2 >= -this.viewDistance && n2 <= this.viewDistance;
    }

    public void updatePlayerChunks(ServerPlayerEntity player) {
        int n = (int)player.x >> 4;
        int n2 = (int)player.z >> 4;
        double d = player.lastX - player.x;
        double d2 = player.lastZ - player.z;
        double d3 = d * d + d2 * d2;
        if (d3 < 64.0) {
            return;
        }
        int n3 = (int)player.lastX >> 4;
        int n4 = (int)player.lastZ >> 4;
        int n5 = n - n3;
        int n6 = n2 - n4;
        if (n5 == 0 && n6 == 0) {
            return;
        }
        for (int i = n - this.viewDistance; i <= n + this.viewDistance; ++i) {
            for (int j = n2 - this.viewDistance; j <= n2 + this.viewDistance; ++j) {
                TrackedChunk trackedChunk;
                if (!this.isWithinViewDistance(i, j, n3, n4)) {
                    this.getOrCreateChunk(i, j, true).addPlayer(player);
                }
                if (this.isWithinViewDistance(i - n5, j - n6, n, n2) || (trackedChunk = this.getOrCreateChunk(i - n5, j - n6, false)) == null) continue;
                trackedChunk.removePlayer(player);
            }
        }
        player.lastX = player.x;
        player.lastZ = player.z;
    }

    public int getBlockViewDistance() {
        return this.viewDistance * 16 - 16;
    }

    @Environment(value=EnvType.SERVER)
    class TrackedChunk {
        private List players = new ArrayList();
        private int chunkX;
        private int chunkZ;
        private ChunkPos chunkPos;
        private short[] dirtyBlocks = new short[10];
        private int dirtyBlockCount = 0;
        private int minX;
        private int minY;
        private int minZ;
        private int maxX;
        private int maxY;
        private int maxZ;

        public TrackedChunk(int chunkX, int chunkY) {
            this.chunkX = chunkX;
            this.chunkZ = chunkY;
            this.chunkPos = new ChunkPos(chunkX, chunkY);
            ChunkMap.this.getWorld().chunkCache.loadChunk(chunkX, chunkY);
        }

        public void addPlayer(ServerPlayerEntity player) {
            if (this.players.contains(player)) {
                throw new IllegalStateException("Failed to add player. " + player + " already is in chunk " + this.chunkX + ", " + this.chunkZ);
            }
            player.activeChunks.add(this.chunkPos);
            player.networkHandler.sendPacket(new ChunkStatusUpdateS2CPacket(this.chunkPos.x, this.chunkPos.z, true));
            this.players.add(player);
            player.pendingChunkUpdates.add(this.chunkPos);
        }

        public void removePlayer(ServerPlayerEntity player) {
            if (!this.players.contains(player)) {
                return;
            }
            this.players.remove(player);
            if (this.players.size() == 0) {
                long l = (long)this.chunkX + Integer.MAX_VALUE | (long)this.chunkZ + Integer.MAX_VALUE << 32;
                ChunkMap.this.chunkMapping.remove(l);
                if (this.dirtyBlockCount > 0) {
                    ChunkMap.this.chunksToUpdate.remove(this);
                }
                ChunkMap.this.getWorld().chunkCache.isLoaded(this.chunkX, this.chunkZ);
            }
            player.pendingChunkUpdates.remove(this.chunkPos);
            if (player.activeChunks.contains(this.chunkPos)) {
                player.networkHandler.sendPacket(new ChunkStatusUpdateS2CPacket(this.chunkX, this.chunkZ, false));
            }
        }

        public void updatePlayerChunks(int x, int y, int z) {
            if (this.dirtyBlockCount == 0) {
                ChunkMap.this.chunksToUpdate.add(this);
                this.minX = this.minY = x;
                this.minZ = this.maxX = y;
                this.maxY = this.maxZ = z;
            }
            if (this.minX > x) {
                this.minX = x;
            }
            if (this.minY < x) {
                this.minY = x;
            }
            if (this.minZ > y) {
                this.minZ = y;
            }
            if (this.maxX < y) {
                this.maxX = y;
            }
            if (this.maxY > z) {
                this.maxY = z;
            }
            if (this.maxZ < z) {
                this.maxZ = z;
            }
            if (this.dirtyBlockCount < 10) {
                short s = (short)(x << 12 | z << 8 | y);
                for (int i = 0; i < this.dirtyBlockCount; ++i) {
                    if (this.dirtyBlocks[i] != s) continue;
                    return;
                }
                this.dirtyBlocks[this.dirtyBlockCount++] = s;
            }
        }

        public void sendPacketToPlayers(Packet packet) {
            for (int i = 0; i < this.players.size(); ++i) {
                ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
                if (!serverPlayerEntity.activeChunks.contains(this.chunkPos)) continue;
                serverPlayerEntity.networkHandler.sendPacket(packet);
            }
        }

        public void updateChunk() {
            ServerWorld serverWorld = ChunkMap.this.getWorld();
            if (this.dirtyBlockCount == 0) {
                return;
            }
            if (this.dirtyBlockCount == 1) {
                int n = this.chunkX * 16 + this.minX;
                int n2 = this.minZ;
                int n3 = this.chunkZ * 16 + this.maxY;
                this.sendPacketToPlayers(new BlockUpdateS2CPacket(n, n2, n3, serverWorld));
                if (Block.BLOCKS_WITH_ENTITY[serverWorld.getBlockId(n, n2, n3)]) {
                    this.sendBlockEntityUpdate(serverWorld.getBlockEntity(n, n2, n3));
                }
            } else if (this.dirtyBlockCount == 10) {
                this.minZ = this.minZ / 2 * 2;
                this.maxX = (this.maxX / 2 + 1) * 2;
                int n = this.minX + this.chunkX * 16;
                int n4 = this.minZ;
                int n5 = this.maxY + this.chunkZ * 16;
                int n6 = this.minY - this.minX + 1;
                int n7 = this.maxX - this.minZ + 2;
                int n8 = this.maxZ - this.maxY + 1;
                this.sendPacketToPlayers(new ChunkDataS2CPacket(n, n4, n5, n6, n7, n8, serverWorld));
                List list = serverWorld.getBlockEntities(n, n4, n5, n + n6, n4 + n7, n5 + n8);
                for (int i = 0; i < list.size(); ++i) {
                    this.sendBlockEntityUpdate((BlockEntity)list.get(i));
                }
            } else {
                this.sendPacketToPlayers(new ChunkDeltaUpdateS2CPacket(this.chunkX, this.chunkZ, this.dirtyBlocks, this.dirtyBlockCount, serverWorld));
                for (int i = 0; i < this.dirtyBlockCount; ++i) {
                    int n = this.chunkX * 16 + (this.dirtyBlockCount >> 12 & 0xF);
                    int n9 = this.dirtyBlockCount & 0xFF;
                    int n10 = this.chunkZ * 16 + (this.dirtyBlockCount >> 8 & 0xF);
                    if (!Block.BLOCKS_WITH_ENTITY[serverWorld.getBlockId(n, n9, n10)]) continue;
                    System.out.println("Sending!");
                    this.sendBlockEntityUpdate(serverWorld.getBlockEntity(n, n9, n10));
                }
            }
            this.dirtyBlockCount = 0;
        }

        private void sendBlockEntityUpdate(BlockEntity blockentity) {
            Packet packet;
            if (blockentity != null && (packet = blockentity.createUpdatePacket()) != null) {
                this.sendPacketToPlayers(packet);
            }
        }
    }
}

