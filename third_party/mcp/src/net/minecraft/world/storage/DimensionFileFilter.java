/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.world.storage;

import java.io.File;
import java.io.FileFilter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import net.minecraft.world.storage.DimensionFileFilterSubclass;

class DimensionFileFilter
implements FileFilter {
    public static final Pattern PATTERN = Pattern.compile("[0-9a-z]|([0-9a-z][0-9a-z])");

    private DimensionFileFilter() {
    }

    public boolean accept(File file) {
        if (file.isDirectory()) {
            Matcher matcher = PATTERN.matcher(file.getName());
            return matcher.matches();
        }
        return false;
    }

    /* synthetic */ DimensionFileFilter(DimensionFileFilterSubclass subclass) {
        this();
    }
}

