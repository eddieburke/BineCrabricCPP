/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import java.util.LinkedList;
import javax.sound.sampled.AudioFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.Library;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemLogger;
import paulscode.sound.Source;

@Environment(value=EnvType.CLIENT)
public class Channel {
    protected Class libraryType = Library.class;
    public int channelType;
    private SoundSystemLogger logger = SoundSystemConfig.getLogger();
    public Source attachedSource = null;

    public Channel(int i) {
        this.channelType = i;
    }

    public void cleanup() {
        this.logger = null;
    }

    public boolean preLoadBuffers(LinkedList linkedList) {
        return true;
    }

    public boolean queueBuffer(byte[] bs) {
        return false;
    }

    public int feedRawAudioData(byte[] bs) {
        return 1;
    }

    public int buffersProcessed() {
        return 0;
    }

    public boolean processBuffer() {
        return false;
    }

    public void setAudioFormat(AudioFormat audioFormat) {
    }

    public void flush() {
    }

    public void close() {
    }

    public void play() {
    }

    public void pause() {
    }

    public void stop() {
    }

    public void rewind() {
    }

    public boolean playing() {
        return false;
    }

    public String getClassName() {
        String string = SoundSystemConfig.getLibraryTitle(this.libraryType);
        if (string.equals("No Sound")) {
            return "Channel";
        }
        return "Channel" + string;
    }

    protected void message(String string) {
        this.logger.message(string, 0);
    }

    protected void importantMessage(String string) {
        this.logger.importantMessage(string, 0);
    }

    protected boolean errorCheck(boolean bl, String string) {
        return this.logger.errorCheck(bl, this.getClassName(), string, 0);
    }

    protected void errorMessage(String string) {
        this.logger.errorMessage(this.getClassName(), string, 0);
    }

    protected void printStackTrace(Exception exception) {
        this.logger.printStackTrace(exception, 1);
    }
}

