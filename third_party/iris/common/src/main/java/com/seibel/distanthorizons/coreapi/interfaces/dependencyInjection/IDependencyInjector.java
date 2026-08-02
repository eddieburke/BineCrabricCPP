/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection;

import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;
import java.util.ArrayList;

public interface IDependencyInjector<BindableType extends IBindable> {
    public void bind(Class<? extends BindableType> var1, BindableType var2) throws IllegalStateException, IllegalArgumentException;

    public boolean checkIfClassImplements(Class<?> var1, Class<?> var2);

    public boolean checkIfClassExtends(Class<?> var1, Class<?> var2);

    public <T extends BindableType> T get(Class<T> var1) throws ClassCastException;

    public <T extends BindableType> ArrayList<T> getAll(Class<T> var1) throws ClassCastException;

    public <T extends BindableType> T get(Class<T> var1, boolean var2) throws ClassCastException;

    public void clear();

    public void runDelayedSetup();
}

