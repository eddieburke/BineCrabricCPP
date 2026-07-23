/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import java.net.URL;
import javax.sound.sampled.AudioFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.SoundBuffer;

@Environment(value=EnvType.CLIENT)
public interface ICodec {
    public void reverseByteOrder(boolean var1);

    public boolean initialize(URL var1);

    public boolean initialized();

    public SoundBuffer read();

    public SoundBuffer readAll();

    public boolean endOfStream();

    public void cleanup();

    public AudioFormat getAudioFormat();
}

