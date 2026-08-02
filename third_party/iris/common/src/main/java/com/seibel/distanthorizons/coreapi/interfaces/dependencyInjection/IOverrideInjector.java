/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;

public interface IOverrideInjector<BindableType extends IBindable> {
    public static final int CORE_PRIORITY = -1;
    public static final int MIN_NON_CORE_OVERRIDE_PRIORITY = 0;
    public static final int DEFAULT_NON_CORE_OVERRIDE_PRIORITY = 10;

    public void bind(Class<? extends IDhApiOverrideable> var1, IDhApiOverrideable var2) throws IllegalStateException, IllegalArgumentException;

    public <T extends IDhApiOverrideable> T get(Class<T> var1) throws ClassCastException;

    public <T extends IDhApiOverrideable> T get(Class<T> var1, int var2) throws ClassCastException;

    public void unbind(Class<? extends IDhApiOverrideable> var1, IDhApiOverrideable var2);

    public void clear();
}

