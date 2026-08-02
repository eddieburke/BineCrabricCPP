/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.DependencyInjection;

import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGenerator;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiLevelWrapper;
import com.seibel.distanthorizons.coreapi.DependencyInjection.OverrideInjector;
import com.seibel.distanthorizons.coreapi.util.StringUtil;
import java.util.HashMap;

public class WorldGeneratorInjector {
    public static final WorldGeneratorInjector INSTANCE = new WorldGeneratorInjector();
    private final HashMap<IDhApiLevelWrapper, OverrideInjector> worldGeneratorByLevelWrapper = new HashMap();
    private final String corePackagePath;

    public WorldGeneratorInjector() {
        String thisPackageName = this.getClass().getPackage().getName();
        int secondPackageEndingIndex = StringUtil.nthIndexOf(thisPackageName, ".", 3);
        this.corePackagePath = thisPackageName.substring(0, secondPackageEndingIndex);
    }

    public WorldGeneratorInjector(String newCorePackagePath) {
        this.corePackagePath = newCorePackagePath;
    }

    public void bind(IDhApiLevelWrapper levelForWorldGenerator, IDhApiWorldGenerator worldGeneratorImplementation) throws NullPointerException, IllegalArgumentException {
        if (levelForWorldGenerator == null) {
            throw new NullPointerException("A [" + IDhApiLevelWrapper.class.getSimpleName() + "] is required when binding a world generator.");
        }
        if (worldGeneratorImplementation == null) {
            throw new NullPointerException("No [" + IDhApiWorldGenerator.class.getSimpleName() + "] given.");
        }
        if (!this.worldGeneratorByLevelWrapper.containsKey(levelForWorldGenerator)) {
            this.worldGeneratorByLevelWrapper.put(levelForWorldGenerator, new OverrideInjector(this.corePackagePath));
        }
        this.worldGeneratorByLevelWrapper.get(levelForWorldGenerator).bind(IDhApiWorldGenerator.class, worldGeneratorImplementation);
    }

    public IDhApiWorldGenerator get(IDhApiLevelWrapper levelForWorldGenerator) throws ClassCastException {
        if (!this.worldGeneratorByLevelWrapper.containsKey(levelForWorldGenerator)) {
            return null;
        }
        return this.worldGeneratorByLevelWrapper.get(levelForWorldGenerator).get(IDhApiWorldGenerator.class);
    }

    public void clear() {
        this.worldGeneratorByLevelWrapper.clear();
    }
}

