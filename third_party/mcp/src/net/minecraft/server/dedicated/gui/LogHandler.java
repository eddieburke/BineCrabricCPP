/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.dedicated.gui;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import javax.swing.JTextArea;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.SERVER)
public class LogHandler
extends Handler {
    private int[] lineCharCounts = new int[1024];
    private int bufferIndex = 0;
    Formatter formatter = new Formatter(){

        public String format(LogRecord logRecord) {
            StringBuilder stringBuilder = new StringBuilder();
            Level level = logRecord.getLevel();
            if (level == Level.FINEST) {
                stringBuilder.append("[FINEST] ");
            } else if (level == Level.FINER) {
                stringBuilder.append("[FINER] ");
            } else if (level == Level.FINE) {
                stringBuilder.append("[FINE] ");
            } else if (level == Level.INFO) {
                stringBuilder.append("[INFO] ");
            } else if (level == Level.WARNING) {
                stringBuilder.append("[WARNING] ");
            } else if (level == Level.SEVERE) {
                stringBuilder.append("[SEVERE] ");
            } else if (level == Level.SEVERE) {
                stringBuilder.append("[" + level.getLocalizedName() + "] ");
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
    };
    private JTextArea textArea;

    public LogHandler(JTextArea textArea) {
        this.setFormatter(this.formatter);
        this.textArea = textArea;
    }

    public void close() {
    }

    public void flush() {
    }

    public void publish(LogRecord logRecord) {
        int n = this.textArea.getDocument().getLength();
        this.textArea.append(this.formatter.format(logRecord));
        this.textArea.setCaretPosition(this.textArea.getDocument().getLength());
        int n2 = this.textArea.getDocument().getLength() - n;
        if (this.lineCharCounts[this.bufferIndex] != 0) {
            this.textArea.replaceRange("", 0, this.lineCharCounts[this.bufferIndex]);
        }
        this.lineCharCounts[this.bufferIndex] = n2;
        this.bufferIndex = (this.bufferIndex + 1) % 1024;
    }
}

