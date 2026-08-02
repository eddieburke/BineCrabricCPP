/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config;

import java.util.function.Consumer;

public interface IDhApiConfigValue<T> {
    public T getValue();

    public T getTrueValue();

    public T getApiValue();

    public boolean setValue(T var1);

    public boolean clearValue();

    public boolean getCanBeOverrodeByApi();

    public T getDefaultValue();

    public T getMaxValue();

    public T getMinValue();

    public void addChangeListener(Consumer<T> var1);
}

