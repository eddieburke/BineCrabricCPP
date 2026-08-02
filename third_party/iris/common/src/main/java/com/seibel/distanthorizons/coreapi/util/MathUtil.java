/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util;

public class MathUtil {
    public static int clamp(int min, int value, int max) {
        return Math.min(max, Math.max(value, min));
    }

    public static float clamp(float min, float value, float max) {
        return Math.min(max, Math.max(value, min));
    }

    public static double clamp(double min, double value, double max) {
        return Math.min(max, Math.max(value, min));
    }

    public static int ceilDiv(int value, int divider) {
        return -Math.floorDiv(-value, divider);
    }

    public static byte min(byte a, byte b) {
        return a < b ? a : b;
    }

    public static byte max(byte a, byte b) {
        return a > b ? a : b;
    }

    public static float fastInvSqrt(float numb) {
        float half = 0.5f * numb;
        int i = Float.floatToIntBits(numb);
        i = 1597463007 - (i >> 1);
        numb = Float.intBitsToFloat(i);
        return numb * (1.5f - half * numb * numb);
    }

    public static float pow2(float x) {
        return x * x;
    }

    public static double pow2(double x) {
        return x * x;
    }

    public static int pow2(int x) {
        return x * x;
    }

    public static long pow2(long x) {
        return x * x;
    }

    public static int log2(int numb) {
        return (int)(Math.log(numb) / Math.log(2.0));
    }
}

