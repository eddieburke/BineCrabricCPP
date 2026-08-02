/*
 * Decompiled with CFR 0.152.
 * 
 * Could not load the following classes:
 *  org.apache.logging.log4j.LogManager
 *  org.apache.logging.log4j.Logger
 */
package com.seibel.distanthorizons.coreapi.DependencyInjection;

import com.seibel.distanthorizons.api.interfaces.events.IDhApiEventInjector;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiCancelableEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiOneTimeEvent;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiCancelableEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiEventParam;
import com.seibel.distanthorizons.coreapi.DependencyInjection.DependencyInjector;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;
import java.util.ArrayList;
import java.util.HashMap;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class ApiEventInjector
extends DependencyInjector<IDhApiEvent>
implements IDhApiEventInjector {
    public static final ApiEventInjector INSTANCE = new ApiEventInjector();
    private static final Logger LOGGER = LogManager.getLogger((String)ApiEventInjector.class.getSimpleName());
    private final HashMap<Class<? extends IDhApiEvent>, Object> firedOneTimeEventParamsByEventInterface = new HashMap();

    private ApiEventInjector() {
        super(IDhApiEvent.class, true);
    }

    @Override
    public void bind(Class<? extends IDhApiEvent> abstractEvent, IDhApiEvent eventImplementation) throws IllegalStateException, IllegalArgumentException {
        if (IDhApiOneTimeEvent.class.isAssignableFrom(abstractEvent) && this.firedOneTimeEventParamsByEventInterface.containsKey(abstractEvent)) {
            Object parameter = this.firedOneTimeEventParamsByEventInterface.get(abstractEvent);
            DhApiEventParam<Object> eventParam = ApiEventInjector.createEventParamWrapper(eventImplementation, parameter);
            eventImplementation.fireEvent(eventParam);
        }
        super.bind(abstractEvent, eventImplementation);
    }

    @Override
    public boolean unbind(Class<? extends IDhApiEvent> abstractEvent, Class<? extends IDhApiEvent> eventClassToRemove) throws IllegalArgumentException {
        boolean implementsInterface = this.checkIfClassImplements(eventClassToRemove, abstractEvent) || this.checkIfClassExtends(eventClassToRemove, abstractEvent);
        boolean implementsBindable = this.checkIfClassImplements(eventClassToRemove, this.bindableInterface);
        if (!implementsInterface) {
            throw new IllegalArgumentException("The event handler [" + eventClassToRemove.getSimpleName() + "] doesn't implement or extend: [" + abstractEvent.getSimpleName() + "].");
        }
        if (!implementsBindable) {
            throw new IllegalArgumentException("The event handler [" + eventClassToRemove.getSimpleName() + "] doesn't implement the interface: [" + IBindable.class.getSimpleName() + "].");
        }
        if (this.dependencies.containsKey(abstractEvent)) {
            ArrayList dependencyList = (ArrayList)this.dependencies.get(abstractEvent);
            int indexToRemove = -1;
            for (int i = 0; i < dependencyList.size(); ++i) {
                IBindable dependency = (IBindable)dependencyList.get(i);
                if (!dependency.getClass().equals(eventClassToRemove)) continue;
                indexToRemove = i;
                break;
            }
            if (indexToRemove != -1) {
                return dependencyList.remove(indexToRemove) != null;
            }
        }
        return false;
    }

    @Override
    public <T, U extends IDhApiEvent<T>> boolean fireAllEvents(Class<U> abstractEventClass, T eventInput) {
        if (IDhApiOneTimeEvent.class.isAssignableFrom(abstractEventClass) && !this.firedOneTimeEventParamsByEventInterface.containsKey(abstractEventClass)) {
            this.firedOneTimeEventParamsByEventInterface.put(abstractEventClass, eventInput);
        }
        boolean cancelEvent = false;
        ArrayList<U> eventList = this.getAll(abstractEventClass);
        ArrayList<IDhApiEvent> eventsToRemove = new ArrayList<IDhApiEvent>();
        for (IDhApiEvent event : eventList) {
            if (event == null) continue;
            try {
                Object input = eventInput;
                if (eventInput instanceof IDhApiEventParam) {
                    try {
                        input = ((IDhApiEventParam)eventInput).copy();
                    }
                    catch (Exception e) {
                        LOGGER.error("Unable to clone event parameter [" + eventInput.getClass().getSimpleName() + "], error: [" + e.getMessage() + "].", (Throwable)e);
                    }
                }
                DhApiEventParam<T> eventParam = ApiEventInjector.createEventParamWrapper(event, input);
                event.fireEvent(eventParam);
                if (eventParam instanceof DhApiCancelableEventParam) {
                    DhApiCancelableEventParam cancelableEventParam = (DhApiCancelableEventParam)eventParam;
                    cancelEvent |= cancelableEventParam.isEventCanceled();
                }
                if (!event.removeAfterFiring()) continue;
                eventsToRemove.add(event);
            }
            catch (Exception e) {
                LOGGER.error("Exception thrown by event handler [" + event.getClass().getSimpleName() + "] for event type [" + abstractEventClass.getSimpleName() + "], error:" + e.getMessage(), (Throwable)e);
            }
        }
        for (IDhApiEvent eventToRemove : eventsToRemove) {
            this.unbind(abstractEventClass, eventToRemove.getClass());
        }
        return cancelEvent;
    }

    public static <T> DhApiEventParam<T> createEventParamWrapper(IDhApiEvent<T> event, T parameter) {
        return event instanceof IDhApiCancelableEvent ? new DhApiCancelableEventParam<T>(parameter) : new DhApiEventParam<T>(parameter);
    }
}

