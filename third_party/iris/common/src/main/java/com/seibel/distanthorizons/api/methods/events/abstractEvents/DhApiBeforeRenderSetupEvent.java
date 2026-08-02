/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;

public abstract class DhApiBeforeRenderSetupEvent
implements IDhApiEvent<DhApiRenderParam> {
    public abstract void beforeSetup(DhApiEventParam<DhApiRenderParam> var1);

    @Override
    public final void fireEvent(DhApiEventParam<DhApiRenderParam> input) {
        this.beforeSetup(input);
    }
}

