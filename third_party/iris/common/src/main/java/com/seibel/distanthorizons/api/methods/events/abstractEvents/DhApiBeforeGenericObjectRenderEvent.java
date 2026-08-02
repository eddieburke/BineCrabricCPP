/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.abstractEvents;

import com.seibel.distanthorizons.api.interfaces.render.IDhApiRenderableBoxGroup;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiCancelableEvent;
import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiCancelableEventParam;
import com.seibel.distanthorizons.api.methods.events.sharedParameterObjects.DhApiRenderParam;

public abstract class DhApiBeforeGenericObjectRenderEvent
implements IDhApiCancelableEvent<EventParam> {
    public abstract void beforeRender(DhApiCancelableEventParam<EventParam> var1);

    @Override
    public final void fireEvent(DhApiCancelableEventParam<EventParam> input) {
        this.beforeRender(input);
    }

    public static class EventParam
    extends DhApiRenderParam
    implements IDhApiEventParam {
        public final long boxGroupId;
        public final String resourceLocationNamespace;
        public final String resourceLocationPath;

        public EventParam(DhApiRenderParam renderParam, IDhApiRenderableBoxGroup boxGroup) {
            super(renderParam);
            this.boxGroupId = boxGroup.getId();
            this.resourceLocationNamespace = boxGroup.getResourceLocationNamespace();
            this.resourceLocationPath = boxGroup.getResourceLocationPath();
        }

        public EventParam(DhApiRenderParam renderParam, long boxGroupId, String resourceLocationNamespace, String resourceLocationPath) {
            super(renderParam);
            this.boxGroupId = boxGroupId;
            this.resourceLocationNamespace = resourceLocationNamespace;
            this.resourceLocationPath = resourceLocationPath;
        }

        @Override
        public EventParam copy() {
            return new EventParam(this, this.boxGroupId, this.resourceLocationNamespace, this.resourceLocationPath);
        }
    }
}

