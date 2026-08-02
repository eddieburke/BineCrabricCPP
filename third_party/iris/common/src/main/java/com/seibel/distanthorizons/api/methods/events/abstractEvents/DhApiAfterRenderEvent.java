/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiAfterRenderEvent
implements IDhApiEvent<Void> {
    public abstract void afterRender(DhApiEventParam<Void> var1);

    @Override
    public final void fireEvent(DhApiEventParam<Void> event) {
        this.afterRender(event);
    }
}

