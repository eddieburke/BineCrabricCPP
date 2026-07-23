package net.minecraft.test;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class TestClientSocket implements AutoCloseable {
    private final Socket socket;
    private final DataInputStream in;
    private final DataOutputStream out;
    private final List<Integer> receivedPacketIds = new ArrayList<>();
    private int playerEntityId = -1;
    private double lastReceivedX;
    private double lastReceivedFeetY;
    private double lastReceivedStance;
    private double lastReceivedZ;

    public TestClientSocket(String host, int port) throws IOException {
        this.socket = new Socket(host, port);
        this.socket.setSoTimeout(30000);
        this.in = new DataInputStream(socket.getInputStream());
        this.out = new DataOutputStream(socket.getOutputStream());
    }

    public void sendLogin(String username) throws IOException {
        out.writeByte(2);
        writeString(username);
        out.flush();

        int handshakePacketId = in.readUnsignedByte();
        if (handshakePacketId == 2) {
            readString();
        }

        out.writeByte(1);
        out.writeInt(14);
        writeString(username);
        out.writeLong(0L);
        out.writeByte(0);
        out.flush();
    }

    public void sendPlayerMove(double x, double feetY, double stance, double z, float yaw, float pitch, boolean onGround) throws IOException {
        out.writeByte(13);
        out.writeDouble(x);
        out.writeDouble(feetY);
        out.writeDouble(stance);
        out.writeDouble(z);
        out.writeFloat(yaw);
        out.writeFloat(pitch);
        out.writeBoolean(onGround);
        out.flush();
    }

    public void sendPlayerAction(int action, int x, int y, int z, int direction) throws IOException {
        out.writeByte(14);
        out.writeByte(action);
        out.writeInt(x);
        out.writeByte(y);
        out.writeInt(z);
        out.writeByte(direction);
        out.flush();
    }

    public int readPacket() throws IOException {
        int packetId = in.readUnsignedByte();
        receivedPacketIds.add(packetId);
        switch (packetId) {
            case 0:
                in.readInt();
                break;
            case 1:
                this.playerEntityId = in.readInt();
                readString();
                in.readLong();
                in.readByte();
                break;
            case 2:
                readString();
                break;
            case 3:
                readString();
                break;
            case 4:
                in.readLong();
                break;
            case 5:
                in.readInt();
                in.readShort();
                in.readShort();
                in.readShort();
                break;
            case 6:
                in.readInt();
                in.readInt();
                in.readInt();
                break;
            case 8:
                in.readShort();
                break;
            case 13:
                lastReceivedX = in.readDouble();
                lastReceivedFeetY = in.readDouble();
                lastReceivedStance = in.readDouble();
                lastReceivedZ = in.readDouble();
                in.readFloat();
                in.readFloat();
                in.readBoolean();
                break;
            case 20:
                in.readInt();
                readString();
                in.readInt();
                in.readInt();
                in.readInt();
                in.readByte();
                in.readByte();
                in.readShort();
                break;
            case 28:
                in.readInt();
                in.readShort();
                in.readShort();
                in.readShort();
                break;
            case 29:
                in.readInt();
                break;
            case 50:
                in.readInt();
                in.readInt();
                in.readBoolean();
                break;
            case 51:
                in.readInt();
                in.readShort();
                in.readInt();
                in.readByte();
                in.readByte();
                in.readByte();
                int compressedSize = in.readInt();
                byte[] data = new byte[compressedSize];
                in.readFully(data);
                break;
            case 52:
                in.readInt();
                in.readInt();
                int entryCount = in.readUnsignedShort();
                byte[] deltaData = new byte[entryCount * 4];
                in.readFully(deltaData);
                break;
            case 53:
                in.readInt();
                in.readByte();
                in.readInt();
                in.readByte();
                in.readByte();
                break;
            case 70:
                in.readByte();
                in.readByte();
                break;
            case 255:
                readString();
                break;
            default:
                throw new IOException("Unhandled S2C packet ID in TestClientSocket: 0x" + Integer.toHexString(packetId));
        }
        return packetId;
    }

    public List<Integer> getReceivedPacketIds() {
        return receivedPacketIds;
    }

    public int getPlayerEntityId() {
        return playerEntityId;
    }

    public double getLastReceivedX() {
        return lastReceivedX;
    }

    public double getLastReceivedFeetY() {
        return lastReceivedFeetY;
    }

    public double getLastReceivedStance() {
        return lastReceivedStance;
    }

    public double getLastReceivedZ() {
        return lastReceivedZ;
    }

    private void writeString(String str) throws IOException {
        out.writeShort(str.length());
        out.writeChars(str);
    }

    private String readString() throws IOException {
        short len = in.readShort();
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            sb.append(in.readChar());
        }
        return sb.toString();
    }

    @Override
    public void close() throws IOException {
        socket.close();
    }
}
