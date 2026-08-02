/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects;

public class DhApiResult<T> {
    public final boolean success;
    public final String message;
    public final T payload;

    private DhApiResult(boolean success, String message) {
        this(success, message, null);
    }

    private DhApiResult(boolean success, String message, T payload) {
        this.success = success;
        this.message = message == null ? "" : message;
        this.payload = payload;
    }

    public static <Pt> DhApiResult<Pt> createSuccess() {
        return new DhApiResult(true, "");
    }

    public static <Pt> DhApiResult<Pt> createSuccess(Pt payload) {
        return new DhApiResult<Pt>(true, "", payload);
    }

    public static <Pt> DhApiResult<Pt> createSuccess(String message, Pt payload) {
        return new DhApiResult<Pt>(true, message, payload);
    }

    public static <Pt> DhApiResult<Pt> createFail(String message) {
        return new DhApiResult(false, message);
    }

    public static <Pt> DhApiResult<Pt> createFail(String message, Pt payload) {
        return new DhApiResult<Pt>(false, message, payload);
    }
}

