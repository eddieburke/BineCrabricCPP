/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.PlayerRespawnPacket;
import net.minecraft.network.packet.s2c.play.GameStateChangeS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;
import net.minecraft.server.ChunkMap;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerLoginNetworkHandler;
import net.minecraft.server.network.ServerPlayerInteractionManager;
import net.minecraft.server.world.PlayerSaveHandler;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.dimension.PortalForcer;

@Environment(value=EnvType.SERVER)
public class PlayerManager {
    public static Logger LOGGER = Logger.getLogger("Minecraft");
    public List players = new ArrayList();
    private MinecraftServer server;
    private ChunkMap[] chunkMaps;
    private int maxPlayerCount;
    private Set bannedPlayers = new HashSet();
    private Set bannedIps = new HashSet();
    private Set ops = new HashSet();
    private Set whitelist = new HashSet();
    private File BANNED_PLAYERS_FILE;
    private File BANNED_IPS_FILE;
    private File OPERATORS_FILE;
    private File WHITELIST_FILE;
    private PlayerSaveHandler saveHandler;
    private boolean whitelistEnabled;

    public PlayerManager(MinecraftServer server) {
        this.chunkMaps = new ChunkMap[2];
        this.server = server;
        this.BANNED_PLAYERS_FILE = server.getFile("banned-players.txt");
        this.BANNED_IPS_FILE = server.getFile("banned-ips.txt");
        this.OPERATORS_FILE = server.getFile("ops.txt");
        this.WHITELIST_FILE = server.getFile("white-list.txt");
        int n = server.properties.getProperty("view-distance", 10);
        this.chunkMaps[0] = new ChunkMap(server, 0, n);
        this.chunkMaps[1] = new ChunkMap(server, -1, n);
        this.maxPlayerCount = server.properties.getProperty("max-players", 20);
        this.whitelistEnabled = server.properties.getProperty("white-list", false);
        this.loadBannedPlayers();
        this.loadBannedIps();
        this.loadOperators();
        this.loadWhitelist();
        this.saveBannedPlayers();
        this.saveBannedIps();
        this.saveOperators();
        this.saveWhitelist();
    }

    public void saveAllPlayers(ServerWorld[] world) {
        this.saveHandler = world[0].getDimensionData().getPlayerSaveHandler();
    }

    public void updatePlayerAfterDimensionChange(ServerPlayerEntity player) {
        this.chunkMaps[0].removePlayer(player);
        this.chunkMaps[1].removePlayer(player);
        this.getChunkMap(player.dimensionId).addPlayer(player);
        ServerWorld serverWorld = this.server.getWorld(player.dimensionId);
        serverWorld.chunkCache.loadChunk((int)player.x >> 4, (int)player.z >> 4);
    }

    public int getBlockViewDistance() {
        return this.chunkMaps[0].getBlockViewDistance();
    }

    private ChunkMap getChunkMap(int dimensionId) {
        return dimensionId == -1 ? this.chunkMaps[1] : this.chunkMaps[0];
    }

    public void loadPlayerData(ServerPlayerEntity player) {
        this.saveHandler.loadPlayerData(player);
    }

    public void addPlayer(ServerPlayerEntity player) {
        this.players.add(player);
        ServerWorld serverWorld = this.server.getWorld(player.dimensionId);
        serverWorld.chunkCache.loadChunk((int)player.x >> 4, (int)player.z >> 4);
        while (serverWorld.getEntityCollisions(player, player.boundingBox).size() != 0) {
            player.setPosition(player.x, player.y + 1.0, player.z);
        }
        serverWorld.spawnEntity(player);
        this.getChunkMap(player.dimensionId).addPlayer(player);
    }

    public void updatePlayerChunks(ServerPlayerEntity player) {
        this.getChunkMap(player.dimensionId).updatePlayerChunks(player);
    }

    public void disconnect(ServerPlayerEntity player) {
        this.saveHandler.savePlayerData(player);
        this.server.getWorld(player.dimensionId).remove(player);
        this.players.remove(player);
        this.getChunkMap(player.dimensionId).removePlayer(player);
    }

    public ServerPlayerEntity connectPlayer(ServerLoginNetworkHandler loginNetworkHandler, String name) {
        if (this.bannedPlayers.contains(name.trim().toLowerCase())) {
            loginNetworkHandler.disconnect("You are banned from this server!");
            return null;
        }
        if (!this.isWhitelisted(name)) {
            loginNetworkHandler.disconnect("You are not white-listed on this server!");
            return null;
        }
        String string = loginNetworkHandler.connection.getAddress().toString();
        string = string.substring(string.indexOf("/") + 1);
        if (this.bannedIps.contains(string = string.substring(0, string.indexOf(":")))) {
            loginNetworkHandler.disconnect("Your IP address is banned from this server!");
            return null;
        }
        if (this.players.size() >= this.maxPlayerCount) {
            loginNetworkHandler.disconnect("The server is full!");
            return null;
        }
        for (int i = 0; i < this.players.size(); ++i) {
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            if (!serverPlayerEntity.name.equalsIgnoreCase(name)) continue;
            serverPlayerEntity.networkHandler.disconnect("You logged in from another location");
        }
        return new ServerPlayerEntity(this.server, this.server.getWorld(0), name, new ServerPlayerInteractionManager(this.server.getWorld(0)));
    }

    public ServerPlayerEntity respawnPlayer(ServerPlayerEntity player, int dimensionId) {
        this.server.getEntityTracker(player.dimensionId).removeListener(player);
        this.server.getEntityTracker(player.dimensionId).onEntityRemoved(player);
        this.getChunkMap(player.dimensionId).removePlayer(player);
        this.players.remove(player);
        this.server.getWorld(player.dimensionId).serverRemove(player);
        Vec3i vec3i = player.getSpawnPos();
        player.dimensionId = dimensionId;
        ServerPlayerEntity serverPlayerEntity = new ServerPlayerEntity(this.server, this.server.getWorld(player.dimensionId), player.name, new ServerPlayerInteractionManager(this.server.getWorld(player.dimensionId)));
        serverPlayerEntity.id = player.id;
        serverPlayerEntity.networkHandler = player.networkHandler;
        ServerWorld serverWorld = this.server.getWorld(player.dimensionId);
        if (vec3i != null) {
            Vec3i vec3i2 = PlayerEntity.findRespawnPosition(this.server.getWorld(player.dimensionId), vec3i);
            if (vec3i2 != null) {
                serverPlayerEntity.setPositionAndAnglesKeepPrevAngles((float)vec3i2.x + 0.5f, (float)vec3i2.y + 0.1f, (float)vec3i2.z + 0.5f, 0.0f, 0.0f);
                serverPlayerEntity.setSpawnPos(vec3i);
            } else {
                serverPlayerEntity.networkHandler.sendPacket(new GameStateChangeS2CPacket(0));
            }
        }
        serverWorld.chunkCache.loadChunk((int)serverPlayerEntity.x >> 4, (int)serverPlayerEntity.z >> 4);
        while (serverWorld.getEntityCollisions(serverPlayerEntity, serverPlayerEntity.boundingBox).size() != 0) {
            serverPlayerEntity.setPosition(serverPlayerEntity.x, serverPlayerEntity.y + 1.0, serverPlayerEntity.z);
        }
        serverPlayerEntity.networkHandler.sendPacket(new PlayerRespawnPacket((byte)serverPlayerEntity.dimensionId));
        serverPlayerEntity.networkHandler.teleport(serverPlayerEntity.x, serverPlayerEntity.y, serverPlayerEntity.z, serverPlayerEntity.yaw, serverPlayerEntity.pitch);
        this.sendWorldInfo(serverPlayerEntity, serverWorld);
        this.getChunkMap(serverPlayerEntity.dimensionId).addPlayer(serverPlayerEntity);
        serverWorld.spawnEntity(serverPlayerEntity);
        this.players.add(serverPlayerEntity);
        serverPlayerEntity.initScreenHandler();
        serverPlayerEntity.clearWakePosition();
        return serverPlayerEntity;
    }

    public void changePlayerDimension(ServerPlayerEntity player) {
        ServerWorld serverWorld = this.server.getWorld(player.dimensionId);
        int n = 0;
        n = player.dimensionId == -1 ? 0 : -1;
        player.dimensionId = n;
        ServerWorld serverWorld2 = this.server.getWorld(player.dimensionId);
        player.networkHandler.sendPacket(new PlayerRespawnPacket((byte)player.dimensionId));
        serverWorld.serverRemove(player);
        player.dead = false;
        double d = player.x;
        double d2 = player.z;
        double d3 = 8.0;
        if (player.dimensionId == -1) {
            player.setPositionAndAnglesKeepPrevAngles(d /= d3, player.y, d2 /= d3, player.yaw, player.pitch);
            if (player.isAlive()) {
                serverWorld.updateEntity(player, false);
            }
        } else {
            player.setPositionAndAnglesKeepPrevAngles(d *= d3, player.y, d2 *= d3, player.yaw, player.pitch);
            if (player.isAlive()) {
                serverWorld.updateEntity(player, false);
            }
        }
        if (player.isAlive()) {
            serverWorld2.spawnEntity(player);
            player.setPositionAndAnglesKeepPrevAngles(d, player.y, d2, player.yaw, player.pitch);
            serverWorld2.updateEntity(player, false);
            serverWorld2.chunkCache.forceLoad = true;
            new PortalForcer().moveToPortal(serverWorld2, player);
            serverWorld2.chunkCache.forceLoad = false;
        }
        this.updatePlayerAfterDimensionChange(player);
        player.networkHandler.teleport(player.x, player.y, player.z, player.yaw, player.pitch);
        player.setWorld(serverWorld2);
        this.sendWorldInfo(player, serverWorld2);
        this.sendPlayerStatus(player);
    }

    public void updateAllChunks() {
        for (int i = 0; i < this.chunkMaps.length; ++i) {
            this.chunkMaps[i].updateChunks();
        }
    }

    public void markDirty(int x, int y, int z, int dimensionId) {
        this.getChunkMap(dimensionId).markBlockForUpdate(x, y, z);
    }

    public void sendToAll(Packet packet) {
        for (int i = 0; i < this.players.size(); ++i) {
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            serverPlayerEntity.networkHandler.sendPacket(packet);
        }
    }

    public void sendToDimension(Packet packet, int dimensionId) {
        for (int i = 0; i < this.players.size(); ++i) {
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            if (serverPlayerEntity.dimensionId != dimensionId) continue;
            serverPlayerEntity.networkHandler.sendPacket(packet);
        }
    }

    public String getPlayerList() {
        String string = "";
        for (int i = 0; i < this.players.size(); ++i) {
            if (i > 0) {
                string = string + ", ";
            }
            string = string + ((ServerPlayerEntity)this.players.get((int)i)).name;
        }
        return string;
    }

    public void banPlayer(String name) {
        this.bannedPlayers.add(name.toLowerCase());
        this.saveBannedPlayers();
    }

    public void unbanPlayer(String name) {
        this.bannedPlayers.remove(name.toLowerCase());
        this.saveBannedPlayers();
    }

    private void loadBannedPlayers() {
        try {
            this.bannedPlayers.clear();
            BufferedReader bufferedReader = new BufferedReader(new FileReader(this.BANNED_PLAYERS_FILE));
            String string = "";
            while ((string = bufferedReader.readLine()) != null) {
                this.bannedPlayers.add(string.trim().toLowerCase());
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to load ban list: " + exception);
        }
    }

    private void saveBannedPlayers() {
        try {
            PrintWriter printWriter = new PrintWriter(new FileWriter(this.BANNED_PLAYERS_FILE, false));
            for (String string : this.bannedPlayers) {
                printWriter.println(string);
            }
            printWriter.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to save ban list: " + exception);
        }
    }

    public void banIp(String ip) {
        this.bannedIps.add(ip.toLowerCase());
        this.saveBannedIps();
    }

    public void unbanIp(String ip) {
        this.bannedIps.remove(ip.toLowerCase());
        this.saveBannedIps();
    }

    private void loadBannedIps() {
        try {
            this.bannedIps.clear();
            BufferedReader bufferedReader = new BufferedReader(new FileReader(this.BANNED_IPS_FILE));
            String string = "";
            while ((string = bufferedReader.readLine()) != null) {
                this.bannedIps.add(string.trim().toLowerCase());
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to load ip ban list: " + exception);
        }
    }

    private void saveBannedIps() {
        try {
            PrintWriter printWriter = new PrintWriter(new FileWriter(this.BANNED_IPS_FILE, false));
            for (String string : this.bannedIps) {
                printWriter.println(string);
            }
            printWriter.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to save ip ban list: " + exception);
        }
    }

    public void addToOperators(String name) {
        this.ops.add(name.toLowerCase());
        this.saveOperators();
    }

    public void removeFromOperators(String name) {
        this.ops.remove(name.toLowerCase());
        this.saveOperators();
    }

    private void loadOperators() {
        try {
            this.ops.clear();
            BufferedReader bufferedReader = new BufferedReader(new FileReader(this.OPERATORS_FILE));
            String string = "";
            while ((string = bufferedReader.readLine()) != null) {
                this.ops.add(string.trim().toLowerCase());
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to load ip ban list: " + exception);
        }
    }

    private void saveOperators() {
        try {
            PrintWriter printWriter = new PrintWriter(new FileWriter(this.OPERATORS_FILE, false));
            for (String string : this.ops) {
                printWriter.println(string);
            }
            printWriter.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to save ip ban list: " + exception);
        }
    }

    private void loadWhitelist() {
        try {
            this.whitelist.clear();
            BufferedReader bufferedReader = new BufferedReader(new FileReader(this.WHITELIST_FILE));
            String string = "";
            while ((string = bufferedReader.readLine()) != null) {
                this.whitelist.add(string.trim().toLowerCase());
            }
            bufferedReader.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to load white-list: " + exception);
        }
    }

    private void saveWhitelist() {
        try {
            PrintWriter printWriter = new PrintWriter(new FileWriter(this.WHITELIST_FILE, false));
            for (String string : this.whitelist) {
                printWriter.println(string);
            }
            printWriter.close();
        }
        catch (Exception exception) {
            LOGGER.warning("Failed to save white-list: " + exception);
        }
    }

    public boolean isWhitelisted(String name) {
        name = name.trim().toLowerCase();
        return !this.whitelistEnabled || this.ops.contains(name) || this.whitelist.contains(name);
    }

    public boolean isOperator(String name) {
        return this.ops.contains(name.trim().toLowerCase());
    }

    public ServerPlayerEntity getPlayer(String name) {
        for (int i = 0; i < this.players.size(); ++i) {
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            if (!serverPlayerEntity.name.equalsIgnoreCase(name)) continue;
            return serverPlayerEntity;
        }
        return null;
    }

    public void messagePlayer(String name, String message) {
        ServerPlayerEntity serverPlayerEntity = this.getPlayer(name);
        if (serverPlayerEntity != null) {
            serverPlayerEntity.networkHandler.sendPacket(new ChatMessagePacket(message));
        }
    }

    public void sendToAround(double x, double y, double z, double range, int dimensionId, Packet packet) {
        this.sendToAround(null, x, y, z, range, dimensionId, packet);
    }

    public void sendToAround(PlayerEntity player, double x, double y, double z, double range, int dimensionId, Packet packet) {
        for (int i = 0; i < this.players.size(); ++i) {
            double d;
            double d2;
            double d3;
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            if (serverPlayerEntity == player || serverPlayerEntity.dimensionId != dimensionId || !((d3 = x - serverPlayerEntity.x) * d3 + (d2 = y - serverPlayerEntity.y) * d2 + (d = z - serverPlayerEntity.z) * d < range * range)) continue;
            serverPlayerEntity.networkHandler.sendPacket(packet);
        }
    }

    public void broadcast(String message) {
        ChatMessagePacket chatMessagePacket = new ChatMessagePacket(message);
        for (int i = 0; i < this.players.size(); ++i) {
            ServerPlayerEntity serverPlayerEntity = (ServerPlayerEntity)this.players.get(i);
            if (!this.isOperator(serverPlayerEntity.name)) continue;
            serverPlayerEntity.networkHandler.sendPacket(chatMessagePacket);
        }
    }

    public boolean sendPacket(String player, Packet packet) {
        ServerPlayerEntity serverPlayerEntity = this.getPlayer(player);
        if (serverPlayerEntity != null) {
            serverPlayerEntity.networkHandler.sendPacket(packet);
            return true;
        }
        return false;
    }

    public void savePlayers() {
        for (int i = 0; i < this.players.size(); ++i) {
            this.saveHandler.savePlayerData((PlayerEntity)this.players.get(i));
        }
    }

    public void updateBlockEntity(int x, int y, int z, BlockEntity blockentity) {
    }

    public void addToWhitelist(String name) {
        this.whitelist.add(name);
        this.saveWhitelist();
    }

    public void removeFromWhitelist(String name) {
        this.whitelist.remove(name);
        this.saveWhitelist();
    }

    public Set getWhitelist() {
        return this.whitelist;
    }

    public void reloadWhitelist() {
        this.loadWhitelist();
    }

    public void sendWorldInfo(ServerPlayerEntity player, ServerWorld world) {
        player.networkHandler.sendPacket(new WorldTimeUpdateS2CPacket(world.getTime()));
        if (world.isRaining()) {
            player.networkHandler.sendPacket(new GameStateChangeS2CPacket(1));
        }
    }

    public void sendPlayerStatus(ServerPlayerEntity player) {
        player.onContentsUpdate(player.playerScreenHandler);
        player.markHealthDirty();
    }
}

