/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound.codecs;

import com.jcraft.jogg.Packet;
import com.jcraft.jogg.Page;
import com.jcraft.jogg.StreamState;
import com.jcraft.jogg.SyncState;
import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.Comment;
import com.jcraft.jorbis.DspState;
import com.jcraft.jorbis.Info;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownServiceException;
import java.nio.ByteOrder;
import javax.sound.sampled.AudioFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.ICodec;
import paulscode.sound.SoundBuffer;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemLogger;

@Environment(value=EnvType.CLIENT)
public class CodecJOrbis
implements ICodec {
    private static final boolean GET = false;
    private static final boolean SET = true;
    private static final boolean XXX = false;
    protected URL url;
    protected URLConnection urlConnection = null;
    private InputStream inputStream;
    private AudioFormat audioFormat;
    private boolean endOfStream = false;
    private boolean initialized = false;
    private byte[] buffer = null;
    private int bufferSize;
    private int count = 0;
    private int index = 0;
    private int convertedBufferSize;
    private float[][][] pcmInfo;
    private int[] pcmIndex;
    private Packet joggPacket = new Packet();
    private Page joggPage = new Page();
    private StreamState joggStreamState = new StreamState();
    private SyncState joggSyncState = new SyncState();
    private DspState jorbisDspState = new DspState();
    private Block jorbisBlock = new Block(this.jorbisDspState);
    private Comment jorbisComment = new Comment();
    private Info jorbisInfo = new Info();
    private SoundSystemLogger logger = SoundSystemConfig.getLogger();
    private static final boolean LITTLE_ENDIAN = ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN;

    public void reverseByteOrder(boolean bl) {
    }

    public boolean initialize(URL uRL) {
        this.initialized(true, false);
        if (this.joggStreamState != null) {
            this.joggStreamState.clear();
        }
        if (this.jorbisBlock != null) {
            this.jorbisBlock.clear();
        }
        if (this.jorbisDspState != null) {
            this.jorbisDspState.clear();
        }
        if (this.jorbisInfo != null) {
            this.jorbisInfo.clear();
        }
        if (this.joggSyncState != null) {
            this.joggSyncState.clear();
        }
        if (this.inputStream != null) {
            try {
                this.inputStream.close();
            }
            catch (IOException iOException) {
                // empty catch block
            }
        }
        this.url = uRL;
        this.bufferSize = SoundSystemConfig.getStreamingBufferSize() / 2;
        this.buffer = null;
        this.count = 0;
        this.index = 0;
        this.joggStreamState = new StreamState();
        this.jorbisBlock = new Block(this.jorbisDspState);
        this.jorbisDspState = new DspState();
        this.jorbisInfo = new Info();
        this.joggSyncState = new SyncState();
        try {
            this.urlConnection = uRL.openConnection();
        }
        catch (UnknownServiceException unknownServiceException) {
            this.errorMessage("Unable to create a UrlConnection in method 'initialize'.");
            this.printStackTrace(unknownServiceException);
            this.cleanup();
            return false;
        }
        catch (IOException iOException) {
            this.errorMessage("Unable to create a UrlConnection in method 'initialize'.");
            this.printStackTrace(iOException);
            this.cleanup();
            return false;
        }
        if (this.urlConnection != null) {
            try {
                this.inputStream = this.openInputStream();
            }
            catch (IOException iOException) {
                this.errorMessage("Unable to acquire inputstream in method 'initialize'.");
                this.printStackTrace(iOException);
                this.cleanup();
                return false;
            }
        }
        this.endOfStream(true, false);
        this.joggSyncState.init();
        this.joggSyncState.buffer(this.bufferSize);
        this.buffer = this.joggSyncState.data;
        try {
            if (!this.readHeader()) {
                this.errorMessage("Error reading the header");
                return false;
            }
        }
        catch (IOException iOException) {
            this.errorMessage("Error reading the header");
            return false;
        }
        this.convertedBufferSize = this.bufferSize * 2;
        this.jorbisDspState.synthesis_init(this.jorbisInfo);
        this.jorbisBlock.init(this.jorbisDspState);
        int n = this.jorbisInfo.channels;
        int n2 = this.jorbisInfo.rate;
        this.audioFormat = new AudioFormat(n2, 16, n, true, false);
        this.pcmInfo = new float[1][][];
        this.pcmIndex = new int[this.jorbisInfo.channels];
        this.initialized(true, true);
        return true;
    }

    protected InputStream openInputStream() {
        return this.urlConnection.getInputStream();
    }

    public boolean initialized() {
        return this.initialized(false, false);
    }

    public SoundBuffer read() {
        byte[] byArray = this.readBytes();
        if (byArray == null) {
            return null;
        }
        return new SoundBuffer(byArray, this.audioFormat);
    }

    public SoundBuffer readAll() {
        byte[] byArray = this.readBytes();
        while (!(this.endOfStream(false, false) || (byArray = CodecJOrbis.appendByteArrays(byArray, this.readBytes())) != null && byArray.length >= SoundSystemConfig.getMaxFileSize())) {
        }
        return new SoundBuffer(byArray, this.audioFormat);
    }

    public boolean endOfStream() {
        return this.endOfStream(false, false);
    }

    public void cleanup() {
        this.joggStreamState.clear();
        this.jorbisBlock.clear();
        this.jorbisDspState.clear();
        this.jorbisInfo.clear();
        this.joggSyncState.clear();
        if (this.inputStream != null) {
            try {
                this.inputStream.close();
            }
            catch (IOException iOException) {
                // empty catch block
            }
        }
        this.joggStreamState = null;
        this.jorbisBlock = null;
        this.jorbisDspState = null;
        this.jorbisInfo = null;
        this.joggSyncState = null;
        this.inputStream = null;
    }

    public AudioFormat getAudioFormat() {
        return this.audioFormat;
    }

    private boolean readHeader() {
        this.index = this.joggSyncState.buffer(this.bufferSize);
        int n = this.inputStream.read(this.joggSyncState.data, this.index, this.bufferSize);
        if (n < 0) {
            n = 0;
        }
        this.joggSyncState.wrote(n);
        if (this.joggSyncState.pageout(this.joggPage) != 1) {
            if (n < this.bufferSize) {
                return true;
            }
            this.errorMessage("Ogg header not recognized in method 'readHeader'.");
            return false;
        }
        this.joggStreamState.init(this.joggPage.serialno());
        this.jorbisInfo.init();
        this.jorbisComment.init();
        if (this.joggStreamState.pagein(this.joggPage) < 0) {
            this.errorMessage("Problem with first Ogg header page in method 'readHeader'.");
            return false;
        }
        if (this.joggStreamState.packetout(this.joggPacket) != 1) {
            this.errorMessage("Problem with first Ogg header packet in method 'readHeader'.");
            return false;
        }
        if (this.jorbisInfo.synthesis_headerin(this.jorbisComment, this.joggPacket) < 0) {
            this.errorMessage("File does not contain Vorbis header in method 'readHeader'.");
            return false;
        }
        int n2 = 0;
        while (n2 < 2) {
            int n3;
            while (n2 < 2 && (n3 = this.joggSyncState.pageout(this.joggPage)) != 0) {
                if (n3 != 1) continue;
                this.joggStreamState.pagein(this.joggPage);
                while (n2 < 2 && (n3 = this.joggStreamState.packetout(this.joggPacket)) != 0) {
                    if (n3 == -1) {
                        this.errorMessage("Secondary Ogg header corrupt in method 'readHeader'.");
                        return false;
                    }
                    this.jorbisInfo.synthesis_headerin(this.jorbisComment, this.joggPacket);
                    ++n2;
                }
            }
            this.index = this.joggSyncState.buffer(this.bufferSize);
            n = this.inputStream.read(this.joggSyncState.data, this.index, this.bufferSize);
            if (n < 0) {
                n = 0;
            }
            if (n == 0 && n2 < 2) {
                this.errorMessage("End of file reached before finished readingOgg header in method 'readHeader'");
                return false;
            }
            this.joggSyncState.wrote(n);
        }
        this.index = this.joggSyncState.buffer(this.bufferSize);
        this.buffer = this.joggSyncState.data;
        return true;
    }

    private byte[] readBytes() {
        if (!this.initialized(false, false)) {
            return null;
        }
        if (this.endOfStream(false, false)) {
            return null;
        }
        byte[] byArray = null;
        switch (this.joggSyncState.pageout(this.joggPage)) {
            case -1: 
            case 0: {
                this.endOfStream(true, true);
                break;
            }
            case 1: {
                this.joggStreamState.pagein(this.joggPage);
                if (this.joggPage.granulepos() == 0L) {
                    this.endOfStream(true, true);
                    break;
                }
                block10: while (true) {
                    switch (this.joggStreamState.packetout(this.joggPacket)) {
                        case -1: 
                        case 0: {
                            break block10;
                        }
                        case 1: {
                            byArray = CodecJOrbis.appendByteArrays(byArray, this.decodeCurrentPacket());
                        }
                        default: {
                            continue block10;
                        }
                    }
                    break;
                }
                if (this.joggPage.eos() == 0) break;
                this.endOfStream(true, true);
            }
        }
        if (!this.endOfStream(false, false)) {
            this.index = this.joggSyncState.buffer(this.bufferSize);
            if (this.index == -1) {
                this.endOfStream(true, true);
            } else {
                this.buffer = this.joggSyncState.data;
                try {
                    this.count = this.inputStream.read(this.buffer, this.index, this.bufferSize);
                }
                catch (Exception exception) {
                    this.printStackTrace(exception);
                    return byArray;
                }
                this.joggSyncState.wrote(this.count);
                if (this.count == 0) {
                    this.endOfStream(true, true);
                }
            }
        }
        return byArray;
    }

    private byte[] decodeCurrentPacket() {
        int n;
        int n2;
        int n3;
        byte[] byArray = new byte[this.convertedBufferSize];
        if (this.jorbisBlock.synthesis(this.joggPacket) == 0) {
            this.jorbisDspState.synthesis_blockin(this.jorbisBlock);
        }
        int n4 = this.convertedBufferSize / (this.jorbisInfo.channels * 2);
        for (n2 = 0; n2 < this.convertedBufferSize && (n = this.jorbisDspState.synthesis_pcmout(this.pcmInfo, this.pcmIndex)) > 0; n2 += n3 * this.jorbisInfo.channels * 2) {
            n3 = n < n4 ? n : n4;
            for (int i = 0; i < this.jorbisInfo.channels; ++i) {
                int n5 = i * 2;
                for (int j = 0; j < n3; ++j) {
                    int n6 = (int)(this.pcmInfo[0][i][this.pcmIndex[i] + j] * 32767.0f);
                    if (n6 > Short.MAX_VALUE) {
                        n6 = Short.MAX_VALUE;
                    }
                    if (n6 < Short.MIN_VALUE) {
                        n6 = Short.MIN_VALUE;
                    }
                    if (n6 < 0) {
                        n6 |= 0x8000;
                    }
                    if (LITTLE_ENDIAN) {
                        byArray[n2 + n5] = (byte)n6;
                        byArray[n2 + n5 + 1] = (byte)(n6 >>> 8);
                    } else {
                        byArray[n2 + n5 + 1] = (byte)n6;
                        byArray[n2 + n5] = (byte)(n6 >>> 8);
                    }
                    n5 += 2 * this.jorbisInfo.channels;
                }
            }
            this.jorbisDspState.synthesis_read(n3);
        }
        byArray = CodecJOrbis.trimArray(byArray, n2);
        return byArray;
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

    private static byte[] appendByteArrays(byte[] bs, byte[] cs) {
        byte[] byArray;
        if (bs == null && cs == null) {
            return null;
        }
        if (bs == null) {
            byArray = new byte[cs.length];
            System.arraycopy(cs, 0, byArray, 0, cs.length);
            cs = null;
        } else if (cs == null) {
            byArray = new byte[bs.length];
            System.arraycopy(bs, 0, byArray, 0, bs.length);
            bs = null;
        } else {
            byArray = new byte[bs.length + cs.length];
            System.arraycopy(bs, 0, byArray, 0, bs.length);
            System.arraycopy(cs, 0, byArray, bs.length, cs.length);
            bs = null;
            cs = null;
        }
        return byArray;
    }

    private void errorMessage(String string) {
        this.logger.errorMessage("CodecJOrbis", string, 0);
    }

    private void printStackTrace(Exception exception) {
        this.logger.printStackTrace(exception, 1);
    }
}

