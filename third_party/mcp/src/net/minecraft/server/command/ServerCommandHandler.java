/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.command;

import java.util.Set;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.PlayerManager;
import net.minecraft.server.command.Command;
import net.minecraft.server.command.CommandOutput;
import net.minecraft.world.ServerWorld;

@Environment(value=EnvType.SERVER)
public class ServerCommandHandler {
    private static Logger logger = Logger.getLogger("Minecraft");
    private MinecraftServer server;

    public ServerCommandHandler(MinecraftServer server) {
        this.server = server;
    }

    public void executeCommand(Command command) {
        block72: {
            String string = command.commandAndArgs;
            CommandOutput commandOutput = command.output;
            String string2 = commandOutput.getName();
            PlayerManager playerManager = this.server.playerManager;
            if (string.toLowerCase().startsWith("help") || string.toLowerCase().startsWith("?")) {
                this.displayHelp(commandOutput);
            } else if (string.toLowerCase().startsWith("list")) {
                commandOutput.sendMessage("Connected players: " + playerManager.getPlayerList());
            } else if (string.toLowerCase().startsWith("stop")) {
                this.logCommand(string2, "Stopping the server..");
                this.server.stop();
            } else if (string.toLowerCase().startsWith("save-all")) {
                this.logCommand(string2, "Forcing save..");
                if (playerManager != null) {
                    playerManager.savePlayers();
                }
                for (int i = 0; i < this.server.worlds.length; ++i) {
                    ServerWorld serverWorld = this.server.worlds[i];
                    serverWorld.saveWithLoadingDisplay(true, null);
                }
                this.logCommand(string2, "Save complete.");
            } else if (string.toLowerCase().startsWith("save-off")) {
                this.logCommand(string2, "Disabling level saving..");
                for (int i = 0; i < this.server.worlds.length; ++i) {
                    ServerWorld serverWorld = this.server.worlds[i];
                    serverWorld.savingDisabled = true;
                }
            } else if (string.toLowerCase().startsWith("save-on")) {
                this.logCommand(string2, "Enabling level saving..");
                for (int i = 0; i < this.server.worlds.length; ++i) {
                    ServerWorld serverWorld = this.server.worlds[i];
                    serverWorld.savingDisabled = false;
                }
            } else if (string.toLowerCase().startsWith("op ")) {
                String string3 = string.substring(string.indexOf(" ")).trim();
                playerManager.addToOperators(string3);
                this.logCommand(string2, "Opping " + string3);
                playerManager.messagePlayer(string3, "\u00a7eYou are now op!");
            } else if (string.toLowerCase().startsWith("deop ")) {
                String string4 = string.substring(string.indexOf(" ")).trim();
                playerManager.removeFromOperators(string4);
                playerManager.messagePlayer(string4, "\u00a7eYou are no longer op!");
                this.logCommand(string2, "De-opping " + string4);
            } else if (string.toLowerCase().startsWith("ban-ip ")) {
                String string5 = string.substring(string.indexOf(" ")).trim();
                playerManager.banIp(string5);
                this.logCommand(string2, "Banning ip " + string5);
            } else if (string.toLowerCase().startsWith("pardon-ip ")) {
                String string6 = string.substring(string.indexOf(" ")).trim();
                playerManager.unbanIp(string6);
                this.logCommand(string2, "Pardoning ip " + string6);
            } else if (string.toLowerCase().startsWith("ban ")) {
                String string7 = string.substring(string.indexOf(" ")).trim();
                playerManager.banPlayer(string7);
                this.logCommand(string2, "Banning " + string7);
                ServerPlayerEntity serverPlayerEntity = playerManager.getPlayer(string7);
                if (serverPlayerEntity != null) {
                    serverPlayerEntity.networkHandler.disconnect("Banned by admin");
                }
            } else if (string.toLowerCase().startsWith("pardon ")) {
                String string8 = string.substring(string.indexOf(" ")).trim();
                playerManager.unbanPlayer(string8);
                this.logCommand(string2, "Pardoning " + string8);
            } else if (string.toLowerCase().startsWith("kick ")) {
                String string9 = string.substring(string.indexOf(" ")).trim();
                ServerPlayerEntity serverPlayerEntity = null;
                for (int i = 0; i < playerManager.players.size(); ++i) {
                    ServerPlayerEntity serverPlayerEntity2 = (ServerPlayerEntity)playerManager.players.get(i);
                    if (!serverPlayerEntity2.name.equalsIgnoreCase(string9)) continue;
                    serverPlayerEntity = serverPlayerEntity2;
                }
                if (serverPlayerEntity != null) {
                    serverPlayerEntity.networkHandler.disconnect("Kicked by admin");
                    this.logCommand(string2, "Kicking " + serverPlayerEntity.name);
                } else {
                    commandOutput.sendMessage("Can't find user " + string9 + ". No kick.");
                }
            } else if (string.toLowerCase().startsWith("tp ")) {
                String[] stringArray = string.split(" ");
                if (stringArray.length == 3) {
                    ServerPlayerEntity serverPlayerEntity = playerManager.getPlayer(stringArray[1]);
                    ServerPlayerEntity serverPlayerEntity3 = playerManager.getPlayer(stringArray[2]);
                    if (serverPlayerEntity == null) {
                        commandOutput.sendMessage("Can't find user " + stringArray[1] + ". No tp.");
                    } else if (serverPlayerEntity3 == null) {
                        commandOutput.sendMessage("Can't find user " + stringArray[2] + ". No tp.");
                    } else if (serverPlayerEntity.dimensionId != serverPlayerEntity3.dimensionId) {
                        commandOutput.sendMessage("User " + stringArray[1] + " and " + stringArray[2] + " are in different dimensions. No tp.");
                    } else {
                        serverPlayerEntity.networkHandler.teleport(serverPlayerEntity3.x, serverPlayerEntity3.y, serverPlayerEntity3.z, serverPlayerEntity3.yaw, serverPlayerEntity3.pitch);
                        this.logCommand(string2, "Teleporting " + stringArray[1] + " to " + stringArray[2] + ".");
                    }
                } else {
                    commandOutput.sendMessage("Syntax error, please provice a source and a target.");
                }
            } else if (string.toLowerCase().startsWith("give ")) {
                String[] stringArray = string.split(" ");
                if (stringArray.length != 3 && stringArray.length != 4) {
                    return;
                }
                String string10 = stringArray[1];
                ServerPlayerEntity serverPlayerEntity = playerManager.getPlayer(string10);
                if (serverPlayerEntity != null) {
                    try {
                        int n = Integer.parseInt(stringArray[2]);
                        if (Item.ITEMS[n] != null) {
                            this.logCommand(string2, "Giving " + serverPlayerEntity.name + " some " + n);
                            int n2 = 1;
                            if (stringArray.length > 3) {
                                n2 = this.parseInt(stringArray[3], 1);
                            }
                            if (n2 < 1) {
                                n2 = 1;
                            }
                            if (n2 > 64) {
                                n2 = 64;
                            }
                            serverPlayerEntity.dropItem(new ItemStack(n, n2, 0));
                            break block72;
                        }
                        commandOutput.sendMessage("There's no item with id " + n);
                    }
                    catch (NumberFormatException numberFormatException) {
                        commandOutput.sendMessage("There's no item with id " + stringArray[2]);
                    }
                } else {
                    commandOutput.sendMessage("Can't find user " + string10);
                }
            } else if (string.toLowerCase().startsWith("time ")) {
                String[] stringArray = string.split(" ");
                if (stringArray.length != 3) {
                    return;
                }
                String string11 = stringArray[1];
                try {
                    int n = Integer.parseInt(stringArray[2]);
                    if ("add".equalsIgnoreCase(string11)) {
                        for (int i = 0; i < this.server.worlds.length; ++i) {
                            ServerWorld serverWorld = this.server.worlds[i];
                            serverWorld.synchronizeTimeAndUpdates(serverWorld.getTime() + (long)n);
                        }
                        this.logCommand(string2, "Added " + n + " to time");
                        break block72;
                    }
                    if ("set".equalsIgnoreCase(string11)) {
                        for (int i = 0; i < this.server.worlds.length; ++i) {
                            ServerWorld serverWorld = this.server.worlds[i];
                            serverWorld.synchronizeTimeAndUpdates(n);
                        }
                        this.logCommand(string2, "Set time to " + n);
                        break block72;
                    }
                    commandOutput.sendMessage("Unknown method, use either \"add\" or \"set\"");
                }
                catch (NumberFormatException numberFormatException) {
                    commandOutput.sendMessage("Unable to convert time value, " + stringArray[2]);
                }
            } else if (string.toLowerCase().startsWith("say ")) {
                string = string.substring(string.indexOf(" ")).trim();
                logger.info("[" + string2 + "] " + string);
                playerManager.sendToAll(new ChatMessagePacket("\u00a7d[Server] " + string));
            } else if (string.toLowerCase().startsWith("tell ")) {
                String[] stringArray = string.split(" ");
                if (stringArray.length >= 3) {
                    string = string.substring(string.indexOf(" ")).trim();
                    string = string.substring(string.indexOf(" ")).trim();
                    logger.info("[" + string2 + "->" + stringArray[1] + "] " + string);
                    string = "\u00a77" + string2 + " whispers " + string;
                    logger.info(string);
                    if (!playerManager.sendPacket(stringArray[1], new ChatMessagePacket(string))) {
                        commandOutput.sendMessage("There's no player by that name online.");
                    }
                }
            } else if (string.toLowerCase().startsWith("whitelist ")) {
                this.executeWhitelist(string2, string, commandOutput);
            } else {
                logger.info("Unknown console command. Type \"help\" for help.");
            }
        }
    }

    private void executeWhitelist(String commandUser, String message, CommandOutput output) {
        String[] stringArray = message.split(" ");
        if (stringArray.length < 2) {
            return;
        }
        String string = stringArray[1].toLowerCase();
        if ("on".equals(string)) {
            this.logCommand(commandUser, "Turned on white-listing");
            this.server.properties.setProperty("white-list", true);
        } else if ("off".equals(string)) {
            this.logCommand(commandUser, "Turned off white-listing");
            this.server.properties.setProperty("white-list", false);
        } else if ("list".equals(string)) {
            Set set = this.server.playerManager.getWhitelist();
            String string2 = "";
            for (String string3 : set) {
                string2 = string2 + string3 + " ";
            }
            output.sendMessage("White-listed players: " + string2);
        } else if ("add".equals(string) && stringArray.length == 3) {
            String string4 = stringArray[2].toLowerCase();
            this.server.playerManager.addToWhitelist(string4);
            this.logCommand(commandUser, "Added " + string4 + " to white-list");
        } else if ("remove".equals(string) && stringArray.length == 3) {
            String string5 = stringArray[2].toLowerCase();
            this.server.playerManager.removeFromWhitelist(string5);
            this.logCommand(commandUser, "Removed " + string5 + " from white-list");
        } else if ("reload".equals(string)) {
            this.server.playerManager.reloadWhitelist();
            this.logCommand(commandUser, "Reloaded white-list from file");
        }
    }

    private void displayHelp(CommandOutput output) {
        output.sendMessage("To run the server without a gui, start it like this:");
        output.sendMessage("   java -Xmx1024M -Xms1024M -jar minecraft_server.jar nogui");
        output.sendMessage("Console commands:");
        output.sendMessage("   help  or  ?               shows this message");
        output.sendMessage("   kick <player>             removes a player from the server");
        output.sendMessage("   ban <player>              bans a player from the server");
        output.sendMessage("   pardon <player>           pardons a banned player so that they can connect again");
        output.sendMessage("   ban-ip <ip>               bans an IP address from the server");
        output.sendMessage("   pardon-ip <ip>            pardons a banned IP address so that they can connect again");
        output.sendMessage("   op <player>               turns a player into an op");
        output.sendMessage("   deop <player>             removes op status from a player");
        output.sendMessage("   tp <player1> <player2>    moves one player to the same location as another player");
        output.sendMessage("   give <player> <id> [num]  gives a player a resource");
        output.sendMessage("   tell <player> <message>   sends a private message to a player");
        output.sendMessage("   stop                      gracefully stops the server");
        output.sendMessage("   save-all                  forces a server-wide level save");
        output.sendMessage("   save-off                  disables terrain saving (useful for backup scripts)");
        output.sendMessage("   save-on                   re-enables terrain saving");
        output.sendMessage("   list                      lists all currently connected players");
        output.sendMessage("   say <message>             broadcasts a message to all players");
        output.sendMessage("   time <add|set> <amount>   adds to or sets the world time (0-24000)");
    }

    private void logCommand(String commandUser, String message) {
        String string = commandUser + ": " + message;
        this.server.playerManager.broadcast("\u00a77(" + string + ")");
        logger.info(string);
    }

    private int parseInt(String string, int fallback) {
        try {
            return Integer.parseInt(string);
        }
        catch (NumberFormatException numberFormatException) {
            return fallback;
        }
    }
}

