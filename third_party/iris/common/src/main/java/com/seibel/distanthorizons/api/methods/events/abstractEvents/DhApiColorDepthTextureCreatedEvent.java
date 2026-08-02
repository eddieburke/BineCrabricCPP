/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiColorDepthTextureCreatedEvent
implements IDhApiEvent<EventParam> {
    public abstract void onResize(DhApiEventParam<EventParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<EventParam> event) {
        this.onResize(event);
    }

    public static class EventParam
    implements IDhApiEventParam {
        public final int previousWidth;
        public final int previousHeight;
        public final int newWidth;
        public final int newHeight;

        public EventParam(int previousWidth, int previousHeight, int newWidth, int newHeight) {
            this.previousWidth = previousWidth;
            this.previousHeight = previousHeight;
            this.newWidth = newWidth;
            this.newHeight = newHeight;
        }

        @Override
        public EventParam copy() {
            return new EventParam(this.previousWidth, this.previousHeight, this.newWidth, this.newHeight);
        }
    }
}

