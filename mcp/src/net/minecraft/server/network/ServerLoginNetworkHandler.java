/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.network;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.Socket;
import java.net.URL;
import java.net.URLEncoder;
import java.util.Random;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.network.Connection;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.handshake.HandshakePacket;
import net.minecraft.network.packet.login.LoginHelloPacket;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.DisconnectPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerPlayNetworkHandler;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;

@Environment(value=EnvType.SERVER)
public class ServerLoginNetworkHandler
extends NetworkHandler {
    public static Logger LOGGER = Logger.getLogger("Minecraft");
    private static Random random = new Random();
    public Connection connection;
    public boolean closed = false;
    private MinecraftServer server;
    private int loginTicks = 0;
    private String username = null;
    private LoginHelloPacket loginPacket = null;
    private String serverId = "";

    public ServerLoginNetworkHandler(MinecraftServer server, Socket socket, String name) {
        this.server = server;
        this.connection = new Connection(socket, name, this);
        this.connection.lag = 0;
    }

    public void tick() {
        if (this.loginPacket != null) {
            this.accept(this.loginPacket);
            this.loginPacket = null;
        }
        if (this.loginTicks++ == 600) {
            this.disconnect("Took too long to log in");
        } else {
            this.connection.tick();
        }
    }

    public void disconnect(String reason) {
        try {
            LOGGER.info("Disconnecting " + this.getConnectionInfo() + ": " + reason);
            this.connection.sendPacket(new DisconnectPacket(reason));
            this.connection.disconnect();
            this.closed = true;
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    public void onHandshake(HandshakePacket packet) {
        if (this.server.onlineMode) {
            this.serverId = Long.toHexString(random.nextLong());
            this.connection.sendPacket(new HandshakePacket(this.serverId));
        } else {
            this.connection.sendPacket(new HandshakePacket("-"));
        }
    }

    public void onHello(final LoginHelloPacket packet) {
        this.username = packet.username;
        if (packet.protocolVersion != 14) {
            if (packet.protocolVersion > 14) {
                this.disconnect("Outdated server!");
            } else {
                this.disconnect("Outdated client!");
            }
            return;
        }
        if (!this.server.onlineMode) {
            this.accept(packet);
        } else {
            new Thread(){

                public void run() {
                    try {
                        String string = ServerLoginNetworkHandler.this.serverId;
                        URL uRL = new URL("http://www.minecraft.net/game/checkserver.jsp?user=" + URLEncoder.encode(packet.username, "UTF-8") + "&serverId=" + URLEncoder.encode(string, "UTF-8"));
                        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(uRL.openStream()));
                        String string2 = bufferedReader.readLine();
                        bufferedReader.close();
                        if (string2.equals("YES")) {
                            ServerLoginNetworkHandler.this.loginPacket = packet;
                        } else {
                            ServerLoginNetworkHandler.this.disconnect("Failed to verify username!");
                        }
                    }
                    catch (Exception exception) {
                        ServerLoginNetworkHandler.this.disconnect("Failed to verify username! [internal error " + exception + "]");
                        exception.printStackTrace();
                    }
                }
            }.start();
        }
    }

    public void accept(LoginHelloPacket packet) {
        ServerPlayerEntity serverPlayerEntity = this.server.playerManager.connectPlayer(this, packet.username);
        if (serverPlayerEntity != null) {
            this.server.playerManager.loadPlayerData(serverPlayerEntity);
            serverPlayerEntity.setWorld(this.server.getWorld(serverPlayerEntity.dimensionId));
            LOGGER.info(this.getConnectionInfo() + " logged in with entity id " + serverPlayerEntity.id + " at (" + serverPlayerEntity.x + ", " + serverPlayerEntity.y + ", " + serverPlayerEntity.z + ")");
            ServerWorld serverWorld = this.server.getWorld(serverPlayerEntity.dimensionId);
            Vec3i vec3i = serverWorld.getSpawnPos();
            ServerPlayNetworkHandler serverPlayNetworkHandler = new ServerPlayNetworkHandler(this.server, this.connection, serverPlayerEntity);
            serverPlayNetworkHandler.sendPacket(new LoginHelloPacket("", serverPlayerEntity.id, serverWorld.getSeed(), (byte)serverWorld.dimension.id));
            serverPlayNetworkHandler.sendPacket(new PlayerSpawnPositionS2CPacket(vec3i.x, vec3i.y, vec3i.z));
            this.server.playerManager.sendWorldInfo(serverPlayerEntity, serverWorld);
            this.server.playerManager.sendToAll(new ChatMessagePacket("\u00a7e" + serverPlayerEntity.name + " joined the game."));
            this.server.playerManager.addPlayer(serverPlayerEntity);
            serverPlayNetworkHandler.teleport(serverPlayerEntity.x, serverPlayerEntity.y, serverPlayerEntity.z, serverPlayerEntity.yaw, serverPlayerEntity.pitch);
            this.server.connections.addConnection(serverPlayNetworkHandler);
            serverPlayNetworkHandler.sendPacket(new WorldTimeUpdateS2CPacket(serverWorld.getTime()));
            serverPlayerEntity.initScreenHandler();
        }
        this.closed = true;
    }

    public void onDisconnected(String reason, Object[] objects) {
        LOGGER.info(this.getConnectionInfo() + " lost connection");
        this.closed = true;
    }

    public void handle(Packet packet) {
        this.disconnect("Protocol error");
    }

    public String getConnectionInfo() {
        if (this.username != null) {
            return this.username + " [" + this.connection.getAddress().toString() + "]";
        }
        return this.connection.getAddress().toString();
    }

    public boolean isServerSide() {
        return true;
    }
}

