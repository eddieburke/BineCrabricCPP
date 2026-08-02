/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.methods.events.interfaces;

import com.seibel.distanthorizons.api.methods.events.interfaces.IDhApiEvent;

@Deprecated
public interface IDhServerMessageReceived<T>
extends IDhApiEvent<T> {
    public void serverMessageReceived(String var1, byte[] var2);
}

