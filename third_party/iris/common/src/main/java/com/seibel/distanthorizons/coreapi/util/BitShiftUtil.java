/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util;

public class BitShiftUtil {
    public static int powerOfTwo(int value) {
        return 1 << value;
    }

    public static long powerOfTwo(long value) {
        return 1L << (int)value;
    }

    public static int half(int value) {
        return value >> 1;
    }

    public static long half(long value) {
        return value >> 1;
    }

    public static int divideByPowerOfTwo(int value, int power) {
        return value >> power;
    }

    public static long divideByPowerOfTwo(long value, long power) {
        return value >> (int)power;
    }

    public static int square(int value) {
        return value << 1;
    }

    public static long square(long value) {
        return value << 1;
    }

    public static int pow(int value, int power) {
        return value << power;
    }

    public static long pow(long value, long power) {
        return value << (int)power;
    }
}

