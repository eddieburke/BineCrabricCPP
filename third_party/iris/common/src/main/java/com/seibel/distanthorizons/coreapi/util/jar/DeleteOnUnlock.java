/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util.jar;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URLDecoder;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.TimeUnit;

public class DeleteOnUnlock {
    public static int SUCCESS_EXIT_CODE = 0;
    public static int FAIL_EXIT_CODE = 1;
    public static int ERROR_EXIT_CODE = 2;
    private static final int ATTEMPT_SPEED_IN_MS = 100;
    private static final int TIMEOUT_IN_MINUTES = 60;
    private static FileWriter logFileWriter;

    public static void main(String[] args) {
        String filePathToDelete = args[0];
        String logFilePath = null;
        if (args.length >= 2) {
            logFilePath = args[1];
        }
        try {
            if (logFilePath != null && logFilePath.trim().length() != 0) {
                File logFile = new File(logFilePath);
                logFileWriter = new FileWriter(logFile, true);
                try {
                    if (!logFile.createNewFile() && !logFile.exists()) {
                        System.err.println("Unable to create log file at: [" + logFile.getPath() + "]");
                    }
                }
                catch (IOException e) {
                    System.err.println(e.getMessage());
                }
            }
            File fileToDelete = new File(URLDecoder.decode(filePathToDelete, "UTF-8"));
            DeleteOnUnlock.log("starting deletion loop... Attempting to delete: [" + fileToDelete.getPath() + "].");
            int i = 0;
            while ((float)i < 36000.0f) {
                DeleteOnUnlock.log("delete attempt [" + i + "]");
                if (fileToDelete.exists() && fileToDelete.renameTo(fileToDelete)) {
                    try {
                        Files.delete(fileToDelete.toPath());
                        if (!fileToDelete.exists()) {
                            DeleteOnUnlock.log("success");
                            break;
                        }
                        DeleteOnUnlock.log("failed to delete without error");
                    }
                    catch (NoSuchFileException e) {
                        DeleteOnUnlock.log("no file found");
                        break;
                    }
                    catch (Exception e) {
                        DeleteOnUnlock.log("failed to delete with error: " + e.getMessage());
                    }
                }
                TimeUnit.MILLISECONDS.sleep(100L);
                ++i;
            }
            boolean programSuccess = !fileToDelete.exists();
            DeleteOnUnlock.log("delete program completed " + (programSuccess ? "successfully" : "unsuccessfully"));
            System.exit(programSuccess ? SUCCESS_EXIT_CODE : FAIL_EXIT_CODE);
        }
        catch (Exception e) {
            String stackTrace = "";
            for (StackTraceElement stackTraceElement : e.getStackTrace()) {
                stackTrace = stackTrace + stackTraceElement.toString() + "\n";
            }
            DeleteOnUnlock.log("Unexpected exception occurred: " + e.getMessage() + "\n\n" + stackTrace);
            e.printStackTrace();
            System.exit(ERROR_EXIT_CODE);
        }
    }

    private static void log(String message) {
        if (logFileWriter != null) {
            try {
                String localDateTime = LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE) + " " + LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_TIME);
                logFileWriter.write(localDateTime + " - " + message + "\n");
                logFileWriter.flush();
            }
            catch (IOException e) {
                System.err.println("Error writing to log: " + e.getMessage());
                e.printStackTrace();
            }
        }
    }
}

