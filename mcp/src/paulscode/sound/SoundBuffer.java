/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import javax.sound.sampled.AudioFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class SoundBuffer {
    public byte[] audioData;
    public AudioFormat audioFormat;

    public SoundBuffer(byte[] bs, AudioFormat audioFormat) {
        this.audioData = bs;
        this.audioFormat = audioFormat;
    }

    public void cleanup() {
        this.audioData = null;
        this.audioFormat = null;
    }

    public void trimData(int i) {
        if (this.audioData == null || i == 0) {
            this.audioData = null;
        } else if (this.audioData.length > i) {
            byte[] byArray = new byte[i];
            System.arraycopy(this.audioData, 0, byArray, 0, i);
            this.audioData = byArray;
        }
    }
}

