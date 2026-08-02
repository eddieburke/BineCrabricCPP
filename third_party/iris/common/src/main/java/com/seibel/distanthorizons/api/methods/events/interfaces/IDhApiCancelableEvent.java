/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.interfaces;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiCancelableEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public interface IDhApiCancelableEvent<T>
extends IDhApiEvent<T> {
    @Override
    public void fireEvent(DhApiCancelableEventParam<T> var1);

    @Override
    @Deprecated
    default public void fireEvent(DhApiEventParam<T> input) {
        if (!input.getClass().isAssignableFrom(DhApiCancelableEventParam.class)) {
            throw new IllegalArgumentException("Programmer error. [" + IDhApiCancelableEvent.class.getSimpleName() + "] was given a [" + DhApiEventParam.class.getSimpleName() + "] when it should only be given a [" + DhApiCancelableEventParam.class.getSimpleName() + "].");
        }
        this.fireEvent((DhApiCancelableEventParam)input);
    }
}

