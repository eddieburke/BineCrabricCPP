/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiLevelLoadEvent
implements IDhApiEvent<EventParam> {
    public abstract void onLevelLoad(DhApiEventParam<EventParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<EventParam> input) {
        this.onLevelLoad(input);
    }

    public static class EventParam
    implements IDhApiEventParam {
        public final IDhApiLevelWrapper levelWrapper;

        public EventParam(IDhApiLevelWrapper newLevelWrapper) {
            this.levelWrapper = newLevelWrapper;
        }

        @Override
        public EventParam copy() {
            return new EventParam(this.levelWrapper);
        }
    }
}

