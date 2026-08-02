/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.DependencyInjection;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.coreapi.DependencyInjection.OverridePriorityListContainer;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IOverrideInjector;
import com.seibel.distanthorizons.coreapi.util.StringUtil;
import java.util.HashMap;

public class OverrideInjector
implements IOverrideInjector<IDhApiOverrideable> {
    public static final OverrideInjector INSTANCE = new OverrideInjector();
    private final HashMap<Class<? extends IDhApiOverrideable>, OverridePriorityListContainer> overrideContainerByInterface = new HashMap();
    private final String corePackagePath;

    public OverrideInjector() {
        String thisPackageName = this.getClass().getPackage().getName();
        int secondPackageEndingIndex = StringUtil.nthIndexOf(thisPackageName, ".", 3);
        this.corePackagePath = thisPackageName.substring(0, secondPackageEndingIndex);
    }

    public OverrideInjector(String newCorePackagePath) {
        this.corePackagePath = newCorePackagePath;
    }

    @Override
    public void bind(Class<? extends IDhApiOverrideable> dependencyInterface, IDhApiOverrideable dependencyImplementation) throws IllegalStateException, IllegalArgumentException {
        IDhApiOverrideable existingOverride;
        OverridePriorityListContainer overrideContainer = this.overrideContainerByInterface.get(dependencyInterface);
        if (overrideContainer == null) {
            overrideContainer = new OverridePriorityListContainer();
            this.overrideContainerByInterface.put(dependencyInterface, overrideContainer);
        }
        if (dependencyImplementation.getPriority() == -1) {
            String packageName = dependencyImplementation.getClass().getPackage().getName();
            if (!packageName.startsWith(this.corePackagePath)) {
                throw new IllegalArgumentException("Only Distant Horizons internal objects can use the Override Priority [-1]. Please use a higher number.");
            }
        } else if (dependencyImplementation.getPriority() < 0) {
            throw new IllegalArgumentException("Invalid priority value [" + dependencyImplementation.getPriority() + "], override priorities must be [" + 0 + "] or greater.");
        }
        if ((existingOverride = overrideContainer.getOverrideWithPriority(dependencyImplementation.getPriority())) != null) {
            throw new IllegalStateException("An override already exists with the priority [" + dependencyImplementation.getPriority() + "].");
        }
        overrideContainer.addOverride(dependencyImplementation);
    }

    @Override
    public void unbind(Class<? extends IDhApiOverrideable> dependencyInterface, IDhApiOverrideable dependencyImplementation) {
        OverridePriorityListContainer overrideContainer = this.overrideContainerByInterface.get(dependencyInterface);
        if (overrideContainer != null) {
            overrideContainer.removeOverride(dependencyImplementation);
        }
    }

    @Override
    public <T extends IDhApiOverrideable> T get(Class<T> interfaceClass) throws ClassCastException {
        OverridePriorityListContainer overrideContainer = this.overrideContainerByInterface.get(interfaceClass);
        return (T)(overrideContainer != null ? overrideContainer.getOverrideWithHighestPriority() : null);
    }

    @Override
    public <T extends IDhApiOverrideable> T get(Class<T> interfaceClass, int priority) throws ClassCastException {
        OverridePriorityListContainer overrideContainer = this.overrideContainerByInterface.get(interfaceClass);
        return (T)(overrideContainer != null ? overrideContainer.getOverrideWithPriority(priority) : null);
    }

    @Override
    public void clear() {
        this.overrideContainerByInterface.clear();
    }
}

