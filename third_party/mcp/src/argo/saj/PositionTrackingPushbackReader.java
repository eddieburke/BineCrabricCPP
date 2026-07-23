/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.saj;

import argo.saj.ThingWithPosition;
import java.io.PushbackReader;
import java.io.Reader;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class PositionTrackingPushbackReader
implements ThingWithPosition {
    private final PushbackReader pushbackReader;
    private int characterCount = 0;
    private int lineCount = 1;
    private boolean lastCharacterWasCarriageReturn = false;

    public PositionTrackingPushbackReader(Reader reader) {
        this.pushbackReader = new PushbackReader(reader);
    }

    public void unread(char c) {
        --this.characterCount;
        if (this.characterCount < 0) {
            this.characterCount = 0;
        }
        this.pushbackReader.unread(c);
    }

    public void uncount(char[] cs) {
        this.characterCount -= cs.length;
        if (this.characterCount < 0) {
            this.characterCount = 0;
        }
    }

    public int read() {
        int n = this.pushbackReader.read();
        this.updateCharacterAndLineCounts(n);
        return n;
    }

    public int read(char[] cs) {
        int n = this.pushbackReader.read(cs);
        for (char c : cs) {
            this.updateCharacterAndLineCounts(c);
        }
        return n;
    }

    private void updateCharacterAndLineCounts(int i) {
        if (13 == i) {
            this.characterCount = 0;
            ++this.lineCount;
            this.lastCharacterWasCarriageReturn = true;
        } else {
            if (10 == i && !this.lastCharacterWasCarriageReturn) {
                this.characterCount = 0;
                ++this.lineCount;
            } else {
                ++this.characterCount;
            }
            this.lastCharacterWasCarriageReturn = false;
        }
    }

    public int getColumn() {
        return this.characterCount;
    }

    public int getRow() {
        return this.lineCount;
    }
}

