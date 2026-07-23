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
import paulscode.sound.SimpleThread;
import paulscode.sound.SoundSystem;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemLogger;

@Environment(value=EnvType.CLIENT)
public class CommandThread
extends SimpleThread {
    protected SoundSystemLogger logger = SoundSystemConfig.getLogger();
    private SoundSystem soundSystem;
    protected String className = "CommandThread";

    public CommandThread(SoundSystem soundSystem) {
        this.soundSystem = soundSystem;
    }

    protected void cleanup() {
        this.kill();
        this.logger = null;
        this.soundSystem = null;
        super.cleanup();
    }

    public void run() {
        long l;
        long l2 = l = System.currentTimeMillis();
        if (this.soundSystem == null) {
            this.errorMessage("SoundSystem was null in method run().", 0);
            this.cleanup();
            return;
        }
        this.snooze(3600000L);
        while (!this.dying()) {
            this.soundSystem.ManageSources();
            this.soundSystem.CommandQueue(null);
            l2 = System.currentTimeMillis();
            if (!this.dying() && l2 - l > 10000L) {
                l = l2;
                this.soundSystem.removeTemporarySources();
            }
            if (this.dying()) continue;
            this.snooze(3600000L);
        }
        this.cleanup();
    }

    protected void message(String string, int i) {
        this.logger.message(string, i);
    }

    protected void importantMessage(String string, int i) {
        this.logger.importantMessage(string, i);
    }

    protected boolean errorCheck(boolean bl, String string) {
        return this.logger.errorCheck(bl, this.className, string, 0);
    }

    protected void errorMessage(String string, int i) {
        this.logger.errorMessage(this.className, string, i);
    }
}

