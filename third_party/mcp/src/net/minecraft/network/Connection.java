/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.network;

import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;

public class Connection {
    public static final Object LOCK = new Object();
    public static int READ_THREAD_COUNTER;
    public static int WRITE_THREAD_COUNTER;
    private Object lock = new Object();
    private Socket socket;
    private final SocketAddress address;
    private DataInputStream inputStream;
    private DataOutputStream outputStream;
    private boolean open = true;
    private List readQueue = Collections.synchronizedList(new ArrayList());
    private List sendQueue = Collections.synchronizedList(new ArrayList());
    private List delayedSendQueue = Collections.synchronizedList(new ArrayList());
    private NetworkHandler networkHandler;
    private boolean closed = false;
    private Thread writer;
    private Thread reader;
    private boolean disconnected = false;
    private String disconnectReason = "";
    private Object[] disconnectReasonArgs;
    private int timeout = 0;
    private int sendQueueSize = 0;
    public static int[] TOTAL_READ_SIZE;
    public static int[] TOTAL_SEND_SIZE;
    public int lag = 0;
    private int delay = 50;

    public Connection(Socket socket, String name, NetworkHandler networkHandler) {
        this.socket = socket;
        this.address = socket.getRemoteSocketAddress();
        this.networkHandler = networkHandler;
        try {
            socket.setSoTimeout(30000);
            socket.setTrafficClass(24);
        }
        catch (SocketException socketException) {
            System.err.println(socketException.getMessage());
        }
        this.inputStream = new DataInputStream(socket.getInputStream());
        this.outputStream = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream(), 5120));
        this.reader = new Thread(name + " read thread"){

            /*
             * WARNING - Removed try catching itself - possible behaviour change.
             */
            public void run() {
                Object object = LOCK;
                synchronized (object) {
                    ++READ_THREAD_COUNTER;
                }
                try {
                    while (Connection.this.open && !Connection.this.closed) {
                        while (Connection.this.read()) {
                        }
                        try {
                            4.sleep(100L);
                        }
                        catch (InterruptedException interruptedException) {}
                    }
                }
                finally {
                    object = LOCK;
                    synchronized (object) {
                        --READ_THREAD_COUNTER;
                    }
                }
            }
        };
        this.writer = new Thread(name + " write thread"){

            /*
             * WARNING - Removed try catching itself - possible behaviour change.
             * Enabled aggressive block sorting
             * Enabled unnecessary exception pruning
             * Enabled aggressive exception aggregation
             */
            public void run() {
                Object object = LOCK;
                synchronized (object) {
                    ++WRITE_THREAD_COUNTER;
                }
                try {
                    while (Connection.this.open) {
                        while (Connection.this.write()) {
                        }
                        try {
                            1.sleep(100L);
                        }
                        catch (InterruptedException interruptedException) {
                            // empty catch block
                        }
                        try {
                            if (Connection.this.outputStream == null) continue;
                            Connection.this.outputStream.flush();
                        }
                        catch (IOException iOException) {
                            if (!Connection.this.disconnected) {
                                Connection.this.disconnect(iOException);
                            }
                            iOException.printStackTrace();
                        }
                    }
                    return;
                }
                finally {
                    object = LOCK;
                    synchronized (object) {
                        --WRITE_THREAD_COUNTER;
                    }
                }
            }
        };
        this.reader.start();
        this.writer.start();
    }

    @Environment(value=EnvType.SERVER)
    public void setNetworkHandler(NetworkHandler networkHandler) {
        this.networkHandler = networkHandler;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void sendPacket(Packet packet) {
        if (this.closed) {
            return;
        }
        Object object = this.lock;
        synchronized (object) {
            this.sendQueueSize += packet.size() + 1;
            if (packet.worldPacket) {
                this.delayedSendQueue.add(packet);
            } else {
                this.sendQueue.add(packet);
            }
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private boolean write() {
        boolean bl;
        block10: {
            bl = false;
            try {
                Packet packet;
                Object object;
                if (!(this.sendQueue.isEmpty() || this.lag != 0 && System.currentTimeMillis() - ((Packet)this.sendQueue.get((int)0)).creationTime < (long)this.lag)) {
                    object = this.lock;
                    synchronized (object) {
                        packet = (Packet)this.sendQueue.remove(0);
                        this.sendQueueSize -= packet.size() + 1;
                    }
                    Packet.write(packet, this.outputStream);
                    int n = packet.getRawId();
                    TOTAL_SEND_SIZE[n] = TOTAL_SEND_SIZE[n] + (packet.size() + 1);
                    bl = true;
                }
                if (this.delay-- > 0 || this.delayedSendQueue.isEmpty() || this.lag != 0 && System.currentTimeMillis() - ((Packet)this.delayedSendQueue.get((int)0)).creationTime < (long)this.lag) break block10;
                object = this.lock;
                synchronized (object) {
                    packet = (Packet)this.delayedSendQueue.remove(0);
                    this.sendQueueSize -= packet.size() + 1;
                }
                Packet.write(packet, this.outputStream);
                int n = packet.getRawId();
                TOTAL_SEND_SIZE[n] = TOTAL_SEND_SIZE[n] + (packet.size() + 1);
                this.delay = 0;
                bl = true;
            }
            catch (Exception exception) {
                if (!this.disconnected) {
                    this.disconnect(exception);
                }
                return false;
            }
        }
        return bl;
    }

    public void interrupt() {
        this.reader.interrupt();
        this.writer.interrupt();
    }

    private boolean read() {
        boolean bl = false;
        try {
            Packet packet = Packet.read(this.inputStream, this.networkHandler.isServerSide());
            if (packet != null) {
                int n = packet.getRawId();
                TOTAL_READ_SIZE[n] = TOTAL_READ_SIZE[n] + (packet.size() + 1);
                this.readQueue.add(packet);
                bl = true;
            } else {
                this.disconnect("disconnect.endOfStream", new Object[0]);
            }
        }
        catch (Exception exception) {
            if (!this.disconnected) {
                this.disconnect(exception);
            }
            return false;
        }
        return bl;
    }

    private void disconnect(Exception exception) {
        exception.printStackTrace();
        this.disconnect("disconnect.genericReason", "Internal exception: " + exception.toString());
    }

    public void disconnect(String reason, Object ... args) {
        if (!this.open) {
            return;
        }
        this.disconnected = true;
        this.disconnectReason = reason;
        this.disconnectReasonArgs = args;
        new Thread(this){
            final /* synthetic */ Connection outerConnection;
            {
                this.outerConnection = arg;
            }

            public void run() {
                try {
                    Thread.sleep(5000L);
                    if (Connection.getReaderThread(this.outerConnection).isAlive()) {
                        try {
                            Connection.getReaderThread(this.outerConnection).stop();
                        }
                        catch (Throwable throwable) {
                            // empty catch block
                        }
                    }
                    if (Connection.getWriterThread(this.outerConnection).isAlive()) {
                        try {
                            Connection.getWriterThread(this.outerConnection).stop();
                        }
                        catch (Throwable throwable) {}
                    }
                }
                catch (InterruptedException interruptedException) {
                    interruptedException.printStackTrace();
                }
            }
        }.start();
        this.open = false;
        try {
            this.inputStream.close();
            this.inputStream = null;
        }
        catch (Throwable throwable) {
            // empty catch block
        }
        try {
            this.outputStream.close();
            this.outputStream = null;
        }
        catch (Throwable throwable) {
            // empty catch block
        }
        try {
            this.socket.close();
            this.socket = null;
        }
        catch (Throwable throwable) {
            // empty catch block
        }
    }

    public void tick() {
        if (this.sendQueueSize > 0x100000) {
            this.disconnect("disconnect.overflow", new Object[0]);
        }
        if (this.readQueue.isEmpty()) {
            if (this.timeout++ == 1200) {
                this.disconnect("disconnect.timeout", new Object[0]);
            }
        } else {
            this.timeout = 0;
        }
        int n = 100;
        while (!this.readQueue.isEmpty() && n-- >= 0) {
            Packet packet = (Packet)this.readQueue.remove(0);
            packet.apply(this.networkHandler);
        }
        this.interrupt();
        if (this.disconnected && this.readQueue.isEmpty()) {
            this.networkHandler.onDisconnected(this.disconnectReason, this.disconnectReasonArgs);
        }
    }

    @Environment(value=EnvType.SERVER)
    public SocketAddress getAddress() {
        return this.address;
    }

    public void disconnect() {
        this.interrupt();
        this.closed = true;
        this.reader.interrupt();
        new Thread(this){
            final /* synthetic */ Connection outerConnection;
            {
                this.outerConnection = arg;
            }

            public void run() {
                try {
                    Thread.sleep(2000L);
                    if (Connection.isReaderAlive(this.outerConnection)) {
                        Connection.getWriterThread(this.outerConnection).interrupt();
                        this.outerConnection.disconnect("disconnect.closed", new Object[0]);
                    }
                }
                catch (Exception exception) {
                    exception.printStackTrace();
                }
            }
        }.start();
    }

    @Environment(value=EnvType.SERVER)
    public int getDelayedSendQueueSize() {
        return this.delayedSendQueue.size();
    }

    static /* synthetic */ Thread getReaderThread(Connection arg) {
        return arg.reader;
    }

    static /* synthetic */ Thread getWriterThread(Connection arg) {
        return arg.writer;
    }

    static {
        TOTAL_READ_SIZE = new int[256];
        TOTAL_SEND_SIZE = new int[256];
    }
}

