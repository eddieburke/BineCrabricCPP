/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiOneTimeEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public abstract class DhApiAfterDhInitEvent
implements IDhApiEvent<Void>,
IDhApiOneTimeEvent<Void> {
    public abstract void afterDistantHorizonsInit(DhApiEventParam<Void> var1);

    @Override
    public final void fireEvent(DhApiEventParam<Void> input) {
        this.afterDistantHorizonsInit(input);
    }
}

