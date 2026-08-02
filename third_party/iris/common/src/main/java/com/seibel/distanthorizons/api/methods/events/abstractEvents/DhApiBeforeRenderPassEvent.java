/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;

public abstract class DhApiBeforeRenderPassEvent
implements IDhApiEvent<DhApiRenderParam> {
    public abstract void beforeRender(DhApiEventParam<DhApiRenderParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<DhApiRenderParam> event) {
        this.beforeRender(event);
    }
}

