/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.sharedParameterObjects;

import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;

public class DhApiCancelableEventParam<T>
extends DhApiEventParam<T> {
    private boolean eventCanceled = false;

    public DhApiCancelableEventParam(T value) {
        super(value);
    }

    public void cancelEvent() {
        this.eventCanceled = true;
    }

    public boolean isEventCanceled() {
        return this.eventCanceled;
    }
}

