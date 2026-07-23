/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.SERVER)
final class ConsoleFormatter
extends Formatter {
    private SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

    ConsoleFormatter() {
    }

    public String format(LogRecord logRecord) {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(this.dateFormat.format(logRecord.getMillis()));
        Level level = logRecord.getLevel();
        if (level == Level.FINEST) {
            stringBuilder.append(" [FINEST] ");
        } else if (level == Level.FINER) {
            stringBuilder.append(" [FINER] ");
        } else if (level == Level.FINE) {
            stringBuilder.append(" [FINE] ");
        } else if (level == Level.INFO) {
            stringBuilder.append(" [INFO] ");
        } else if (level == Level.WARNING) {
            stringBuilder.append(" [WARNING] ");
        } else if (level == Level.SEVERE) {
            stringBuilder.append(" [SEVERE] ");
        } else if (level == Level.SEVERE) {
            stringBuilder.append(" [" + level.getLocalizedName() + "] ");
        }
        stringBuilder.append(logRecord.getMessage());
        stringBuilder.append('\n');
        Throwable throwable = logRecord.getThrown();
        if (throwable != null) {
            StringWriter stringWriter = new StringWriter();
            throwable.printStackTrace(new PrintWriter(stringWriter));
            stringBuilder.append(stringWriter.toString());
        }
        return stringBuilder.toString();
    }
}

