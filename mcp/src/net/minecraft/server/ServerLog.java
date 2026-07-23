/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.server.ConsoleFormatter;

@Environment(value=EnvType.SERVER)
public class ServerLog {
    public static Logger LOGGER = Logger.getLogger("Minecraft");

    public static void init() {
        ConsoleFormatter consoleFormatter = new ConsoleFormatter();
        LOGGER.setUseParentHandlers(false);
        ConsoleHandler consoleHandler = new ConsoleHandler();
        consoleHandler.setFormatter(consoleFormatter);
        LOGGER.addHandler(consoleHandler);
        try {
            FileHandler fileHandler = new FileHandler("server.log", true);
            fileHandler.setFormatter(consoleFormatter);
            LOGGER.addHandler(fileHandler);
        }
        catch (Exception exception) {
            LOGGER.log(Level.WARNING, "Failed to log to server.log", exception);
        }
    }
}

