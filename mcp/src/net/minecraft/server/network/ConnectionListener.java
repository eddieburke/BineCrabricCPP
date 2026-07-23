/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.network;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerLoginNetworkHandler;
import net.minecraft.server.network.ServerPlayNetworkHandler;

@Environment(value=EnvType.SERVER)
public class ConnectionListener {
    public static Logger LOGGER = Logger.getLogger("Minecraft");
    private ServerSocket socket;
    private Thread thread;
    public volatile boolean open = false;
    private int connectionCounter = 0;
    private ArrayList pendingConnections = new ArrayList();
    private ArrayList connections = new ArrayList();
    public MinecraftServer server;

    public ConnectionListener(final MinecraftServer server, InetAddress address, int port) {
        this.server = server;
        this.socket = new ServerSocket(port, 0, address);
        this.socket.setPerformancePreferences(0, 2, 1);
        this.open = true;
        this.thread = new Thread("Listen thread"){

            public void run() {
                HashMap<InetAddress, Long> hashMap = new HashMap<InetAddress, Long>();
                while (ConnectionListener.this.open) {
                    try {
                        Socket socket = ConnectionListener.this.socket.accept();
                        if (socket == null) continue;
                        InetAddress inetAddress = socket.getInetAddress();
                        if (hashMap.containsKey(inetAddress) && !"127.0.0.1".equals(inetAddress.getHostAddress()) && System.currentTimeMillis() - (Long)hashMap.get(inetAddress) < 5000L) {
                            hashMap.put(inetAddress, System.currentTimeMillis());
                            socket.close();
                            continue;
                        }
                        hashMap.put(inetAddress, System.currentTimeMillis());
                        ServerLoginNetworkHandler serverLoginNetworkHandler = new ServerLoginNetworkHandler(server, socket, "Connection #" + ConnectionListener.this.connectionCounter++);
                        ConnectionListener.this.addPendingConnection(serverLoginNetworkHandler);
                    }
                    catch (IOException iOException) {
                        iOException.printStackTrace();
                    }
                }
            }
        };
        this.thread.start();
    }

    public void addConnection(ServerPlayNetworkHandler connection) {
        this.connections.add(connection);
    }

    private void addPendingConnection(ServerLoginNetworkHandler connection) {
        if (connection == null) {
            throw new IllegalArgumentException("Got null pendingconnection!");
        }
        this.pendingConnections.add(connection);
    }

    public void tick() {
        NetworkHandler networkHandler;
        int n;
        for (n = 0; n < this.pendingConnections.size(); ++n) {
            networkHandler = (ServerLoginNetworkHandler)this.pendingConnections.get(n);
            try {
                ((ServerLoginNetworkHandler)networkHandler).tick();
            }
            catch (Exception exception) {
                ((ServerLoginNetworkHandler)networkHandler).disconnect("Internal server error");
                LOGGER.log(Level.WARNING, "Failed to handle packet: " + exception, exception);
            }
            if (((ServerLoginNetworkHandler)networkHandler).closed) {
                this.pendingConnections.remove(n--);
            }
            ((ServerLoginNetworkHandler)networkHandler).connection.interrupt();
        }
        for (n = 0; n < this.connections.size(); ++n) {
            networkHandler = (ServerPlayNetworkHandler)this.connections.get(n);
            try {
                ((ServerPlayNetworkHandler)networkHandler).tick();
            }
            catch (Exception exception) {
                LOGGER.log(Level.WARNING, "Failed to handle packet: " + exception, exception);
                ((ServerPlayNetworkHandler)networkHandler).disconnect("Internal server error");
            }
            if (((ServerPlayNetworkHandler)networkHandler).disconnected) {
                this.connections.remove(n--);
            }
            ((ServerPlayNetworkHandler)networkHandler).connection.interrupt();
        }
    }
}

