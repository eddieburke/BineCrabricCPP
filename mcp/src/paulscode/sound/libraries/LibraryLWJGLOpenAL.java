/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound.libraries;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import javax.sound.sampled.AudioFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import org.lwjgl.BufferUtils;
import org.lwjgl.LWJGLException;
import org.lwjgl.openal.AL;
import org.lwjgl.openal.AL10;
import paulscode.sound.Channel;
import paulscode.sound.FilenameURL;
import paulscode.sound.ICodec;
import paulscode.sound.Library;
import paulscode.sound.ListenerData;
import paulscode.sound.SoundBuffer;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemException;
import paulscode.sound.Source;
import paulscode.sound.libraries.ChannelLWJGLOpenAL;
import paulscode.sound.libraries.SourceLWJGLOpenAL;

@Environment(value=EnvType.CLIENT)
public class LibraryLWJGLOpenAL
extends Library {
    private static final boolean GET = false;
    private static final boolean SET = true;
    private static final boolean XXX = false;
    private FloatBuffer listenerPositionAL = null;
    private FloatBuffer listenerOrientation = null;
    private FloatBuffer listenerVelocity = null;
    private HashMap ALBufferMap = new HashMap();
    private static boolean alPitchSupported = true;

    public void init() {
        boolean bl = false;
        try {
            AL.create();
            bl = this.checkALError();
        }
        catch (LWJGLException lWJGLException) {
            this.errorMessage("Unable to initialize OpenAL.  Probable cause: OpenAL not supported.");
            this.printStackTrace(lWJGLException);
            throw new SoundSystemException(lWJGLException.getMessage(), 6);
        }
        if (bl) {
            this.importantMessage("OpenAL did not initialize properly!");
        } else {
            this.message("OpenAL initialized.");
        }
        this.listenerPositionAL = BufferUtils.createFloatBuffer(3).put(new float[]{this.listener.position.x, this.listener.position.y, this.listener.position.z});
        this.listenerOrientation = BufferUtils.createFloatBuffer(6).put(new float[]{this.listener.lookAt.x, this.listener.lookAt.y, this.listener.lookAt.z, this.listener.up.x, this.listener.up.y, this.listener.up.z});
        this.listenerVelocity = BufferUtils.createFloatBuffer(3).put(new float[]{0.0f, 0.0f, 0.0f});
        this.listenerPositionAL.flip();
        this.listenerOrientation.flip();
        this.listenerVelocity.flip();
        AL10.alListener(4100, this.listenerPositionAL);
        bl = this.checkALError() || bl;
        AL10.alListener(4111, this.listenerOrientation);
        bl = this.checkALError() || bl;
        AL10.alListener(4102, this.listenerVelocity);
        boolean bl2 = bl = this.checkALError() || bl;
        if (bl) {
            this.importantMessage("OpenAL did not initialize properly!");
            throw new SoundSystemException("Problem encountered while loading OpenAL or creating the listener.  Probably cause:  OpenAL not supported", 6);
        }
        super.init();
        ChannelLWJGLOpenAL channelLWJGLOpenAL = (ChannelLWJGLOpenAL)this.normalChannels.get(1);
        try {
            AL10.alSourcef(channelLWJGLOpenAL.ALSource.get(0), 4099, 1.0f);
            if (this.checkALError()) {
                LibraryLWJGLOpenAL.alPitchSupported(true, false);
                throw new SoundSystemException("OpenAL: AL_PITCH not supported.", 13);
            }
            LibraryLWJGLOpenAL.alPitchSupported(true, true);
        }
        catch (Exception exception) {
            LibraryLWJGLOpenAL.alPitchSupported(true, false);
            throw new SoundSystemException("OpenAL: AL_PITCH not supported.", 13);
        }
    }

    public static boolean libraryCompatible() {
        if (AL.isCreated()) {
            return true;
        }
        try {
            AL.create();
        }
        catch (Exception exception) {
            return false;
        }
        try {
            AL.destroy();
        }
        catch (Exception exception) {
            // empty catch block
        }
        return true;
    }

    protected Channel createChannel(int i) {
        IntBuffer intBuffer = BufferUtils.createIntBuffer(1);
        try {
            AL10.alGenSources(intBuffer);
        }
        catch (Exception exception) {
            AL10.alGetError();
            return null;
        }
        if (AL10.alGetError() != 0) {
            return null;
        }
        ChannelLWJGLOpenAL channelLWJGLOpenAL = new ChannelLWJGLOpenAL(i, intBuffer);
        return channelLWJGLOpenAL;
    }

    public void cleanup() {
        super.cleanup();
        Set set = this.bufferMap.keySet();
        for (String string : set) {
            IntBuffer intBuffer = (IntBuffer)this.ALBufferMap.get(string);
            if (intBuffer == null) continue;
            AL10.alDeleteBuffers(intBuffer);
            this.checkALError();
            intBuffer.clear();
        }
        this.bufferMap.clear();
        AL.destroy();
        this.bufferMap = null;
        this.listenerPositionAL = null;
        this.listenerOrientation = null;
        this.listenerVelocity = null;
    }

    /*
     * Enabled aggressive block sorting
     */
    public boolean loadSound(FilenameURL filenameURL) {
        if (this.bufferMap == null) {
            this.bufferMap = new HashMap();
            this.importantMessage("Buffer Map was null in method 'loadSound'");
        }
        if (this.ALBufferMap == null) {
            this.ALBufferMap = new HashMap();
            this.importantMessage("Open AL Buffer Map was null in method'loadSound'");
        }
        if (this.errorCheck(filenameURL == null, "Filename/URL not specified in method 'loadSound'")) {
            return false;
        }
        if (this.bufferMap.get(filenameURL.getFilename()) != null) {
            return true;
        }
        ICodec iCodec = SoundSystemConfig.getCodec(filenameURL.getFilename());
        if (this.errorCheck(iCodec == null, "No codec found for file '" + filenameURL.getFilename() + "' in method 'loadSound'")) {
            return false;
        }
        iCodec.initialize(filenameURL.getURL());
        SoundBuffer soundBuffer = iCodec.readAll();
        iCodec.cleanup();
        iCodec = null;
        if (this.errorCheck(soundBuffer == null, "Sound buffer null in method 'loadSound'")) {
            return false;
        }
        this.bufferMap.put(filenameURL.getFilename(), soundBuffer);
        AudioFormat audioFormat = soundBuffer.audioFormat;
        int n = 0;
        if (audioFormat.getChannels() == 1) {
            if (audioFormat.getSampleSizeInBits() == 8) {
                n = 4352;
            } else {
                if (audioFormat.getSampleSizeInBits() != 16) {
                    this.errorMessage("Illegal sample size in method 'loadSound'");
                    return false;
                }
                n = 4353;
            }
        } else {
            if (audioFormat.getChannels() != 2) {
                this.errorMessage("File neither mono nor stereo in method 'loadSound'");
                return false;
            }
            if (audioFormat.getSampleSizeInBits() == 8) {
                n = 4354;
            } else {
                if (audioFormat.getSampleSizeInBits() != 16) {
                    this.errorMessage("Illegal sample size in method 'loadSound'");
                    return false;
                }
                n = 4355;
            }
        }
        IntBuffer intBuffer = BufferUtils.createIntBuffer(1);
        AL10.alGenBuffers(intBuffer);
        if (this.errorCheck(AL10.alGetError() != 0, "alGenBuffers error when loading " + filenameURL.getFilename())) {
            return false;
        }
        ByteBuffer byteBuffer = BufferUtils.createByteBuffer(soundBuffer.audioData.length);
        byteBuffer.clear();
        byteBuffer.put(soundBuffer.audioData);
        byteBuffer.flip();
        AL10.alBufferData(intBuffer.get(0), n, byteBuffer, (int)audioFormat.getSampleRate());
        if (this.errorCheck(AL10.alGetError() != 0, "alBufferData error when loading " + filenameURL.getFilename()) && this.errorCheck(intBuffer == null, "Sound buffer was not created for " + filenameURL.getFilename())) {
            return false;
        }
        this.ALBufferMap.put(filenameURL.getFilename(), intBuffer);
        return true;
    }

    public void unloadSound(String string) {
        this.ALBufferMap.remove(string);
        super.unloadSound(string);
    }

    public void setMasterVolume(float f) {
        super.setMasterVolume(f);
        AL10.alListenerf(4106, f);
        this.checkALError();
    }

    public void newSource(boolean bl, boolean bl2, boolean bl3, String string, FilenameURL filenameURL, float f, float g, float h, int i, float j) {
        IntBuffer intBuffer = null;
        if (!bl2) {
            intBuffer = (IntBuffer)this.ALBufferMap.get(filenameURL.getFilename());
            if (intBuffer == null && !this.loadSound(filenameURL)) {
                this.errorMessage("Source '" + string + "' was not created " + "because an error occurred while loading " + filenameURL.getFilename());
                return;
            }
            intBuffer = (IntBuffer)this.ALBufferMap.get(filenameURL.getFilename());
            if (intBuffer == null) {
                this.errorMessage("Source '" + string + "' was not created " + "because a sound buffer was not found for " + filenameURL.getFilename());
                return;
            }
        }
        SoundBuffer soundBuffer = null;
        if (!bl2) {
            soundBuffer = (SoundBuffer)this.bufferMap.get(filenameURL.getFilename());
            if (soundBuffer == null && !this.loadSound(filenameURL)) {
                this.errorMessage("Source '" + string + "' was not created " + "because an error occurred while loading " + filenameURL.getFilename());
                return;
            }
            soundBuffer = (SoundBuffer)this.bufferMap.get(filenameURL.getFilename());
            if (soundBuffer == null) {
                this.errorMessage("Source '" + string + "' was not created " + "because audio data was not found for " + filenameURL.getFilename());
                return;
            }
        }
        this.sourceMap.put(string, new SourceLWJGLOpenAL(this.listenerPositionAL, intBuffer, bl, bl2, bl3, string, filenameURL, soundBuffer, f, g, h, i, j, false));
    }

    public void rawDataStream(AudioFormat audioFormat, boolean bl, String string, float f, float g, float h, int i, float j) {
        this.sourceMap.put(string, new SourceLWJGLOpenAL(this.listenerPositionAL, audioFormat, bl, string, f, g, h, i, j));
    }

    public void quickPlay(boolean bl, boolean bl2, boolean bl3, String string, FilenameURL filenameURL, float f, float g, float h, int i, float j, boolean bl4) {
        IntBuffer intBuffer = null;
        if (!bl2) {
            intBuffer = (IntBuffer)this.ALBufferMap.get(filenameURL.getFilename());
            if (intBuffer == null) {
                this.loadSound(filenameURL);
            }
            if ((intBuffer = (IntBuffer)this.ALBufferMap.get(filenameURL.getFilename())) == null) {
                this.errorMessage("Sound buffer was not created for " + filenameURL.getFilename());
                return;
            }
        }
        SoundBuffer soundBuffer = null;
        if (!bl2) {
            soundBuffer = (SoundBuffer)this.bufferMap.get(filenameURL.getFilename());
            if (soundBuffer == null && !this.loadSound(filenameURL)) {
                this.errorMessage("Source '" + string + "' was not created " + "because an error occurred while loading " + filenameURL.getFilename());
                return;
            }
            soundBuffer = (SoundBuffer)this.bufferMap.get(filenameURL.getFilename());
            if (soundBuffer == null) {
                this.errorMessage("Source '" + string + "' was not created " + "because audio data was not found for " + filenameURL.getFilename());
                return;
            }
        }
        SourceLWJGLOpenAL sourceLWJGLOpenAL = new SourceLWJGLOpenAL(this.listenerPositionAL, intBuffer, bl, bl2, bl3, string, filenameURL, soundBuffer, f, g, h, i, j, false);
        this.sourceMap.put(string, sourceLWJGLOpenAL);
        this.play(sourceLWJGLOpenAL);
        if (bl4) {
            sourceLWJGLOpenAL.setTemporary(true);
        }
    }

    public void copySources(HashMap hashMap) {
        if (hashMap == null) {
            return;
        }
        Set set = hashMap.keySet();
        Iterator iterator = set.iterator();
        if (this.bufferMap == null) {
            this.bufferMap = new HashMap();
            this.importantMessage("Buffer Map was null in method 'copySources'");
        }
        if (this.ALBufferMap == null) {
            this.ALBufferMap = new HashMap();
            this.importantMessage("Open AL Buffer Map was null in method'copySources'");
        }
        this.sourceMap.clear();
        while (iterator.hasNext()) {
            String string = (String)iterator.next();
            Source source = (Source)hashMap.get(string);
            if (source == null) continue;
            SoundBuffer soundBuffer = null;
            if (!source.toStream) {
                this.loadSound(source.filenameURL);
                soundBuffer = (SoundBuffer)this.bufferMap.get(source.filenameURL.getFilename());
            }
            if (!source.toStream && soundBuffer == null) continue;
            this.sourceMap.put(string, new SourceLWJGLOpenAL(this.listenerPositionAL, (IntBuffer)this.ALBufferMap.get(source.filenameURL.getFilename()), source, soundBuffer));
        }
    }

    public void setListenerPosition(float f, float g, float h) {
        super.setListenerPosition(f, g, h);
        this.listenerPositionAL.put(0, f);
        this.listenerPositionAL.put(1, g);
        this.listenerPositionAL.put(2, h);
        AL10.alListener(4100, this.listenerPositionAL);
        this.checkALError();
    }

    public void setListenerAngle(float f) {
        super.setListenerAngle(f);
        this.listenerOrientation.put(0, this.listener.lookAt.x);
        this.listenerOrientation.put(2, this.listener.lookAt.z);
        AL10.alListener(4111, this.listenerOrientation);
        this.checkALError();
    }

    public void setListenerOrientation(float f, float g, float h, float i, float j, float k) {
        super.setListenerOrientation(f, g, h, i, j, k);
        this.listenerOrientation.put(0, f);
        this.listenerOrientation.put(1, g);
        this.listenerOrientation.put(2, h);
        this.listenerOrientation.put(3, i);
        this.listenerOrientation.put(4, j);
        this.listenerOrientation.put(5, k);
        AL10.alListener(4111, this.listenerOrientation);
        this.checkALError();
    }

    public void setListenerData(ListenerData listenerData) {
        super.setListenerData(listenerData);
        this.listenerPositionAL.put(0, listenerData.position.x);
        this.listenerPositionAL.put(1, listenerData.position.y);
        this.listenerPositionAL.put(2, listenerData.position.z);
        AL10.alListener(4100, this.listenerPositionAL);
        this.listenerOrientation.put(0, listenerData.lookAt.x);
        this.listenerOrientation.put(1, listenerData.lookAt.y);
        this.listenerOrientation.put(2, listenerData.lookAt.z);
        this.listenerOrientation.put(3, listenerData.up.x);
        this.listenerOrientation.put(4, listenerData.up.y);
        this.listenerOrientation.put(5, listenerData.up.z);
        AL10.alListener(4111, this.listenerOrientation);
        this.checkALError();
    }

    private boolean checkALError() {
        switch (AL10.alGetError()) {
            case 0: {
                return false;
            }
            case 40961: {
                this.errorMessage("Invalid name parameter.");
                return true;
            }
            case 40962: {
                this.errorMessage("Invalid parameter.");
                return true;
            }
            case 40963: {
                this.errorMessage("Invalid enumerated parameter value.");
                return true;
            }
            case 40964: {
                this.errorMessage("Illegal call.");
                return true;
            }
            case 40965: {
                this.errorMessage("Unable to allocate memory.");
                return true;
            }
        }
        this.errorMessage("An unrecognized error occurred.");
        return true;
    }

    public static boolean alPitchSupported() {
        return LibraryLWJGLOpenAL.alPitchSupported(false, false);
    }

    private static synchronized boolean alPitchSupported(boolean bl, boolean bl2) {
        if (bl) {
            alPitchSupported = bl2;
        }
        return alPitchSupported;
    }

    public static String getTitle() {
        return "LWJGL OpenAL";
    }

    public static String getDescription() {
        return "The LWJGL binding of OpenAL.  For more information, see http://www.lwjgl.org";
    }

    public String getClassName() {
        return "LibraryLWJGLOpenAL";
    }
}

