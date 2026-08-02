/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiChunkModifiedEvent
implements IDhApiEvent<EventParam> {
    public abstract void onChunkModified(DhApiEventParam<EventParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<EventParam> input) {
        this.onChunkModified(input);
    }

    public static class EventParam
    implements IDhApiEventParam {
        public final IDhApiLevelWrapper levelWrapper;
        public final int chunkX;
        public final int chunkZ;

        public EventParam(IDhApiLevelWrapper newLevelWrapper, int chunkX, int chunkZ) {
            this.levelWrapper = newLevelWrapper;
            this.chunkX = chunkX;
            this.chunkZ = chunkZ;
        }

        @Override
        public EventParam copy() {
            return new EventParam(this.levelWrapper, this.chunkX, this.chunkZ);
        }
    }
}

