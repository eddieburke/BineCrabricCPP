/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.util;

import java.util.Arrays;

public class StringUtil {
    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();

    public static int nthIndexOf(String str, String substr, int n) {
        int pos = str.indexOf(substr);
        while (--n > 0 && pos != -1) {
            pos = str.indexOf(substr, pos + 1);
        }
        return pos;
    }

    public static <T> String join(String delimiter, T[] list) {
        return StringUtil.join(delimiter, Arrays.asList(list));
    }

    public static <T> String join(String delimiter, Iterable<T> list) {
        StringBuilder stringBuilder = new StringBuilder();
        boolean firstItem = true;
        for (T item : list) {
            if (!firstItem) {
                stringBuilder.append(delimiter);
            }
            stringBuilder.append(item);
            firstItem = false;
        }
        return stringBuilder.toString();
    }

    public static String byteArrayToHexString(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for (int i = 0; i < bytes.length; ++i) {
            int v = bytes[i] & 0xFF;
            hexChars[i * 2] = HEX_ARRAY[v >>> 4];
            hexChars[i * 2 + 1] = HEX_ARRAY[v & 0xF];
        }
        return new String(hexChars);
    }
}

