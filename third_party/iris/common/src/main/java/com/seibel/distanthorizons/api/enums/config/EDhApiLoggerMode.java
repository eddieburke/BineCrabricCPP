/*
 * Decompiled with CFR 0.152.
 * 
 * Could not load the following classes:
 *  org.apache.logging.log4j.Level
 */
package com.seibel.distanthorizons.api.enums.config;

import org.apache.logging.log4j.Level;

public enum EDhApiLoggerMode {
    DISABLED(Level.OFF, Level.OFF),
    LOG_ALL_TO_FILE(Level.ALL, Level.OFF),
    LOG_ERROR_TO_CHAT(Level.ALL, Level.ERROR),
    LOG_WARNING_TO_CHAT(Level.ALL, Level.WARN),
    LOG_INFO_TO_CHAT(Level.ALL, Level.INFO),
    LOG_DEBUG_TO_CHAT(Level.ALL, Level.DEBUG),
    LOG_ALL_TO_CHAT(Level.ALL, Level.ALL),
    LOG_ERROR_TO_CHAT_AND_FILE(Level.ERROR, Level.ERROR),
    LOG_WARNING_TO_CHAT_AND_FILE(Level.WARN, Level.WARN),
    LOG_INFO_TO_CHAT_AND_FILE(Level.INFO, Level.INFO),
    LOG_DEBUG_TO_CHAT_AND_FILE(Level.DEBUG, Level.DEBUG),
    LOG_WARNING_TO_CHAT_AND_INFO_TO_FILE(Level.INFO, Level.WARN),
    LOG_ERROR_TO_CHAT_AND_INFO_TO_FILE(Level.INFO, Level.ERROR);

    public final Level levelForFile;
    public final Level levelForChat;

    private EDhApiLoggerMode(Level levelForFile, Level levelForChat) {
        this.levelForFile = levelForFile;
        this.levelForChat = levelForChat;
    }
}

