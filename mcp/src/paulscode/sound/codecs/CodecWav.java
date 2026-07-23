/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound.codecs;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ShortBuffer;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.ICodec;
import paulscode.sound.SoundBuffer;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemLogger;

@Environment(value=EnvType.CLIENT)
public class CodecWav
implements ICodec {
    private static final boolean GET = false;
    private static final boolean SET = true;
    private static final boolean XXX = false;
    private boolean endOfStream = false;
    private boolean initialized = false;
    private AudioInputStream myAudioInputStream = null;
    private SoundSystemLogger logger = SoundSystemConfig.getLogger();

    public void reverseByteOrder(boolean bl) {
    }

    public boolean initialize(URL uRL) {
        this.initialized(true, false);
        this.cleanup();
        if (uRL == null) {
            this.errorMessage("url null in method 'initialize'");
            this.cleanup();
            return false;
        }
        try {
            this.myAudioInputStream = AudioSystem.getAudioInputStream(new BufferedInputStream(uRL.openStream()));
        }
        catch (UnsupportedAudioFileException unsupportedAudioFileException) {
            this.errorMessage("Unsupported audio format in method 'initialize'");
            this.printStackTrace(unsupportedAudioFileException);
            return false;
        }
        catch (IOException iOException) {
            this.errorMessage("Error setting up audio input stream in method 'initialize'");
            this.printStackTrace(iOException);
            return false;
        }
        this.endOfStream(true, false);
        this.initialized(true, true);
        return true;
    }

    public boolean initialized() {
        return this.initialized(false, false);
    }

    public SoundBuffer read() {
        int n;
        if (this.myAudioInputStream == null) {
            return null;
        }
        AudioFormat audioFormat = this.myAudioInputStream.getFormat();
        if (audioFormat == null) {
            this.errorMessage("Audio Format null in method 'read'");
            return null;
        }
        int n2 = 0;
        byte[] byArray = new byte[SoundSystemConfig.getStreamingBufferSize()];
        try {
            for (n = 0; !this.endOfStream(false, false) && n < byArray.length; n += n2) {
                n2 = this.myAudioInputStream.read(byArray, n, byArray.length - n);
                if (n2 > 0) continue;
                this.endOfStream(true, true);
                break;
            }
        }
        catch (IOException iOException) {
            this.endOfStream(true, true);
            return null;
        }
        if (n <= 0) {
            return null;
        }
        if (n < byArray.length) {
            byArray = CodecWav.trimArray(byArray, n);
        }
        byte[] byArray2 = CodecWav.convertAudioBytes(byArray, audioFormat.getSampleSizeInBits() == 16);
        SoundBuffer soundBuffer = new SoundBuffer(byArray2, audioFormat);
        return soundBuffer;
    }

    public SoundBuffer readAll() {
        int n;
        int n2;
        if (this.myAudioInputStream == null) {
            this.errorMessage("Audio input stream null in method 'readAll'");
            return null;
        }
        AudioFormat audioFormat = this.myAudioInputStream.getFormat();
        if (audioFormat == null) {
            this.errorMessage("Audio Format null in method 'readAll'");
            return null;
        }
        byte[] byArray = null;
        int n3 = audioFormat.getChannels() * (int)this.myAudioInputStream.getFrameLength() * audioFormat.getSampleSizeInBits() / 8;
        if (n3 > 0) {
            byArray = new byte[audioFormat.getChannels() * (int)this.myAudioInputStream.getFrameLength() * audioFormat.getSampleSizeInBits() / 8];
            n2 = 0;
            try {
                for (n = 0; (n2 = this.myAudioInputStream.read(byArray, n, byArray.length - n)) != -1 && n < byArray.length; n += n2) {
                }
            }
            catch (IOException iOException) {
                this.errorMessage("Exception thrown while reading from the AudioInputStream (location #1).");
                this.printStackTrace(iOException);
                return null;
            }
        }
        n = 0;
        int n4 = 0;
        byte[] byArray2 = null;
        byArray2 = new byte[SoundSystemConfig.getFileChunkSize()];
        for (n2 = 0; !this.endOfStream(false, false) && n2 < SoundSystemConfig.getMaxFileSize(); n2 += n) {
            n4 = 0;
            try {
                for (n = 0; n < byArray2.length; n += n4) {
                    n4 = this.myAudioInputStream.read(byArray2, n, byArray2.length - n);
                    if (n4 > 0) continue;
                    this.endOfStream(true, true);
                    break;
                }
            }
            catch (IOException iOException) {
                this.errorMessage("Exception thrown while reading from the AudioInputStream (location #2).");
                this.printStackTrace(iOException);
                return null;
            }
            byArray = CodecWav.appendByteArrays(byArray, byArray2, n);
        }
        byte[] byArray3 = CodecWav.convertAudioBytes(byArray, audioFormat.getSampleSizeInBits() == 16);
        SoundBuffer soundBuffer = new SoundBuffer(byArray3, audioFormat);
        try {
            this.myAudioInputStream.close();
        }
        catch (IOException iOException) {
            // empty catch block
        }
        return soundBuffer;
    }

    public boolean endOfStream() {
        return this.endOfStream(false, false);
    }

    public void cleanup() {
        if (this.myAudioInputStream != null) {
            try {
                this.myAudioInputStream.close();
            }
            catch (Exception exception) {
                // empty catch block
            }
        }
        this.myAudioInputStream = null;
    }

    public AudioFormat getAudioFormat() {
        if (this.myAudioInputStream == null) {
            return null;
        }
        return this.myAudioInputStream.getFormat();
    }

    private synchronized boolean initialized(boolean bl, boolean bl2) {
        if (bl) {
            this.initialized = bl2;
        }
        return this.initialized;
    }

    private synchronized boolean endOfStream(boolean bl, boolean bl2) {
        if (bl) {
            this.endOfStream = bl2;
        }
        return this.endOfStream;
    }

    private static byte[] trimArray(byte[] bs, int i) {
        byte[] byArray = null;
        if (bs != null && bs.length > i) {
            byArray = new byte[i];
            System.arraycopy(bs, 0, byArray, 0, i);
        }
        return byArray;
    }

    private static byte[] convertAudioBytes(byte[] bs, boolean bl) {
        Object object;
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(bs.length);
        byteBuffer.order(ByteOrder.nativeOrder());
        ByteBuffer byteBuffer2 = ByteBuffer.wrap(bs);
        byteBuffer2.order(ByteOrder.LITTLE_ENDIAN);
        if (bl) {
            object = byteBuffer.asShortBuffer();
            ShortBuffer shortBuffer = byteBuffer2.asShortBuffer();
            while (shortBuffer.hasRemaining()) {
                ((ShortBuffer)object).put(shortBuffer.get());
            }
        } else {
            while (byteBuffer2.hasRemaining()) {
                byteBuffer.put(byteBuffer2.get());
            }
        }
        byteBuffer.rewind();
        if (!byteBuffer.hasArray()) {
            object = new byte[byteBuffer.capacity()];
            byteBuffer.get((byte[])object);
            byteBuffer.clear();
            return object;
        }
        return byteBuffer.array();
    }

    private static byte[] appendByteArrays(byte[] bs, byte[] cs, int i) {
        byte[] byArray;
        if (bs == null && cs == null) {
            return null;
        }
        if (bs == null) {
            byArray = new byte[i];
            System.arraycopy(cs, 0, byArray, 0, i);
            cs = null;
        } else if (cs == null) {
            byArray = new byte[bs.length];
            System.arraycopy(bs, 0, byArray, 0, bs.length);
            bs = null;
        } else {
            byArray = new byte[bs.length + i];
            System.arraycopy(bs, 0, byArray, 0, bs.length);
            System.arraycopy(cs, 0, byArray, bs.length, i);
            bs = null;
            cs = null;
        }
        return byArray;
    }

    private void errorMessage(String string) {
        this.logger.errorMessage("CodecWav", string, 0);
    }

    private void printStackTrace(Exception exception) {
        this.logger.printStackTrace(exception, 1);
    }
}

