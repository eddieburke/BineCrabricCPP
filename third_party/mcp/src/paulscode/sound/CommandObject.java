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
public class CommandObject {
    public static final int INITIALIZE = 1;
    public static final int LOAD_SOUND = 2;
    public static final int UNLOAD_SOUND = 4;
    public static final int QUEUE_SOUND = 5;
    public static final int DEQUEUE_SOUND = 6;
    public static final int FADE_OUT = 7;
    public static final int FADE_OUT_IN = 8;
    public static final int CHECK_FADE_VOLUMES = 9;
    public static final int NEW_SOURCE = 10;
    public static final int RAW_DATA_STREAM = 11;
    public static final int QUICK_PLAY = 12;
    public static final int SET_POSITION = 13;
    public static final int SET_VOLUME = 14;
    public static final int SET_PITCH = 15;
    public static final int SET_PRIORITY = 16;
    public static final int SET_LOOPING = 17;
    public static final int SET_ATTENUATION = 18;
    public static final int SET_DIST_OR_ROLL = 19;
    public static final int PLAY = 21;
    public static final int FEED_RAW_AUDIO_DATA = 22;
    public static final int PAUSE = 23;
    public static final int STOP = 24;
    public static final int REWIND = 25;
    public static final int FLUSH = 26;
    public static final int CULL = 27;
    public static final int ACTIVATE = 28;
    public static final int SET_TEMPORARY = 29;
    public static final int REMOVE_SOURCE = 30;
    public static final int MOVE_LISTENER = 31;
    public static final int SET_LISTENER_POSITION = 32;
    public static final int TURN_LISTENER = 33;
    public static final int SET_LISTENER_ANGLE = 34;
    public static final int SET_LISTENER_ORIENTATION = 35;
    public static final int SET_MASTER_VOLUME = 36;
    public static final int NEW_LIBRARY = 37;
    public byte[] buffer;
    public int[] intArgs;
    public float[] floatArgs;
    public long[] longArgs;
    public boolean[] boolArgs;
    public String[] stringArgs;
    public Class[] classArgs;
    public Object[] objectArgs;
    public int Command;

    public CommandObject(int i) {
        this.Command = i;
    }

    public CommandObject(int i, int j) {
        this.Command = i;
        this.intArgs = new int[1];
        this.intArgs[0] = j;
    }

    public CommandObject(int i, Class clazz) {
        this.Command = i;
        this.classArgs = new Class[1];
        this.classArgs[0] = clazz;
    }

    public CommandObject(int i, float f) {
        this.Command = i;
        this.floatArgs = new float[1];
        this.floatArgs[0] = f;
    }

    public CommandObject(int i, String string) {
        this.Command = i;
        this.stringArgs = new String[1];
        this.stringArgs[0] = string;
    }

    public CommandObject(int i, Object object) {
        this.Command = i;
        this.objectArgs = new Object[1];
        this.objectArgs[0] = object;
    }

    public CommandObject(int i, String string, Object object) {
        this.Command = i;
        this.stringArgs = new String[1];
        this.stringArgs[0] = string;
        this.objectArgs = new Object[1];
        this.objectArgs[0] = object;
    }

    public CommandObject(int i, String string, byte[] bs) {
        this.Command = i;
        this.stringArgs = new String[1];
        this.stringArgs[0] = string;
        this.buffer = bs;
    }

    public CommandObject(int i, String string, Object object, long l) {
        this.Command = i;
        this.stringArgs = new String[1];
        this.stringArgs[0] = string;
        this.objectArgs = new Object[1];
        this.objectArgs[0] = object;
        this.longArgs = new long[1];
        this.longArgs[0] = l;
    }

    public CommandObject(int i, String string, Object object, long l, long m) {
        this.Command = i;
        this.stringArgs = new String[1];
        this.stringArgs[0] = string;
        this.objectArgs = new Object[1];
        this.objectArgs[0] = object;
        this.longArgs = new long[2];
        this.longArgs[0] = l;
        this.longArgs[1] = m;
    }

    public CommandObject(int i, String string, String string2) {
        this.Command = i;
        this.stringArgs = new String[2];
        this.stringArgs[0] = string;
        this.stringArgs[1] = string2;
    }

    public CommandObject(int i, String string, int j) {
        this.Command = i;
        this.intArgs = new int[1];
        this.stringArgs = new String[1];
        this.intArgs[0] = j;
        this.stringArgs[0] = string;
    }

    public CommandObject(int i, String string, float f) {
        this.Command = i;
        this.floatArgs = new float[1];
        this.stringArgs = new String[1];
        this.floatArgs[0] = f;
        this.stringArgs[0] = string;
    }

    public CommandObject(int i, String string, boolean bl) {
        this.Command = i;
        this.boolArgs = new boolean[1];
        this.stringArgs = new String[1];
        this.boolArgs[0] = bl;
        this.stringArgs[0] = string;
    }

    public CommandObject(int i, float f, float g, float h) {
        this.Command = i;
        this.floatArgs = new float[3];
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
    }

    public CommandObject(int i, String string, float f, float g, float h) {
        this.Command = i;
        this.floatArgs = new float[3];
        this.stringArgs = new String[1];
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
        this.stringArgs[0] = string;
    }

    public CommandObject(int i, float f, float g, float h, float j, float k, float l) {
        this.Command = i;
        this.floatArgs = new float[6];
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
        this.floatArgs[3] = j;
        this.floatArgs[4] = k;
        this.floatArgs[5] = l;
    }

    public CommandObject(int i, boolean bl, boolean bl2, boolean bl3, String string, Object object, float f, float g, float h, int j, float k) {
        this.Command = i;
        this.intArgs = new int[1];
        this.floatArgs = new float[4];
        this.boolArgs = new boolean[3];
        this.stringArgs = new String[1];
        this.objectArgs = new Object[1];
        this.intArgs[0] = j;
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
        this.floatArgs[3] = k;
        this.boolArgs[0] = bl;
        this.boolArgs[1] = bl2;
        this.boolArgs[2] = bl3;
        this.stringArgs[0] = string;
        this.objectArgs[0] = object;
    }

    public CommandObject(int i, boolean bl, boolean bl2, boolean bl3, String string, Object object, float f, float g, float h, int j, float k, boolean bl4) {
        this.Command = i;
        this.intArgs = new int[1];
        this.floatArgs = new float[4];
        this.boolArgs = new boolean[4];
        this.stringArgs = new String[1];
        this.objectArgs = new Object[1];
        this.intArgs[0] = j;
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
        this.floatArgs[3] = k;
        this.boolArgs[0] = bl;
        this.boolArgs[1] = bl2;
        this.boolArgs[2] = bl3;
        this.boolArgs[3] = bl4;
        this.stringArgs[0] = string;
        this.objectArgs[0] = object;
    }

    public CommandObject(int i, Object object, boolean bl, String string, float f, float g, float h, int j, float k) {
        this.Command = i;
        this.intArgs = new int[1];
        this.floatArgs = new float[4];
        this.boolArgs = new boolean[1];
        this.stringArgs = new String[1];
        this.objectArgs = new Object[1];
        this.intArgs[0] = j;
        this.floatArgs[0] = f;
        this.floatArgs[1] = g;
        this.floatArgs[2] = h;
        this.floatArgs[3] = k;
        this.boolArgs[0] = bl;
        this.stringArgs[0] = string;
        this.objectArgs[0] = object;
    }
}

