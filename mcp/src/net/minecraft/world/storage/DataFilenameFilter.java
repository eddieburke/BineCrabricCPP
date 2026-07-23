/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.storage;

import java.io.File;
import java.io.FilenameFilter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import net.minecraft.world.storage.DimensionFileFilterSubclass;

class DataFilenameFilter
implements FilenameFilter {
    public static final Pattern PATTERN = Pattern.compile("c\\.(-?[0-9a-z]+)\\.(-?[0-9a-z]+)\\.dat");

    private DataFilenameFilter() {
    }

    public boolean accept(File file, String s) {
        Matcher matcher = PATTERN.matcher(s);
        return matcher.matches();
    }

    /* synthetic */ DataFilenameFilter(DimensionFileFilterSubclass subclass) {
        this();
    }
}

