/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.objects.DhApiResult;
import com.seibel.distanthorizons.coreapi.DependencyInjection.ApiEventInjector;

public class DhApiEventRegister {
    public static DhApiResult<Void> on(Class<? extends IDhApiEvent> eventInterface, IDhApiEvent eventHandlerImplementation) {
        try {
            ApiEventInjector.INSTANCE.bind(eventInterface, eventHandlerImplementation);
            return DhApiResult.createSuccess();
        }
        catch (IllegalStateException e) {
            return DhApiResult.createFail(e.getMessage());
        }
    }

    public static DhApiResult<Void> off(Class<? extends IDhApiEvent> eventInterface, Class<IDhApiEvent> eventHandlerClass) {
        if (ApiEventInjector.INSTANCE.unbind(eventInterface, eventHandlerClass)) {
            return DhApiResult.createSuccess();
        }
        return DhApiResult.createFail("No event handler [" + eventHandlerClass.getSimpleName() + "] was bound for the event [" + eventInterface.getSimpleName() + "].");
    }
}

