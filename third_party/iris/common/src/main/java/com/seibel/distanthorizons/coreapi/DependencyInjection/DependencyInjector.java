/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.DependencyInjection;

import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IDependencyInjector;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class DependencyInjector<BindableType extends IBindable>
implements IDependencyInjector<BindableType> {
    protected final Map<Class<? extends BindableType>, ArrayList<BindableType>> dependencies = new HashMap<Class<? extends BindableType>, ArrayList<BindableType>>();
    protected final Class<? extends BindableType> bindableInterface;
    protected final boolean allowDuplicateBindings;

    public DependencyInjector(Class<BindableType> newBindableInterface) {
        this.bindableInterface = newBindableInterface;
        this.allowDuplicateBindings = false;
    }

    public DependencyInjector(Class<BindableType> newBindableInterface, boolean newAllowDuplicateBindings) {
        this.bindableInterface = newBindableInterface;
        this.allowDuplicateBindings = newAllowDuplicateBindings;
    }

    @Override
    public void bind(Class<? extends BindableType> dependencyInterface, BindableType dependencyImplementation) throws IllegalStateException, IllegalArgumentException {
        if (this.dependencies.containsKey(dependencyInterface) && !this.allowDuplicateBindings) {
            throw new IllegalStateException("The dependency [" + dependencyInterface.getSimpleName() + "] has already been bound.");
        }
        boolean implementsInterface = this.checkIfClassImplements(dependencyImplementation.getClass(), dependencyInterface) || this.checkIfClassExtends(dependencyImplementation.getClass(), dependencyInterface);
        boolean implementsBindable = this.checkIfClassImplements(dependencyImplementation.getClass(), this.bindableInterface);
        if (!implementsInterface) {
            throw new IllegalArgumentException("The dependency [" + dependencyImplementation.getClass().getSimpleName() + "] doesn't implement or extend: [" + dependencyInterface.getSimpleName() + "].");
        }
        if (!implementsBindable) {
            throw new IllegalArgumentException("The dependency [" + dependencyImplementation.getClass().getSimpleName() + "] doesn't implement the interface: [" + IBindable.class.getSimpleName() + "].");
        }
        if (!this.dependencies.containsKey(dependencyInterface)) {
            this.dependencies.put(dependencyInterface, new ArrayList());
        }
        this.dependencies.get(dependencyInterface).add(dependencyImplementation);
    }

    @Override
    public boolean checkIfClassImplements(Class<?> classToTest, Class<?> interfaceToLookFor) {
        if (classToTest.getSuperclass() != Object.class && classToTest.getSuperclass() != null && this.checkIfClassImplements(classToTest.getSuperclass(), interfaceToLookFor)) {
            return true;
        }
        for (Class<?> implementationInterface : classToTest.getInterfaces()) {
            if (implementationInterface.getInterfaces().length != 0 && this.checkIfClassImplements(implementationInterface, interfaceToLookFor)) {
                return true;
            }
            if (!implementationInterface.equals(interfaceToLookFor)) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean checkIfClassExtends(Class<?> classToTest, Class<?> extensionToLookFor) {
        return extensionToLookFor.isAssignableFrom(classToTest);
    }

    @Override
    public <T extends BindableType> T get(Class<T> interfaceClass) throws ClassCastException {
        return (T)((IBindable)this.getInternalLogic(interfaceClass, false).get(0));
    }

    @Override
    public <T extends BindableType> ArrayList<T> getAll(Class<T> interfaceClass) throws ClassCastException {
        return this.getInternalLogic(interfaceClass, false);
    }

    @Override
    public <T extends BindableType> T get(Class<T> interfaceClass, boolean allowIncompleteDependencies) throws ClassCastException {
        return (T)((IBindable)this.getInternalLogic(interfaceClass, allowIncompleteDependencies).get(0));
    }

    private <T extends BindableType> ArrayList<T> getInternalLogic(Class<T> interfaceClass, boolean allowIncompleteDependencies) throws ClassCastException {
        ArrayList<BindableType> dependencyList = this.dependencies.get(interfaceClass);
        if (dependencyList != null && dependencyList.size() != 0) {
            for (IBindable dependency : dependencyList) {
                if (dependency.getDelayedSetupComplete() || allowIncompleteDependencies) continue;
                throw new IllegalStateException("Got dependency of type [" + interfaceClass.getSimpleName() + "], but the dependency's delayed setup hasn't been run!");
            }
            return dependencyList;
        }
        ArrayList<Object> emptyList = new ArrayList<Object>();
        emptyList.add(null);
        return emptyList;
    }

    @Override
    public void clear() {
        this.dependencies.clear();
    }

    @Override
    public void runDelayedSetup() {
        for (Class<? extends BindableType> clazz : this.dependencies.keySet()) {
            BindableType concreteObject = this.get(clazz, true);
            if (concreteObject.getDelayedSetupComplete()) continue;
            concreteObject.finishDelayedSetup();
        }
    }
}

