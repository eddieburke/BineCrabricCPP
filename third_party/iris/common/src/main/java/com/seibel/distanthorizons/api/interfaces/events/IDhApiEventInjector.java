/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.events;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IDependencyInjector;

public interface IDhApiEventInjector
extends IDependencyInjector<IDhApiEvent> {
    public boolean unbind(Class<? extends IDhApiEvent> var1, Class<? extends IDhApiEvent> var2) throws IllegalArgumentException;

    public <T, U extends IDhApiEvent<T>> boolean fireAllEvents(Class<U> var1, T var2);
}

