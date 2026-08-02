/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;
import com.seibel.distanthorizons.api.objects.math.DhApiVec3f;

public abstract class DhApiBeforeBufferRenderEvent
implements IDhApiEvent<EventParam> {
    public abstract void beforeRender(DhApiEventParam<EventParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<EventParam> input) {
        this.beforeRender(input);
    }

    public static class EventParam
    extends DhApiRenderParam
    implements IDhApiEventParam {
        public final DhApiVec3f modelPos;

        public EventParam(DhApiRenderParam parent, DhApiVec3f modelPos) {
            super(parent);
            this.modelPos = modelPos;
        }

        @Override
        public EventParam copy() {
            return new EventParam(this, this.modelPos.copy());
        }
    }
}

