/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.sound;

import java.io.File;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.sound.CodecMus;
import net.minecraft.client.sound.Sound;
import net.minecraft.client.sound.SoundEntry;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.MathHelper;
import paulscode.sound.SoundSystem;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.codecs.CodecJOrbis;
import paulscode.sound.codecs.CodecWav;
import paulscode.sound.libraries.LibraryLWJGLOpenAL;

@Environment(value=EnvType.CLIENT)
public class SoundManager {
    private static SoundSystem soundSystem;
    private SoundEntry sounds = new SoundEntry();
    private SoundEntry streamingSounds = new SoundEntry();
    private SoundEntry music = new SoundEntry();
    private int soundSourceSuffix = 0;
    private GameOptions gameOptions;
    private static boolean started;
    private Random random = new Random();
    private int timeUntilNextSong = this.random.nextInt(12000);

    public void loadSounds(GameOptions gameOptions) {
        this.streamingSounds.isRandom = false;
        this.gameOptions = gameOptions;
        if (!(started || gameOptions != null && gameOptions.soundVolume == 0.0f && gameOptions.musicVolume == 0.0f)) {
            this.start();
        }
    }

    private void start() {
        try {
            float f = this.gameOptions.soundVolume;
            float f2 = this.gameOptions.musicVolume;
            this.gameOptions.soundVolume = 0.0f;
            this.gameOptions.musicVolume = 0.0f;
            this.gameOptions.save();
            SoundSystemConfig.addLibrary(LibraryLWJGLOpenAL.class);
            SoundSystemConfig.setCodec("ogg", CodecJOrbis.class);
            SoundSystemConfig.setCodec("mus", CodecMus.class);
            SoundSystemConfig.setCodec("wav", CodecWav.class);
            soundSystem = new SoundSystem();
            this.gameOptions.soundVolume = f;
            this.gameOptions.musicVolume = f2;
            this.gameOptions.save();
        }
        catch (Throwable throwable) {
            throwable.printStackTrace();
            System.err.println("error linking with the LibraryJavaSound plug-in");
        }
        started = true;
    }

    public void updateMusicVolume() {
        if (!(started || this.gameOptions.soundVolume == 0.0f && this.gameOptions.musicVolume == 0.0f)) {
            this.start();
        }
        if (started) {
            if (this.gameOptions.musicVolume == 0.0f) {
                soundSystem.stop("BgMusic");
            } else {
                soundSystem.setVolume("BgMusic", this.gameOptions.musicVolume);
            }
        }
    }

    public void stop() {
        if (started) {
            soundSystem.cleanup();
        }
    }

    public void loadSound(String id, File soundFile) {
        this.sounds.loadStatic(id, soundFile);
    }

    public void loadStreaming(String id, File soundFile) {
        this.streamingSounds.loadStatic(id, soundFile);
    }

    public void loadMusic(String id, File soundFile) {
        this.music.loadStatic(id, soundFile);
    }

    public void tick() {
        if (!started || this.gameOptions.musicVolume == 0.0f) {
            return;
        }
        if (!soundSystem.playing("BgMusic") && !soundSystem.playing("streaming")) {
            if (this.timeUntilNextSong > 0) {
                --this.timeUntilNextSong;
                return;
            }
            Sound sound = this.music.getSounds();
            if (sound != null) {
                this.timeUntilNextSong = this.random.nextInt(12000) + 12000;
                soundSystem.backgroundMusic("BgMusic", sound.soundFile, sound.id, false);
                soundSystem.setVolume("BgMusic", this.gameOptions.musicVolume);
                soundSystem.play("BgMusic");
            }
        }
    }

    public void updateListenerPosition(LivingEntity player, float scale) {
        if (!started || this.gameOptions.soundVolume == 0.0f) {
            return;
        }
        if (player == null) {
            return;
        }
        float f = player.prevYaw + (player.yaw - player.prevYaw) * scale;
        double d = player.prevX + (player.x - player.prevX) * (double)scale;
        double d2 = player.prevY + (player.y - player.prevY) * (double)scale;
        double d3 = player.prevZ + (player.z - player.prevZ) * (double)scale;
        float f2 = MathHelper.cos(-f * ((float)Math.PI / 180) - (float)Math.PI);
        float f3 = MathHelper.sin(-f * ((float)Math.PI / 180) - (float)Math.PI);
        float f4 = -f3;
        float f5 = 0.0f;
        float f6 = -f2;
        float f7 = 0.0f;
        float f8 = 1.0f;
        float f9 = 0.0f;
        soundSystem.setListenerPosition((float)d, (float)d2, (float)d3);
        soundSystem.setListenerOrientation(f4, f5, f6, f7, f8, f9);
    }

    public void playStreaming(String id, float x, float y, float z, float volume, float pitch) {
        if (!started || this.gameOptions.soundVolume == 0.0f) {
            return;
        }
        String string = "streaming";
        if (soundSystem.playing("streaming")) {
            soundSystem.stop("streaming");
        }
        if (id == null) {
            return;
        }
        Sound sound = this.streamingSounds.get(id);
        if (sound != null && volume > 0.0f) {
            if (soundSystem.playing("BgMusic")) {
                soundSystem.stop("BgMusic");
            }
            float f = 16.0f;
            soundSystem.newStreamingSource(true, string, sound.soundFile, sound.id, false, x, y, z, 2, f * 4.0f);
            soundSystem.setVolume(string, 0.5f * this.gameOptions.soundVolume);
            soundSystem.play(string);
        }
    }

    public void playSound(String id, float x, float y, float z, float volume, float pitch) {
        if (!started || this.gameOptions.soundVolume == 0.0f) {
            return;
        }
        Sound sound = this.sounds.get(id);
        if (sound != null && volume > 0.0f) {
            this.soundSourceSuffix = (this.soundSourceSuffix + 1) % 256;
            String string = "sound_" + this.soundSourceSuffix;
            float f = 16.0f;
            if (volume > 1.0f) {
                f *= volume;
            }
            soundSystem.newSource(volume > 1.0f, string, sound.soundFile, sound.id, false, x, y, z, 2, f);
            soundSystem.setPitch(string, pitch);
            if (volume > 1.0f) {
                volume = 1.0f;
            }
            soundSystem.setVolume(string, volume * this.gameOptions.soundVolume);
            soundSystem.play(string);
        }
    }

    public void playSound(String id, float volume, float pitch) {
        if (!started || this.gameOptions.soundVolume == 0.0f) {
            return;
        }
        Sound sound = this.sounds.get(id);
        if (sound != null) {
            this.soundSourceSuffix = (this.soundSourceSuffix + 1) % 256;
            String string = "sound_" + this.soundSourceSuffix;
            soundSystem.newSource(false, string, sound.soundFile, sound.id, false, 0.0f, 0.0f, 0.0f, 0, 0.0f);
            if (volume > 1.0f) {
                volume = 1.0f;
            }
            soundSystem.setPitch(string, pitch);
            soundSystem.setVolume(string, (volume *= 0.25f) * this.gameOptions.soundVolume);
            soundSystem.play(string);
        }
    }

    static {
        started = false;
    }
}

