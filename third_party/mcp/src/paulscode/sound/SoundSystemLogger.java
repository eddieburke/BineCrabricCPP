/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class SoundSystemLogger {
    public void message(String string, int i) {
        String string2 = "";
        for (int j = 0; j < i; ++j) {
            string2 = string2 + "    ";
        }
        String string3 = string2 + string;
        System.out.println(string3);
    }

    public void importantMessage(String string, int i) {
        String string2 = "";
        for (int j = 0; j < i; ++j) {
            string2 = string2 + "    ";
        }
        String string3 = string2 + string;
        System.out.println(string3);
    }

    public boolean errorCheck(boolean print, String string, String string2, int i) {
        if (print) {
            this.errorMessage(string, string2, i);
        }
        return print;
    }

    public void errorMessage(String string, String string2, int i) {
        String string3 = "";
        for (int j = 0; j < i; ++j) {
            string3 = string3 + "    ";
        }
        String string4 = string3 + "Error in class '" + string + "'";
        String string5 = "    " + string3 + string2;
        System.out.println(string4);
        System.out.println(string5);
    }

    public void printStackTrace(Exception exception, int i) {
        this.printExceptionMessage(exception, i);
        this.importantMessage("STACK TRACE:", i);
        if (exception == null) {
            return;
        }
        StackTraceElement[] stackTraceElementArray = exception.getStackTrace();
        if (stackTraceElementArray == null) {
            return;
        }
        for (int j = 0; j < stackTraceElementArray.length; ++j) {
            StackTraceElement stackTraceElement = stackTraceElementArray[j];
            if (stackTraceElement == null) continue;
            this.message(stackTraceElement.toString(), i + 1);
        }
    }

    public void printExceptionMessage(Exception exception, int i) {
        this.importantMessage("ERROR MESSAGE:", i);
        if (exception.getMessage() == null) {
            this.message("(none)", i + 1);
        } else {
            this.message(exception.getMessage(), i + 1);
        }
    }
}

