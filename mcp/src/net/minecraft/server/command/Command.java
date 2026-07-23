/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.command;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.server.command.CommandOutput;

@Environment(value=EnvType.SERVER)
public class Command {
    public final String commandAndArgs;
    public final CommandOutput output;

    public Command(String contents, CommandOutput output) {
        this.commandAndArgs = contents;
        this.output = output;
    }
}

