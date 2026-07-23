/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.sound;

import java.io.File;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.sound.Sound;

@Environment(value=EnvType.CLIENT)
public class SoundEntry {
    private Random random = new Random();
    private Map weightedSoundSet = new HashMap();
    private List loadedSounds = new ArrayList();
    public int loadedSoundCount = 0;
    public boolean isRandom = true;

    public Sound loadStatic(String soundName, File soundFile) {
        try {
            String string = soundName;
            soundName = soundName.substring(0, soundName.indexOf("."));
            if (this.isRandom) {
                while (Character.isDigit(soundName.charAt(soundName.length() - 1))) {
                    soundName = soundName.substring(0, soundName.length() - 1);
                }
            }
            if (!this.weightedSoundSet.containsKey(soundName = soundName.replaceAll("/", "."))) {
                this.weightedSoundSet.put(soundName, new ArrayList());
            }
            Sound sound = new Sound(string, soundFile.toURI().toURL());
            ((List)this.weightedSoundSet.get(soundName)).add(sound);
            this.loadedSounds.add(sound);
            ++this.loadedSoundCount;
            return sound;
        }
        catch (MalformedURLException malformedURLException) {
            malformedURLException.printStackTrace();
            throw new RuntimeException(malformedURLException);
        }
    }

    public Sound get(String id) {
        List list = (List)this.weightedSoundSet.get(id);
        if (list == null) {
            return null;
        }
        return (Sound)list.get(this.random.nextInt(list.size()));
    }

    public Sound getSounds() {
        if (this.loadedSounds.size() == 0) {
            return null;
        }
        return (Sound)this.loadedSounds.get(this.random.nextInt(this.loadedSounds.size()));
    }
}

