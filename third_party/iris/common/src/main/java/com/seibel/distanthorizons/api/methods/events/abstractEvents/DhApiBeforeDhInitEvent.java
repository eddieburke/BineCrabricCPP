/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiBeforeDhInitEvent
implements IDhApiEvent<Void> {
    public abstract void beforeDistantHorizonsInit(DhApiEventParam<Void> var1);

    @Override
    public final void fireEvent(DhApiEventParam<Void> input) {
        this.beforeDistantHorizonsInit(input);
    }
}

