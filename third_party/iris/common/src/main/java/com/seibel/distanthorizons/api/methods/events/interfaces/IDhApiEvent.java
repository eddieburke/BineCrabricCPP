/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.interfaces;

import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;

public interface IDhApiEvent<T>
extends IBindable {
    default public boolean removeAfterFiring() {
        return false;
    }

    public void fireEvent(DhApiEventParam<T> var1);
}

