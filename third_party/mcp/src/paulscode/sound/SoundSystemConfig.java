/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Locale;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.ICodec;
import paulscode.sound.Library;
import paulscode.sound.SoundSystemException;
import paulscode.sound.SoundSystemLogger;

@Environment(value=EnvType.CLIENT)
public class SoundSystemConfig {
    public static final Object THREAD_SYNC = new Object();
    public static final int TYPE_NORMAL = 0;
    public static final int TYPE_STREAMING = 1;
    public static final int ATTENUATION_NONE = 0;
    public static final int ATTENUATION_ROLLOFF = 1;
    public static final int ATTENUATION_LINEAR = 2;
    public static final String EXTENSION_MIDI = ".*[mM][iI][dD][iI]?$";
    public static final String PREFIX_URL = "^[hH][tT][tT][pP]://.*";
    private static SoundSystemLogger logger = null;
    private static LinkedList libraries;
    private static LinkedList codecs;
    private static int numberNormalChannels;
    private static int numberStreamingChannels;
    private static float masterGain;
    private static int defaultAttenuationModel;
    private static float defaultRolloffFactor;
    private static float defaultFadeDistance;
    private static String soundFilesPackage;
    private static int streamingBufferSize;
    private static int numberStreamingBuffers;
    private static int maxFileSize;
    private static int fileChunkSize;
    private static boolean midiCodec;

    public static void addLibrary(Class clazz) {
        if (clazz == null) {
            throw new SoundSystemException("Parameter null in method 'addLibrary'", 2);
        }
        if (!Library.class.isAssignableFrom(clazz)) {
            throw new SoundSystemException("The specified class does not extend class 'Library' in method 'addLibrary'");
        }
        if (libraries == null) {
            libraries = new LinkedList();
        }
        if (!libraries.contains(clazz)) {
            libraries.add(clazz);
        }
    }

    public static void removeLibrary(Class clazz) {
        if (libraries == null || clazz == null) {
            return;
        }
        libraries.remove(clazz);
    }

    public static LinkedList getLibraries() {
        return libraries;
    }

    public static boolean libraryCompatible(Class clazz) {
        if (clazz == null) {
            SoundSystemConfig.errorMessage("Parameter 'libraryClass' null in method'librayCompatible'");
            return false;
        }
        if (!Library.class.isAssignableFrom(clazz)) {
            SoundSystemConfig.errorMessage("The specified class does not extend class 'Library' in method 'libraryCompatible'");
            return false;
        }
        Object object = SoundSystemConfig.runMethod(clazz, "libraryCompatible", new Class[0], new Object[0]);
        if (object == null) {
            SoundSystemConfig.errorMessage("Method 'Library.libraryCompatible' returned 'null' in method 'libraryCompatible'");
            return false;
        }
        return (Boolean)object;
    }

    public static String getLibraryTitle(Class clazz) {
        if (clazz == null) {
            SoundSystemConfig.errorMessage("Parameter 'libraryClass' null in method'getLibrayTitle'");
            return null;
        }
        if (!Library.class.isAssignableFrom(clazz)) {
            SoundSystemConfig.errorMessage("The specified class does not extend class 'Library' in method 'getLibraryTitle'");
            return null;
        }
        Object object = SoundSystemConfig.runMethod(clazz, "getTitle", new Class[0], new Object[0]);
        if (object == null) {
            SoundSystemConfig.errorMessage("Method 'Library.getTitle' returned 'null' in method 'getLibraryTitle'");
            return null;
        }
        return (String)object;
    }

    public static String getLibraryDescription(Class clazz) {
        if (clazz == null) {
            SoundSystemConfig.errorMessage("Parameter 'libraryClass' null in method'getLibrayDescription'");
            return null;
        }
        if (!Library.class.isAssignableFrom(clazz)) {
            SoundSystemConfig.errorMessage("The specified class does not extend class 'Library' in method 'getLibraryDescription'");
            return null;
        }
        Object object = SoundSystemConfig.runMethod(clazz, "getDescription", new Class[0], new Object[0]);
        if (object == null) {
            SoundSystemConfig.errorMessage("Method 'Library.getDescription' returned 'null' in method 'getLibraryDescription'");
            return null;
        }
        return (String)object;
    }

    public static void setLogger(SoundSystemLogger soundSystemLogger) {
        logger = soundSystemLogger;
    }

    public static SoundSystemLogger getLogger() {
        return logger;
    }

    public static synchronized void setNumberNormalChannels(int i) {
        numberNormalChannels = i;
    }

    public static synchronized int getNumberNormalChannels() {
        return numberNormalChannels;
    }

    public static synchronized void setNumberStreamingChannels(int i) {
        numberStreamingChannels = i;
    }

    public static synchronized int getNumberStreamingChannels() {
        return numberStreamingChannels;
    }

    public static synchronized void setMasterGain(float f) {
        masterGain = f;
    }

    public static synchronized float getMasterGain() {
        return masterGain;
    }

    public static synchronized void setDefaultAttenuation(int i) {
        defaultAttenuationModel = i;
    }

    public static synchronized int getDefaultAttenuation() {
        return defaultAttenuationModel;
    }

    public static synchronized void setDefaultRolloff(float f) {
        defaultRolloffFactor = f;
    }

    public static synchronized float getDefaultRolloff() {
        return defaultRolloffFactor;
    }

    public static synchronized void setDefaultFadeDistance(float f) {
        defaultFadeDistance = f;
    }

    public static synchronized float getDefaultFadeDistance() {
        return defaultFadeDistance;
    }

    public static synchronized void setSoundFilesPackage(String string) {
        soundFilesPackage = string;
    }

    public static synchronized String getSoundFilesPackage() {
        return soundFilesPackage;
    }

    public static synchronized void setStreamingBufferSize(int i) {
        streamingBufferSize = i;
    }

    public static synchronized int getStreamingBufferSize() {
        return streamingBufferSize;
    }

    public static synchronized void setNumberStreamingBuffers(int i) {
        numberStreamingBuffers = i;
    }

    public static synchronized int getNumberStreamingBuffers() {
        return numberStreamingBuffers;
    }

    public static synchronized void setMaxFileSize(int i) {
        maxFileSize = i;
    }

    public static synchronized int getMaxFileSize() {
        return maxFileSize;
    }

    public static synchronized void setFileChunkSize(int i) {
        fileChunkSize = i;
    }

    public static synchronized int getFileChunkSize() {
        return fileChunkSize;
    }

    public static synchronized void setCodec(String string, Class clazz) {
        if (string == null) {
            throw new SoundSystemException("Parameter 'extension' null in method 'setCodec'.", 2);
        }
        if (clazz == null) {
            throw new SoundSystemException("Parameter 'iCodecClass' null in method 'setCodec'.", 2);
        }
        if (!ICodec.class.isAssignableFrom(clazz)) {
            throw new SoundSystemException("The specified class does not implement interface 'ICodec' in method 'setCodec'", 3);
        }
        if (codecs == null) {
            codecs = new LinkedList();
        }
        ListIterator listIterator = codecs.listIterator();
        while (listIterator.hasNext()) {
            Codec codec = (Codec)listIterator.next();
            if (!string.matches(codec.extensionRegX)) continue;
            listIterator.remove();
        }
        codecs.add(new Codec(string, clazz));
        if (string.matches(EXTENSION_MIDI)) {
            midiCodec = true;
        }
    }

    public static synchronized ICodec getCodec(String string) {
        if (codecs == null) {
            return null;
        }
        ListIterator listIterator = codecs.listIterator();
        while (listIterator.hasNext()) {
            Codec codec = (Codec)listIterator.next();
            if (!string.matches(codec.extensionRegX)) continue;
            return codec.getInstance();
        }
        return null;
    }

    public static boolean midiCodec() {
        return midiCodec;
    }

    private static void errorMessage(String string) {
        if (logger != null) {
            logger.errorMessage("SoundSystemConfig", string, 0);
        }
    }

    private static Object runMethod(Class clazz, String string, Class[] classs, Object[] objects) {
        Method method = null;
        try {
            method = clazz.getMethod(string, classs);
        }
        catch (NoSuchMethodException noSuchMethodException) {
            SoundSystemConfig.errorMessage("NoSuchMethodException thrown when attempting to call method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        catch (SecurityException securityException) {
            SoundSystemConfig.errorMessage("Access denied when attempting to call method '" + string + "' in method 'runMethod'");
            return null;
        }
        catch (NullPointerException nullPointerException) {
            SoundSystemConfig.errorMessage("NullPointerException thrown when attempting to call method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        if (method == null) {
            SoundSystemConfig.errorMessage("Method '" + string + "' not found for the class " + "specified in method 'runMethod'");
            return null;
        }
        Object object = null;
        try {
            object = method.invoke(null, objects);
        }
        catch (IllegalAccessException illegalAccessException) {
            SoundSystemConfig.errorMessage("IllegalAccessException thrown when attempting to invoke method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        catch (IllegalArgumentException illegalArgumentException) {
            SoundSystemConfig.errorMessage("IllegalArgumentException thrown when attempting to invoke method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        catch (InvocationTargetException invocationTargetException) {
            SoundSystemConfig.errorMessage("InvocationTargetException thrown while attempting to invoke method 'Library.getTitle' in method 'getLibraryTitle'");
            return null;
        }
        catch (NullPointerException nullPointerException) {
            SoundSystemConfig.errorMessage("NullPointerException thrown when attempting to invoke method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        catch (ExceptionInInitializerError exceptionInInitializerError) {
            SoundSystemConfig.errorMessage("ExceptionInInitializerError thrown when attempting to invoke method '" + string + "' in " + "method 'runMethod'");
            return null;
        }
        return object;
    }

    static {
        codecs = null;
        numberNormalChannels = 28;
        numberStreamingChannels = 4;
        masterGain = 1.0f;
        defaultAttenuationModel = 1;
        defaultRolloffFactor = 0.03f;
        defaultFadeDistance = 1000.0f;
        soundFilesPackage = "Sounds/";
        streamingBufferSize = 131072;
        numberStreamingBuffers = 3;
        maxFileSize = 0x10000000;
        fileChunkSize = 0x100000;
        midiCodec = false;
    }

    @Environment(value=EnvType.CLIENT)
    class Codec {
        public String extensionRegX = "";
        public Class iCodecClass;

        public Codec(String string, Class clazz) {
            if (string != null && string.length() > 0) {
                this.extensionRegX = ".*";
                for (int i = 0; i < string.length(); ++i) {
                    String string2 = string.substring(i, i + 1);
                    this.extensionRegX = this.extensionRegX + "[" + string2.toLowerCase(Locale.ENGLISH) + string2.toUpperCase(Locale.ENGLISH) + "]";
                }
                this.extensionRegX = this.extensionRegX + "$";
            }
            this.iCodecClass = clazz;
        }

        public ICodec getInstance() {
            if (this.iCodecClass == null) {
                return null;
            }
            Object var1_1 = null;
            try {
                var1_1 = this.iCodecClass.newInstance();
            }
            catch (InstantiationException instantiationException) {
                this.instantiationErrorMessage();
                return null;
            }
            catch (IllegalAccessException illegalAccessException) {
                this.instantiationErrorMessage();
                return null;
            }
            catch (ExceptionInInitializerError exceptionInInitializerError) {
                this.instantiationErrorMessage();
                return null;
            }
            catch (SecurityException securityException) {
                this.instantiationErrorMessage();
                return null;
            }
            if (var1_1 == null) {
                this.instantiationErrorMessage();
                return null;
            }
            return var1_1;
        }

        private void instantiationErrorMessage() {
            SoundSystemConfig.errorMessage("Unrecognized ICodec implementation in method 'getInstance'.  Ensure that the implementing class has one public, parameterless constructor.");
        }
    }
}

