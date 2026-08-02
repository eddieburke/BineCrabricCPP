/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.interfaces.config;

import java.util.function.Consumer;

public interface IConfigEntry<T> {
    public T getDefaultValue();

    public void setApiValue(T var1);

    public T getApiValue();

    public boolean getAllowApiOverride();

    public void set(T var1);

    public T get();

    public T getTrueValue();

    public void setWithoutSaving(T var1);

    public T getMin();

    public void setMin(T var1);

    public T getMax();

    public void setMax(T var1);

    public void setMinMax(T var1, T var2);

    public String getComment();

    public void setComment(String var1);

    public byte isValid();

    public byte isValid(T var1);

    public boolean equals(IConfigEntry<?> var1);

    public void addValueChangeListener(Consumer<T> var1);
}

