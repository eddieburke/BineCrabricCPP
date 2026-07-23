/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package paulscode.sound;

import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import paulscode.sound.SimpleThread;
import paulscode.sound.SoundSystemConfig;
import paulscode.sound.SoundSystemLogger;
import paulscode.sound.Source;

@Environment(value=EnvType.CLIENT)
public class StreamThread
extends SimpleThread {
    private SoundSystemLogger logger;
    private List streamingSources;
    private final Object listLock = new Object();

    public StreamThread() {
        this.logger = SoundSystemConfig.getLogger();
        this.streamingSources = new LinkedList();
    }

    protected void cleanup() {
        this.kill();
        super.cleanup();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void run() {
        try {
            this.snooze(3600000L);
            while (!this.dying()) {
                while (!this.dying() && !this.streamingSources.isEmpty()) {
                    Object object = this.listLock;
                    synchronized (object) {
                        ListIterator listIterator = this.streamingSources.listIterator();
                        while (!this.dying() && listIterator.hasNext()) {
                            Source source = (Source)listIterator.next();
                            if (source == null) {
                                listIterator.remove();
                                continue;
                            }
                            if (source.stopped()) {
                                if (source.rawDataStream) continue;
                                listIterator.remove();
                                continue;
                            }
                            if (!source.active()) {
                                if (source.toLoop || source.rawDataStream) {
                                    source.toPlay = true;
                                }
                                listIterator.remove();
                                continue;
                            }
                            if (source.paused()) continue;
                            source.checkFadeOut();
                            if (source.stream() || source.rawDataStream || source.channel != null && source.channel.processBuffer()) continue;
                            if (source.toLoop) {
                                if (source.playing()) continue;
                                if (source.checkFadeOut()) {
                                    source.preLoad = true;
                                    continue;
                                }
                                source.incrementSoundSequence();
                                source.preLoad = true;
                                continue;
                            }
                            if (source.playing() || source.checkFadeOut()) continue;
                            if (source.incrementSoundSequence()) {
                                source.preLoad = true;
                                continue;
                            }
                            listIterator.remove();
                        }
                    }
                    if (this.dying() || this.streamingSources.isEmpty()) continue;
                    this.snooze(20L);
                }
                if (this.dying() || !this.streamingSources.isEmpty()) continue;
                this.snooze(3600000L);
            }
        }
        finally {
            this.cleanup();
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void watch(Source source) {
        if (source == null) {
            return;
        }
        if (this.streamingSources.contains(source)) {
            return;
        }
        Object object = this.listLock;
        synchronized (object) {
            ListIterator listIterator = this.streamingSources.listIterator();
            while (listIterator.hasNext()) {
                Source source2 = (Source)listIterator.next();
                if (source2 == null) {
                    listIterator.remove();
                    continue;
                }
                if (source.channel != source2.channel) continue;
                source2.stop();
                listIterator.remove();
            }
            this.streamingSources.add(source);
        }
    }

    private void message(String val) {
        this.logger.message(val, 0);
    }

    private void importantMessage(String val) {
        this.logger.importantMessage(val, 0);
    }

    private boolean errorCheck(boolean print, String val) {
        return this.logger.errorCheck(print, "StreamThread", val, 0);
    }

    private void errorMessage(String val) {
        this.logger.errorMessage("StreamThread", val, 0);
    }
}

