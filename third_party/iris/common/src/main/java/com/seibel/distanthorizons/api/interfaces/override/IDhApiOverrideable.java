/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override;

import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;

public interface IDhApiOverrideable
extends IBindable {
    default public int getPriority() {
        return 10;
    }
}

