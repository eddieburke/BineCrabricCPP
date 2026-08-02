/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection;

public interface IBindable {
    default public void finishDelayedSetup() {
    }

    default public boolean getDelayedSetupComplete() {
        return true;
    }
}

