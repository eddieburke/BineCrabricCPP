/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.network;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.network.Connection;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.play.ClickSlotC2SPacket;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInputC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.DisconnectPacket;
import net.minecraft.network.packet.play.EntityAnimationPacket;
import net.minecraft.network.packet.play.KeepAlivePacket;
import net.minecraft.network.packet.play.PlayerMoveFullPacket;
import net.minecraft.network.packet.play.PlayerMovePacket;
import net.minecraft.network.packet.play.PlayerRespawnPacket;
import net.minecraft.network.packet.play.ScreenHandlerAcknowledgementPacket;
import net.minecraft.network.packet.play.UpdateSignPacket;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.CloseScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerSlotUpdateS2CPacket;
import net.minecraft.screen.slot.Slot;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.command.CommandOutput;
import net.minecraft.util.CharacterUtils;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;

@Environment(value=EnvType.SERVER)
public class ServerPlayNetworkHandler
extends NetworkHandler
implements CommandOutput {
    public static Logger LOGGER = Logger.getLogger("Minecraft");
    public Connection connection;
    public boolean disconnected = false;
    private MinecraftServer server;
    private ServerPlayerEntity player;
    private int ticks;
    private int lastKeepAliveTime;
    private int floatingTime;
    private boolean moved;
    private double teleportTargetX;
    private double teleportTargetY;
    private double teleportTargetZ;
    private boolean teleported = true;
    private Map transactions = new HashMap();

    public ServerPlayNetworkHandler(MinecraftServer server, Connection connection, ServerPlayerEntity player) {
        this.server = server;
        this.connection = connection;
        connection.setNetworkHandler(this);
        this.player = player;
        player.networkHandler = this;
    }

    public void tick() {
        this.moved = false;
        this.connection.tick();
        if (this.ticks - this.lastKeepAliveTime > 20) {
            this.sendPacket(new KeepAlivePacket());
        }
    }

    public void disconnect(String reason) {
        this.player.onDisconnect();
        this.sendPacket(new DisconnectPacket(reason));
        this.connection.disconnect();
        this.server.playerManager.sendToAll(new ChatMessagePacket("\u00a7e" + this.player.name + " left the game."));
        this.server.playerManager.disconnect(this.player);
        this.disconnected = true;
    }

    public void onPlayerInput(PlayerInputC2SPacket packet) {
        this.player.updateInput(packet.getSideways(), packet.getForward(), packet.isJumping(), packet.isSneaking(), packet.getPitch(), packet.getYaw());
    }

    public void onPlayerMove(PlayerMovePacket packet) {
        double d;
        ServerWorld serverWorld = this.server.getWorld(this.player.dimensionId);
        this.moved = true;
        if (!this.teleported) {
            d = packet.y - this.teleportTargetY;
            if (packet.x == this.teleportTargetX && d * d < 0.01 && packet.z == this.teleportTargetZ) {
                this.teleported = true;
            }
        }
        if (this.teleported) {
            boolean bl;
            double d2;
            if (this.player.vehicle != null) {
                float f = this.player.yaw;
                float f2 = this.player.pitch;
                this.player.vehicle.updatePassengerPosition();
                double d3 = this.player.x;
                double d4 = this.player.y;
                double d5 = this.player.z;
                double d6 = 0.0;
                double d7 = 0.0;
                if (packet.changeLook) {
                    f = packet.yaw;
                    f2 = packet.pitch;
                }
                if (packet.changePosition && packet.y == -999.0 && packet.eyeHeight == -999.0) {
                    d6 = packet.x;
                    d7 = packet.z;
                }
                this.player.onGround = packet.onGround;
                this.player.playerTick(true);
                this.player.move(d6, 0.0, d7);
                this.player.setPositionAndAngles(d3, d4, d5, f, f2);
                this.player.velocityX = d6;
                this.player.velocityZ = d7;
                if (this.player.vehicle != null) {
                    serverWorld.tickVehicle(this.player.vehicle, true);
                }
                if (this.player.vehicle != null) {
                    this.player.vehicle.updatePassengerPosition();
                }
                this.server.playerManager.updatePlayerChunks(this.player);
                this.teleportTargetX = this.player.x;
                this.teleportTargetY = this.player.y;
                this.teleportTargetZ = this.player.z;
                serverWorld.updateEntity(this.player);
                return;
            }
            if (this.player.isSleeping()) {
                this.player.playerTick(true);
                this.player.setPositionAndAngles(this.teleportTargetX, this.teleportTargetY, this.teleportTargetZ, this.player.yaw, this.player.pitch);
                serverWorld.updateEntity(this.player);
                return;
            }
            d = this.player.y;
            this.teleportTargetX = this.player.x;
            this.teleportTargetY = this.player.y;
            this.teleportTargetZ = this.player.z;
            double d8 = this.player.x;
            double d9 = this.player.y;
            double d10 = this.player.z;
            float f = this.player.yaw;
            float f3 = this.player.pitch;
            if (packet.changePosition && packet.y == -999.0 && packet.eyeHeight == -999.0) {
                packet.changePosition = false;
            }
            if (packet.changePosition) {
                d8 = packet.x;
                d9 = packet.y;
                d10 = packet.z;
                d2 = packet.eyeHeight - packet.y;
                if (!this.player.isSleeping() && (d2 > 1.65 || d2 < 0.1)) {
                    this.disconnect("Illegal stance");
                    LOGGER.warning(this.player.name + " had an illegal stance: " + d2);
                    return;
                }
                if (Math.abs(packet.x) > 3.2E7 || Math.abs(packet.z) > 3.2E7) {
                    this.disconnect("Illegal position");
                    return;
                }
            }
            if (packet.changeLook) {
                f = packet.yaw;
                f3 = packet.pitch;
            }
            this.player.playerTick(true);
            this.player.cameraOffset = 0.0f;
            this.player.setPositionAndAngles(this.teleportTargetX, this.teleportTargetY, this.teleportTargetZ, f, f3);
            if (!this.teleported) {
                return;
            }
            d2 = d8 - this.player.x;
            double d11 = d9 - this.player.y;
            double d12 = d10 - this.player.z;
            double d13 = d2 * d2 + d11 * d11 + d12 * d12;
            if (d13 > 100.0) {
                LOGGER.warning(this.player.name + " moved too quickly!");
                this.disconnect("You moved too quickly :( (Hacking?)");
                return;
            }
            float f4 = 0.0625f;
            boolean bl2 = serverWorld.getEntityCollisions(this.player, this.player.boundingBox.copy().contract(f4, f4, f4)).size() == 0;
            this.player.move(d2, d11, d12);
            d2 = d8 - this.player.x;
            d11 = d9 - this.player.y;
            if (d11 > -0.5 || d11 < 0.5) {
                d11 = 0.0;
            }
            d12 = d10 - this.player.z;
            d13 = d2 * d2 + d11 * d11 + d12 * d12;
            boolean bl3 = false;
            if (d13 > 0.0625 && !this.player.isSleeping()) {
                bl3 = true;
                LOGGER.warning(this.player.name + " moved wrongly!");
                System.out.println("Got position " + d8 + ", " + d9 + ", " + d10);
                System.out.println("Expected " + this.player.x + ", " + this.player.y + ", " + this.player.z);
            }
            this.player.setPositionAndAngles(d8, d9, d10, f, f3);
            boolean bl4 = bl = serverWorld.getEntityCollisions(this.player, this.player.boundingBox.copy().contract(f4, f4, f4)).size() == 0;
            if (bl2 && (bl3 || !bl) && !this.player.isSleeping()) {
                this.teleport(this.teleportTargetX, this.teleportTargetY, this.teleportTargetZ, f, f3);
                return;
            }
            Box box = this.player.boundingBox.copy().expand(f4, f4, f4).stretch(0.0, -0.55, 0.0);
            if (!this.server.flightEnabled && !serverWorld.isAnyBlockInBox(box)) {
                if (d11 >= -0.03125) {
                    ++this.floatingTime;
                    if (this.floatingTime > 80) {
                        LOGGER.warning(this.player.name + " was kicked for floating too long!");
                        this.disconnect("Flying is not enabled on this server");
                        return;
                    }
                }
            } else {
                this.floatingTime = 0;
            }
            this.player.onGround = packet.onGround;
            this.server.playerManager.updatePlayerChunks(this.player);
            this.player.handleFall(this.player.y - d, packet.onGround);
        }
    }

    public void teleport(double x, double y, double z, float yaw, float pitch) {
        this.teleported = false;
        this.teleportTargetX = x;
        this.teleportTargetY = y;
        this.teleportTargetZ = z;
        this.player.setPositionAndAngles(x, y, z, yaw, pitch);
        this.player.networkHandler.sendPacket(new PlayerMoveFullPacket(x, y + (double)1.62f, y, z, yaw, pitch, false));
    }

    public void handlePlayerAction(PlayerActionC2SPacket packet) {
        double d;
        double d2;
        double d3;
        double d4;
        int n;
        double d5;
        double d6;
        double d7;
        double d8;
        ServerWorld serverWorld = this.server.getWorld(this.player.dimensionId);
        if (packet.action == 4) {
            this.player.dropSelectedItem();
            return;
        }
        serverWorld.bypassSpawnProtection = serverWorld.dimension.id != 0 || this.server.playerManager.isOperator(this.player.name);
        boolean bl = serverWorld.bypassSpawnProtection;
        boolean bl2 = false;
        if (packet.action == 0) {
            bl2 = true;
        }
        if (packet.action == 2) {
            bl2 = true;
        }
        int n2 = packet.x;
        int n3 = packet.y;
        int n4 = packet.z;
        if (bl2 && (d8 = (d7 = this.player.x - ((double)n2 + 0.5)) * d7 + (d6 = this.player.y - ((double)n3 + 0.5)) * d6 + (d5 = this.player.z - ((double)n4 + 0.5)) * d5) > 36.0) {
            return;
        }
        Vec3i vec3i = serverWorld.getSpawnPos();
        int n5 = (int)MathHelper.abs(n2 - vec3i.x);
        if (n5 > (n = (int)MathHelper.abs(n4 - vec3i.z))) {
            n = n5;
        }
        if (packet.action == 0) {
            if (n > 16 || bl) {
                this.player.interactionManager.onBlockBreakingAction(n2, n3, n4, packet.direction);
            } else {
                this.player.networkHandler.sendPacket(new BlockUpdateS2CPacket(n2, n3, n4, serverWorld));
            }
        } else if (packet.action == 2) {
            this.player.interactionManager.continueMining(n2, n3, n4);
            if (serverWorld.getBlockId(n2, n3, n4) != 0) {
                this.player.networkHandler.sendPacket(new BlockUpdateS2CPacket(n2, n3, n4, serverWorld));
            }
        } else if (packet.action == 3 && (d4 = (d3 = this.player.x - ((double)n2 + 0.5)) * d3 + (d2 = this.player.y - ((double)n3 + 0.5)) * d2 + (d = this.player.z - ((double)n4 + 0.5)) * d) < 256.0) {
            this.player.networkHandler.sendPacket(new BlockUpdateS2CPacket(n2, n3, n4, serverWorld));
        }
        serverWorld.bypassSpawnProtection = false;
    }

    public void onPlayerInteractBlock(PlayerInteractBlockC2SPacket packet) {
        ServerWorld serverWorld = this.server.getWorld(this.player.dimensionId);
        ItemStack itemStack = this.player.inventory.getSelectedItem();
        serverWorld.bypassSpawnProtection = serverWorld.dimension.id != 0 || this.server.playerManager.isOperator(this.player.name);
        boolean bl = serverWorld.bypassSpawnProtection;
        if (packet.side == 255) {
            if (itemStack == null) {
                return;
            }
            this.player.interactionManager.interactItem(this.player, serverWorld, itemStack);
        } else {
            int n;
            int n2 = packet.x;
            int n3 = packet.y;
            int n4 = packet.z;
            int n5 = packet.side;
            Vec3i vec3i = serverWorld.getSpawnPos();
            int n6 = (int)MathHelper.abs(n2 - vec3i.x);
            if (n6 > (n = (int)MathHelper.abs(n4 - vec3i.z))) {
                n = n6;
            }
            if (this.teleported && this.player.getSquaredDistance((double)n2 + 0.5, (double)n3 + 0.5, (double)n4 + 0.5) < 64.0 && (n > 16 || bl)) {
                this.player.interactionManager.interactBlock(this.player, serverWorld, itemStack, n2, n3, n4, n5);
            }
            this.player.networkHandler.sendPacket(new BlockUpdateS2CPacket(n2, n3, n4, serverWorld));
            if (n5 == 0) {
                --n3;
            }
            if (n5 == 1) {
                ++n3;
            }
            if (n5 == 2) {
                --n4;
            }
            if (n5 == 3) {
                ++n4;
            }
            if (n5 == 4) {
                --n2;
            }
            if (n5 == 5) {
                ++n2;
            }
            this.player.networkHandler.sendPacket(new BlockUpdateS2CPacket(n2, n3, n4, serverWorld));
        }
        itemStack = this.player.inventory.getSelectedItem();
        if (itemStack != null && itemStack.count == 0) {
            this.player.inventory.main[this.player.inventory.selectedSlot] = null;
        }
        this.player.skipPacketSlotUpdates = true;
        this.player.inventory.main[this.player.inventory.selectedSlot] = ItemStack.clone(this.player.inventory.main[this.player.inventory.selectedSlot]);
        Slot slot = this.player.currentScreenHandler.getSlot(this.player.inventory, this.player.inventory.selectedSlot);
        this.player.currentScreenHandler.sendContentUpdates();
        this.player.skipPacketSlotUpdates = false;
        if (!ItemStack.areEqual(this.player.inventory.getSelectedItem(), packet.stack)) {
            this.sendPacket(new ScreenHandlerSlotUpdateS2CPacket(this.player.currentScreenHandler.syncId, slot.id, this.player.inventory.getSelectedItem()));
        }
        serverWorld.bypassSpawnProtection = false;
    }

    public void onDisconnected(String reason, Object[] objects) {
        LOGGER.info(this.player.name + " lost connection: " + reason);
        this.server.playerManager.sendToAll(new ChatMessagePacket("\u00a7e" + this.player.name + " left the game."));
        this.server.playerManager.disconnect(this.player);
        this.disconnected = true;
    }

    public void handle(Packet packet) {
        LOGGER.warning(this.getClass() + " wasn't prepared to deal with a " + packet.getClass());
        this.disconnect("Protocol error, unexpected packet");
    }

    public void sendPacket(Packet packet) {
        this.connection.sendPacket(packet);
        this.lastKeepAliveTime = this.ticks;
    }

    public void onUpdateSelectedSlot(UpdateSelectedSlotC2SPacket packet) {
        if (packet.selectedSlot < 0 || packet.selectedSlot > PlayerInventory.getHotbarSize()) {
            LOGGER.warning(this.player.name + " tried to set an invalid carried item");
            return;
        }
        this.player.inventory.selectedSlot = packet.selectedSlot;
    }

    public void onChatMessage(ChatMessagePacket packet) {
        String string = packet.chatMessage;
        if (string.length() > 100) {
            this.disconnect("Chat message too long");
            return;
        }
        string = string.trim();
        for (int i = 0; i < string.length(); ++i) {
            if (CharacterUtils.VALID_CHARACTERS.indexOf(string.charAt(i)) >= 0) continue;
            this.disconnect("Illegal characters in chat");
            return;
        }
        if (string.startsWith("/")) {
            this.handleCommand(string);
        } else {
            string = "<" + this.player.name + "> " + string;
            LOGGER.info(string);
            this.server.playerManager.sendToAll(new ChatMessagePacket(string));
        }
    }

    private void handleCommand(String message) {
        if (message.toLowerCase().startsWith("/me ")) {
            message = "* " + this.player.name + " " + message.substring(message.indexOf(" ")).trim();
            LOGGER.info(message);
            this.server.playerManager.sendToAll(new ChatMessagePacket(message));
        } else if (message.toLowerCase().startsWith("/kill")) {
            this.player.damage(null, 1000);
        } else if (message.toLowerCase().startsWith("/tell ")) {
            String[] stringArray = message.split(" ");
            if (stringArray.length >= 3) {
                message = message.substring(message.indexOf(" ")).trim();
                message = message.substring(message.indexOf(" ")).trim();
                message = "\u00a77" + this.player.name + " whispers " + message;
                LOGGER.info(message + " to " + stringArray[1]);
                if (!this.server.playerManager.sendPacket(stringArray[1], new ChatMessagePacket(message))) {
                    this.sendPacket(new ChatMessagePacket("\u00a7cThere's no player by that name online."));
                }
            }
        } else if (this.server.playerManager.isOperator(this.player.name)) {
            String string = message.substring(1);
            LOGGER.info(this.player.name + " issued server command: " + string);
            this.server.queueCommands(string, this);
        } else {
            String string = message.substring(1);
            LOGGER.info(this.player.name + " tried command: " + string);
        }
    }

    public void onEntityAnimation(EntityAnimationPacket packet) {
        if (packet.animationId == 1) {
            this.player.swingHand();
        }
    }

    public void handleClientCommand(ClientCommandC2SPacket packet) {
        if (packet.mode == 1) {
            this.player.setSneaking(true);
        } else if (packet.mode == 2) {
            this.player.setSneaking(false);
        } else if (packet.mode == 3) {
            this.player.wakeUp(false, true, true);
            this.teleported = false;
        }
    }

    public void onDisconnect(DisconnectPacket packet) {
        this.connection.disconnect("disconnect.quitting", new Object[0]);
    }

    public int getBlockDataSendQueueSize() {
        return this.connection.getDelayedSendQueueSize();
    }

    public void sendMessage(String message) {
        this.sendPacket(new ChatMessagePacket("\u00a77" + message));
    }

    public String getName() {
        return this.player.name;
    }

    public void handleInteractEntity(PlayerInteractEntityC2SPacket packet) {
        ServerWorld serverWorld = this.server.getWorld(this.player.dimensionId);
        Entity entity = serverWorld.getEntity(packet.entityId);
        if (entity != null && this.player.canSee(entity) && this.player.getSquaredDistance(entity) < 36.0) {
            if (packet.isLeftClick == 0) {
                this.player.interact(entity);
            } else if (packet.isLeftClick == 1) {
                this.player.attack(entity);
            }
        }
    }

    public void onPlayerRespawn(PlayerRespawnPacket packet) {
        if (this.player.health > 0) {
            return;
        }
        this.player = this.server.playerManager.respawnPlayer(this.player, 0);
    }

    public void onCloseScreen(CloseScreenS2CPacket packet) {
        this.player.onHandledScreenClosed();
    }

    public void onClickSlot(ClickSlotC2SPacket packet) {
        if (this.player.currentScreenHandler.syncId == packet.syncId && this.player.currentScreenHandler.canOpen(this.player)) {
            ItemStack itemStack = this.player.currentScreenHandler.onSlotClick(packet.slot, packet.button, packet.holdingShift, this.player);
            if (ItemStack.areEqual(packet.stack, itemStack)) {
                this.player.networkHandler.sendPacket(new ScreenHandlerAcknowledgementPacket(packet.syncId, packet.actionType, true));
                this.player.skipPacketSlotUpdates = true;
                this.player.currentScreenHandler.sendContentUpdates();
                this.player.updateCursorStack();
                this.player.skipPacketSlotUpdates = false;
            } else {
                this.transactions.put(this.player.currentScreenHandler.syncId, packet.actionType);
                this.player.networkHandler.sendPacket(new ScreenHandlerAcknowledgementPacket(packet.syncId, packet.actionType, false));
                this.player.currentScreenHandler.updatePlayerList(this.player, false);
                ArrayList<ItemStack> arrayList = new ArrayList<ItemStack>();
                for (int i = 0; i < this.player.currentScreenHandler.slots.size(); ++i) {
                    arrayList.add(((Slot)this.player.currentScreenHandler.slots.get(i)).getStack());
                }
                this.player.onContentsUpdate(this.player.currentScreenHandler, arrayList);
            }
        }
    }

    public void onScreenHandlerAcknowledgement(ScreenHandlerAcknowledgementPacket packet) {
        Short s = (Short)this.transactions.get(this.player.currentScreenHandler.syncId);
        if (s != null && packet.actionType == s && this.player.currentScreenHandler.syncId == packet.syncId && !this.player.currentScreenHandler.canOpen(this.player)) {
            this.player.currentScreenHandler.updatePlayerList(this.player, true);
        }
    }

    public void handleUpdateSign(UpdateSignPacket packet) {
        ServerWorld serverWorld = this.server.getWorld(this.player.dimensionId);
        if (serverWorld.isPosLoaded(packet.x, packet.y, packet.z)) {
            int n;
            int n2;
            int n3;
            SignBlockEntity signBlockEntity;
            BlockEntity blockEntity = serverWorld.getBlockEntity(packet.x, packet.y, packet.z);
            if (blockEntity instanceof SignBlockEntity && !(signBlockEntity = (SignBlockEntity)blockEntity).isEditable()) {
                this.server.warn("Player " + this.player.name + " just tried to change non-editable sign");
                return;
            }
            for (n3 = 0; n3 < 4; ++n3) {
                n2 = 1;
                if (packet.text[n3].length() > 15) {
                    n2 = 0;
                } else {
                    for (n = 0; n < packet.text[n3].length(); ++n) {
                        if (CharacterUtils.VALID_CHARACTERS.indexOf(packet.text[n3].charAt(n)) >= 0) continue;
                        n2 = 0;
                    }
                }
                if (n2 != 0) continue;
                packet.text[n3] = "!?";
            }
            if (blockEntity instanceof SignBlockEntity) {
                n3 = packet.x;
                n2 = packet.y;
                n = packet.z;
                SignBlockEntity signBlockEntity2 = (SignBlockEntity)blockEntity;
                for (int i = 0; i < 4; ++i) {
                    signBlockEntity2.texts[i] = packet.text[i];
                }
                signBlockEntity2.setEditable(false);
                signBlockEntity2.markDirty();
                serverWorld.blockUpdateEvent(n3, n2, n);
            }
        }
    }

    public boolean isServerSide() {
        return true;
    }
}

