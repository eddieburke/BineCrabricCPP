/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network.packet;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.PacketTracker;
import net.minecraft.network.packet.c2s.play.ClickSlotC2SPacket;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInputC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.network.packet.handshake.HandshakePacket;
import net.minecraft.network.packet.login.LoginHelloPacket;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.DisconnectPacket;
import net.minecraft.network.packet.play.EntityAnimationPacket;
import net.minecraft.network.packet.play.KeepAlivePacket;
import net.minecraft.network.packet.play.PlayerMoveFullPacket;
import net.minecraft.network.packet.play.PlayerMoveLookAndOnGroundPacket;
import net.minecraft.network.packet.play.PlayerMovePacket;
import net.minecraft.network.packet.play.PlayerMovePositionAndOnGroundPacket;
import net.minecraft.network.packet.play.PlayerRespawnPacket;
import net.minecraft.network.packet.play.ScreenHandlerAcknowledgementPacket;
import net.minecraft.network.packet.play.UpdateSignPacket;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDataS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkStatusUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.CloseScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityDestroyS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityEquipmentUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityMoveRelativeS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityRotateAndMoveRelativeS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityRotateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityS2CPacket;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityTrackerUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVehicleSetS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import net.minecraft.network.packet.s2c.play.GameStateChangeS2CPacket;
import net.minecraft.network.packet.s2c.play.GlobalEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.HealthUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.IncreaseStatS2CPacket;
import net.minecraft.network.packet.s2c.play.InventoryS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemPickupAnimationS2CPacket;
import net.minecraft.network.packet.s2c.play.LivingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.MapUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.OpenScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.PaintingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayNoteSoundS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSleepUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerPropertyUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerSlotUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldEventS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;

public abstract class Packet {
    private static Map ID_TO_TYPE = new HashMap();
    private static Map TYPE_TO_ID = new HashMap();
    private static Set S2C = new HashSet();
    private static Set C2S = new HashSet();
    public final long creationTime = System.currentTimeMillis();
    public boolean worldPacket = false;
    private static HashMap trackers;
    private static int incomingCount;

    static void register(int rawId, boolean clientBound, boolean serverBound, Class type) {
        if (ID_TO_TYPE.containsKey(rawId)) {
            throw new IllegalArgumentException("Duplicate packet id:" + rawId);
        }
        if (TYPE_TO_ID.containsKey(type)) {
            throw new IllegalArgumentException("Duplicate packet class:" + type);
        }
        ID_TO_TYPE.put(rawId, type);
        TYPE_TO_ID.put(type, rawId);
        if (clientBound) {
            S2C.add(rawId);
        }
        if (serverBound) {
            C2S.add(rawId);
        }
    }

    public static Packet create(int rawId) {
        try {
            Class clazz = (Class)ID_TO_TYPE.get(rawId);
            if (clazz == null) {
                return null;
            }
            return (Packet)clazz.newInstance();
        }
        catch (Exception exception) {
            exception.printStackTrace();
            System.out.println("Skipping packet with id " + rawId);
            return null;
        }
    }

    public final int getRawId() {
        return (Integer)TYPE_TO_ID.get(this.getClass());
    }

    public static Packet read(DataInputStream stream, boolean server) {
        int n = 0;
        Packet packet = null;
        try {
            n = stream.read();
            if (n == -1) {
                return null;
            }
            if (server && !C2S.contains(n) || !server && !S2C.contains(n)) {
                throw new IOException("Bad packet id " + n);
            }
            packet = Packet.create(n);
            if (packet == null) {
                throw new IOException("Bad packet id " + n);
            }
            packet.read(stream);
        }
        catch (EOFException eOFException) {
            System.out.println("Reached end of stream");
            return null;
        }
        PacketTracker packetTracker = (PacketTracker)trackers.get(n);
        if (packetTracker == null) {
            packetTracker = new PacketTracker(null);
            trackers.put(n, packetTracker);
        }
        packetTracker.update(packet.size());
        if (++incomingCount % 1000 == 0) {
            // empty if block
        }
        return packet;
    }

    public static void write(Packet packet, DataOutputStream stream) {
        stream.write(packet.getRawId());
        packet.write(stream);
    }

    public static void writeString(String string, DataOutputStream stream) {
        if (string.length() > Short.MAX_VALUE) {
            throw new IOException("String too big");
        }
        stream.writeShort(string.length());
        stream.writeChars(string);
    }

    public static String readString(DataInputStream stream, int maxLength) {
        int n = stream.readShort();
        if (n > maxLength) {
            throw new IOException("Received string length longer than maximum allowed (" + n + " > " + maxLength + ")");
        }
        if (n < 0) {
            throw new IOException("Received string length is less than zero! Weird string!");
        }
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < n; ++i) {
            stringBuilder.append(stream.readChar());
        }
        return stringBuilder.toString();
    }

    public abstract void read(DataInputStream var1);

    public abstract void write(DataOutputStream var1);

    public abstract void apply(NetworkHandler var1);

    public abstract int size();

    static {
        Packet.register(0, true, true, KeepAlivePacket.class);
        Packet.register(1, true, true, LoginHelloPacket.class);
        Packet.register(2, true, true, HandshakePacket.class);
        Packet.register(3, true, true, ChatMessagePacket.class);
        Packet.register(4, true, false, WorldTimeUpdateS2CPacket.class);
        Packet.register(5, true, false, EntityEquipmentUpdateS2CPacket.class);
        Packet.register(6, true, false, PlayerSpawnPositionS2CPacket.class);
        Packet.register(7, false, true, PlayerInteractEntityC2SPacket.class);
        Packet.register(8, true, false, HealthUpdateS2CPacket.class);
        Packet.register(9, true, true, PlayerRespawnPacket.class);
        Packet.register(10, true, true, PlayerMovePacket.class);
        Packet.register(11, true, true, PlayerMovePositionAndOnGroundPacket.class);
        Packet.register(12, true, true, PlayerMoveLookAndOnGroundPacket.class);
        Packet.register(13, true, true, PlayerMoveFullPacket.class);
        Packet.register(14, false, true, PlayerActionC2SPacket.class);
        Packet.register(15, false, true, PlayerInteractBlockC2SPacket.class);
        Packet.register(16, false, true, UpdateSelectedSlotC2SPacket.class);
        Packet.register(17, true, false, PlayerSleepUpdateS2CPacket.class);
        Packet.register(18, true, true, EntityAnimationPacket.class);
        Packet.register(19, false, true, ClientCommandC2SPacket.class);
        Packet.register(20, true, false, PlayerSpawnS2CPacket.class);
        Packet.register(21, true, false, ItemEntitySpawnS2CPacket.class);
        Packet.register(22, true, false, ItemPickupAnimationS2CPacket.class);
        Packet.register(23, true, false, EntitySpawnS2CPacket.class);
        Packet.register(24, true, false, LivingEntitySpawnS2CPacket.class);
        Packet.register(25, true, false, PaintingEntitySpawnS2CPacket.class);
        Packet.register(27, false, true, PlayerInputC2SPacket.class);
        Packet.register(28, true, false, EntityVelocityUpdateS2CPacket.class);
        Packet.register(29, true, false, EntityDestroyS2CPacket.class);
        Packet.register(30, true, false, EntityS2CPacket.class);
        Packet.register(31, true, false, EntityMoveRelativeS2CPacket.class);
        Packet.register(32, true, false, EntityRotateS2CPacket.class);
        Packet.register(33, true, false, EntityRotateAndMoveRelativeS2CPacket.class);
        Packet.register(34, true, false, EntityPositionS2CPacket.class);
        Packet.register(38, true, false, EntityStatusS2CPacket.class);
        Packet.register(39, true, false, EntityVehicleSetS2CPacket.class);
        Packet.register(40, true, false, EntityTrackerUpdateS2CPacket.class);
        Packet.register(50, true, false, ChunkStatusUpdateS2CPacket.class);
        Packet.register(51, true, false, ChunkDataS2CPacket.class);
        Packet.register(52, true, false, ChunkDeltaUpdateS2CPacket.class);
        Packet.register(53, true, false, BlockUpdateS2CPacket.class);
        Packet.register(54, true, false, PlayNoteSoundS2CPacket.class);
        Packet.register(60, true, false, ExplosionS2CPacket.class);
        Packet.register(61, true, false, WorldEventS2CPacket.class);
        Packet.register(70, true, false, GameStateChangeS2CPacket.class);
        Packet.register(71, true, false, GlobalEntitySpawnS2CPacket.class);
        Packet.register(100, true, false, OpenScreenS2CPacket.class);
        Packet.register(101, true, true, CloseScreenS2CPacket.class);
        Packet.register(102, false, true, ClickSlotC2SPacket.class);
        Packet.register(103, true, false, ScreenHandlerSlotUpdateS2CPacket.class);
        Packet.register(104, true, false, InventoryS2CPacket.class);
        Packet.register(105, true, false, ScreenHandlerPropertyUpdateS2CPacket.class);
        Packet.register(106, true, true, ScreenHandlerAcknowledgementPacket.class);
        Packet.register(130, true, true, UpdateSignPacket.class);
        Packet.register(131, true, false, MapUpdateS2CPacket.class);
        Packet.register(200, true, false, IncreaseStatS2CPacket.class);
        Packet.register(255, true, true, DisconnectPacket.class);
        trackers = new HashMap();
        incomingCount = 0;
    }
}

