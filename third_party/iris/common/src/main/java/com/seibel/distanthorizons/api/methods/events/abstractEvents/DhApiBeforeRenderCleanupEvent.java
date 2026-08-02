/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;

public abstract class DhApiBeforeRenderCleanupEvent
implements IDhApiEvent<DhApiRenderParam> {
    public abstract void beforeCleanup(DhApiEventParam<DhApiRenderParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<DhApiRenderParam> event) {
        this.beforeCleanup(event);
    }
}

